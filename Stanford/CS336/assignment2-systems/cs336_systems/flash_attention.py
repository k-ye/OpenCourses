import torch
from torch.autograd.function import FunctionCtx


class FlashAttentionFunc(torch.autograd.Function):
    TILE_SIZE = 16

    @staticmethod
    def forward(ctx: FunctionCtx, Q: torch.Tensor, K: torch.Tensor, V: torch.Tensor, is_causal: bool):
        # Q: ... N d
        # K, V: ... N d
        N_q, N_k = Q.shape[-2], K.shape[-2]
        TILE_SIZE = FlashAttentionFunc.TILE_SIZE
        assert N_q % TILE_SIZE == 0
        assert N_k % TILE_SIZE == 0
        T_q = N_q // TILE_SIZE
        T_k = N_k // TILE_SIZE
        for i in range(T_q):
            Q_i = Q[..., i * TILE_SIZE : (i + 1) * TILE_SIZE, :]
