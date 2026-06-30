import math

import torch


class Linear(torch.nn.Module):
    def __init__(
        self, in_features: int, out_features: int, device: torch.device | None = None, dtype: torch.dtype | None = None
    ):
        super().__init__()
        sigma_sqr = 2.0 / (in_features + out_features)
        sigma = math.sqrt(sigma_sqr)
        weight = torch.empty(size=(out_features, in_features), dtype=dtype, device=device)
        torch.nn.init.trunc_normal_(weight, mean=0, std=sigma, a=-3.0 * sigma, b=3.0 * sigma)
        self.weight = torch.nn.Parameter(weight)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return x @ self.weight.T


class Embedding(torch.nn.Module):
    def __init__(
        self,
        num_embeddings: int,
        embedding_dim: int,
        device: torch.device | None = None,
        dtype: torch.dtype | None = None,
    ):
        super().__init__()
        weight = torch.empty(size=(num_embeddings, embedding_dim), dtype=dtype, device=device)
        torch.nn.init.trunc_normal_(weight, mean=0, std=1.0, a=-3.0, b=3.0)
        self.weight = torch.nn.Parameter(weight)

    def forward(self, token_ids: torch.Tensor) -> torch.Tensor:
        return self.weight[token_ids]
