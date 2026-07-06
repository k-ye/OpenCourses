import torch
from typing import Iterable, BinaryIO, IO
import numpy.typing as npt
import numpy as np
from einops import rearrange

import os


def calc_cross_entropy(logits: torch.Tensor, targets: torch.Tensor) -> torch.Tensor:
    max_logits = torch.max(logits, dim=-1, keepdim=True).values
    shifted = logits - max_logits
    log_denom = torch.log(torch.sum(torch.exp(shifted), dim=-1))

    targets_indices = rearrange(targets, "... -> ... 1")
    target_logits = shifted.gather(dim=-1, index=targets_indices)
    target_logits = rearrange(target_logits, "... 1 -> ...")
    loss = log_denom - target_logits
    return loss.mean()


def do_gradient_clipping(
    parameters: Iterable[torch.nn.Parameter],
    max_l2_norm: float,
    eps: float = 1e-6,
):
    grads = [p.grad for p in parameters if p.grad is not None]
    if not grads:
        return
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


def save_checkpoint(
    model: torch.nn.Module,
    optimizer: torch.optim.Optimizer,
    iteration: int,
    out: str | os.PathLike | BinaryIO | IO[bytes],
    *,
    muon_optimizer: torch.optim.Muon | None = None,
):
    obj = {
        "model": model.state_dict(),
        "optimizer": optimizer.state_dict(),
        "iteration": iteration,
    }
    if muon_optimizer:
        obj["muon_optimizer"] = muon_optimizer.state_dict()
    torch.save(obj, out)


def load_checkpoint(
    src: str | os.PathLike | BinaryIO | IO[bytes],
    model: torch.nn.Module,
    optimizer: torch.optim.Optimizer,
    *,
    muon_optimizer: torch.optim.Muon | None = None,
) -> int:
    obj = torch.load(src)
    model.load_state_dict(obj["model"])
    optimizer.load_state_dict(obj["optimizer"])
    if muon_optimizer:
        muon_optimizer.load_state_dict(obj["muon_optimizer"])
    return obj["iteration"]
