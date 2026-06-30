import math

import torch


class Linear(torch.nn.Module):
    def __init__(self, in_features: int, out_features: int, device: torch.device | None=None, dtype: torch.dtype|None=None):
        super().__init__()
        sigma_sqr = 2.0 / (in_features + out_features)
        sigma = math.sqrt(sigma_sqr)
        w = torch.empty(size=(out_features, in_features), dtype=dtype, device=device)
        torch.nn.init.trunc_normal_(w, mean=0, std=sigma, a=-3.0*sigma, b=3.0*sigma)
        self.weight = torch.nn.Parameter(w)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return x @ self.weight.T
