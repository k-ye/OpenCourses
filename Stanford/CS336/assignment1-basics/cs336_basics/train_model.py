import torch
from typing import Iterable


def do_gradient_clipping(
    parameters: Iterable[torch.nn.Parameter],
    max_l2_norm: float,
    eps: float = 1e-6,
):
    grads = [p.grad for p in parameters if p.grad is not None]
    l2_norm = torch.sqrt(sum(torch.sum(g * g) for g in grads))
    if l2_norm > max_l2_norm:
        scale = max_l2_norm / (l2_norm + eps)
        for g in grads:
            g.mul_(scale)
