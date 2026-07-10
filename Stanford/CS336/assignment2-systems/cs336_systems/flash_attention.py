import torch
from torch.autograd.function import FunctionCtx
import math
from einops import rearrange
import triton
import triton.language as tl


def unsqueeze_last(x: torch.Tensor) -> torch.Tensor:
    # return x.unsqueeze(-1)
    return rearrange(x, "... -> ... 1")


class FlashAttentionFunc(torch.autograd.Function):
    Q_TILE_SIZE = 16
    K_TILE_SIZE = 16

    @staticmethod
    def forward(ctx: FunctionCtx, Q: torch.Tensor, K: torch.Tensor, V: torch.Tensor, is_causal: bool):
        # Q: ... N d
        # K, V: ... N d
        N_q, N_k = Q.shape[-2], K.shape[-2]
        head_dim = Q.shape[-1]
        shapes_before_seq = Q.shape[:-2]
        B_q = FlashAttentionFunc.Q_TILE_SIZE
        B_k = FlashAttentionFunc.K_TILE_SIZE
        assert N_q % B_q == 0
        assert N_k % B_k == 0
        T_q = N_q // B_q
        T_k = N_k // B_k
        d_sqrt = math.sqrt(head_dim)

        O_out = torch.empty((*shapes_before_seq, N_q, head_dim), dtype=Q.dtype, device=Q.device)
        L_out = torch.empty((*shapes_before_seq, N_q), dtype=torch.float32, device=Q.device)
        for i in range(T_q):
            Q_i = Q[..., i * B_q : (i + 1) * B_q, :]
            # *Q.shape[:-2] to make sure the batch dims are in
            # O_i: ... B_q d
            O_i = torch.zeros((*shapes_before_seq, B_q, head_dim), dtype=torch.float32, device=Q.device)
            l_i = torch.zeros((*shapes_before_seq, B_q), dtype=torch.float32, device=Q.device)
            m_i = torch.full((*shapes_before_seq, B_q), -torch.inf, dtype=torch.float32, device=Q.device)
            for j in range(T_k):
                # ... B_k d
                K_j = K[..., j * B_k : (j + 1) * B_k, :]
                V_j = V[..., j * B_k : (j + 1) * B_k, :]
                # S_ij: ... B_q B_k
                S_ij = Q_i @ rearrange(K_j, "... t d -> ... d t") / d_sqrt
                # m_i_new: ... B_q
                m_i_new = torch.max(m_i, torch.amax(S_ij, dim=-1, keepdim=False))
                # P_i_tilde: ... B_q B_k
                P_i_tilde = torch.exp(S_ij - unsqueeze_last(m_i_new))
                # l_i_new: ... B_q
                alpha = torch.exp(m_i - m_i_new)
                l_i_new = alpha * l_i + torch.sum(P_i_tilde, dim=-1, keepdim=False)
                # O_i_new: ... B_q d
                # NOTE: diag(v) @ M = unsqueeze_last(v) * M
                O_i_new = unsqueeze_last(alpha) * O_i + P_i_tilde @ V_j

                O_i = O_i_new
                l_i = l_i_new
                m_i = m_i_new
            O_i = (1.0 / unsqueeze_last(l_i)) * O_i
            L_i = m_i + torch.log(l_i)
            O_out[..., i * B_q : (i + 1) * B_q, :] = O_i
            L_out[..., i * B_q : (i + 1) * B_q] = L_i
        ctx.save_for_backward(Q, K, V, O_out, L_out)
        ctx.is_causal = is_causal
        return O_out

    @staticmethod
    def backward(ctx, *grad_outputs):
        raise NotImplementedError


@triton.jit
def flash_fwd_kernel(
    Q_ptr,
    K_ptr,
    V_ptr,
    O_ptr,
    L_ptr,
    # Q
    stride_qb,
    stride_qq,
    stride_qd,
    # K
    stride_kb,
    stride_kk,
    stride_kd,
    # V
    stride_vb,
    stride_vk,
    stride_vd,
    # O_out
    stride_ob,
    stride_oq,
    stride_od,
    # L_out
    stride_lb,
    stride_lq,
    N_QUERIES,
    N_KEYS,
    scale,
    D: tl.constexpr,
    Q_TILE_SIZE: tl.constexpr,
    K_TILE_SIZE: tl.constexpr,
):
    query_tile_index = tl.program_id(0)
    batch_index = tl.program_id(1)

    Q_block_ptr = tl.make_block_ptr(
        Q_ptr + batch_index * stride_qb,
        shape=(N_QUERIES, D),
        strides=(stride_qq, stride_qd),
        offsets=(query_tile_index * Q_TILE_SIZE, 0),
        block_shape=(Q_TILE_SIZE, D),
        order=(1, 0),
    )
    K_block_ptr = tl.make_block_ptr(
        K_ptr + batch_index * stride_kb,
        shape=(N_KEYS, D),
        strides=(stride_kk, stride_kd),
        offsets=(0, 0),
        block_shape=(K_TILE_SIZE, D),
        order=(1, 0),
    )
    V_block_ptr = tl.make_block_ptr(
        V_ptr + batch_index * stride_vb,
        shape=(N_KEYS, D),
        strides=(stride_vk, stride_vd),
        offsets=(0, 0),
        block_shape=(K_TILE_SIZE, D),
        order=(1, 0),
    )
    O_block_ptr = tl.make_block_ptr(
        O_ptr + batch_index * stride_ob,
        shape=(N_QUERIES, D),
        strides=(stride_oq, stride_od),
        offsets=(query_tile_index * Q_TILE_SIZE, 0),
        block_shape=(Q_TILE_SIZE, D),
        order=(1, 0),
    )
    L_block_ptr = tl.make_block_ptr(
        L_ptr + batch_index * stride_lb,
        shape=(N_QUERIES,),
        strides=(stride_lq,),
        offsets=(query_tile_index * Q_TILE_SIZE,),
        block_shape=(Q_TILE_SIZE,),
        order=(0,),
    )

    O_i = tl.zeros((Q_TILE_SIZE, D), dtype=tl.float32)
    l_i = tl.zeros((Q_TILE_SIZE,), dtype=tl.float32)
    m_i = tl.full((Q_TILE_SIZE,), -math.inf, dtype=tl.float32)

    Q_i = tl.load(Q_block_ptr, boundary_check=(0, 1), padding_option="zero")
    for j in range(tl.cdiv(N_KEYS, K_TILE_SIZE)):
        K_j = tl.load(K_block_ptr, boundary_check=(0, 1), padding_option="zero")

        S_ij = tl.dot(Q_i, tl.trans(K_j)) * scale
        m_i_new = tl.maximum(m_i, tl.max(S_ij, axis=-1))
        P_i_tilde = tl.exp(S_ij - m_i_new[:, None])
        alpha = tl.exp(m_i - m_i_new)
        l_i_new = alpha * l_i + tl.sum(P_i_tilde, axis=-1)

        V_j = tl.load(V_block_ptr, boundary_check=(0, 1), padding_option="zero")
        O_i_new = alpha[:, None] * O_i + tl.dot(P_i_tilde, V_j)

        O_i = O_i_new
        l_i = l_i_new
        m_i = m_i_new

        K_block_ptr = tl.advance(K_block_ptr, (K_TILE_SIZE, 0))
        V_block_ptr = tl.advance(V_block_ptr, (K_TILE_SIZE, 0))

    O_i = (1.0 / l_i[:, None]) * O_i
    L_i = m_i + tl.log(l_i)

    tl.store(O_block_ptr, O_i, boundary_check=(0, 1))
    tl.store(L_block_ptr, L_i, boundary_check=(0,))


class FlashAttentionTritonFunc(torch.autograd.Function):
    Q_TILE_SIZE = 16
    K_TILE_SIZE = 16
