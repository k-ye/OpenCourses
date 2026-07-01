import torch
import math


class AdamW(torch.optim.Optimizer):
    def __init__(
        self,
        params,
        lr: float,
        weight_decay: float,
        betas: tuple[float, float],
        eps: float = 1e-8,
    ):
        defaults = {
            "lr": lr,
            "weight_decay": weight_decay,
            "betas": betas,
            "eps": eps,
        }
        super().__init__(params, defaults)

    @torch.no_grad()
    def step(self):
        for group in self.param_groups:
            lr = group["lr"]
            b1, b2 = group["betas"]
            weight_decay = group["weight_decay"]
            eps = group["eps"]
            for p in group["params"]:
                if p.grad is None:
                    continue

                state = self.state[p]
                if len(state) == 0:
                    # lazy init
                    state["t"] = 1
                    state["m"] = torch.zeros_like(p)
                    state["v"] = torch.zeros_like(p)

                p -= lr * weight_decay * p

                grad = p.grad
                m = state["m"]
                v = state["v"]
                # in-place ops, so we don't create temporary tensors, nor need to
                # store m, v back to state
                m *= b1
                m += (1.0 - b1) * grad
                v *= b2
                v += (1.0 - b2) * grad * grad

                t = state["t"]
                alpha_t = lr * math.sqrt(1.0 - b2**t) / (1.0 - b1**t)
                p -= alpha_t * m / (torch.sqrt(v) + eps)

                state["t"] = t + 1
