import torch
from typing import Iterable
import numpy.typing as npt
import numpy as np


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


def get_batch(
    dataset: npt.NDArray,
    batch_size: int,
    context_length: int,
    device: str,
) -> tuple[torch.Tensor, torch.Tensor]:
    dev = torch.device(device)

    starts = np.random.randint(0, len(dataset) - context_length, size=(batch_size,))
    offsets = np.arange(context_length)
    indices = starts[:, None] + offsets[None, :]

    x = torch.tensor(dataset[indices], device=dev, dtype=torch.long)
    y = torch.tensor(dataset[indices + 1], device=dev, dtype=torch.long)

    return (x, y)
