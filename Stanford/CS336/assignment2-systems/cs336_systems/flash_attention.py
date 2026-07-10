import torch
from torch.autograd.function import FunctionCtx


class FlashAttentionFunc(torch.autograd.Function):
    Q_TILE_SIZE = 16
    K_TILE_SIZE = 16

    @staticmethod
    def forward(ctx: FunctionCtx, Q: torch.Tensor, K: torch.Tensor, V: torch.Tensor, is_causal: bool):
        # Q: ... N d
        # K, V: ... N d
        N_q, N_k = Q.shape[-2], K.shape[-2]
        head_dim = Q.shape[-1]
        B_q = FlashAttentionFunc.Q_TILE_SIZE
        B_k = FlashAttentionFunc.K_TILE_SIZE
        assert N_q % B_q == 0
        assert N_k % B_k == 0
        T_q = N_q // B_q
        T_k = N_k // B_k
        for i in range(T_q):
            Q_i = Q[..., i * B_q : (i + 1) * B_q, :]
            # *Q.shape[:-2] to make sure the batch dims are in
            O_i = torch.zeros((*Q.shape[:-2], B_q, head_dim), dtype=torch.float32, device=Q.device)
            l_i = torch.zeros((*Q.shape[:-2], B_q), dtype=torch.float32, device=Q.device)
            m_i = torch.full((*Q.shape[:-2], B_q), -torch.inf, dtype=torch.float32, device=Q.device)
            for j in range(T_k):
                K_j = K[..., j * B_k : (j + 1) * B_k, :]
                V_j = V[..., j * B_k : (j + 1) * B_k, :]
