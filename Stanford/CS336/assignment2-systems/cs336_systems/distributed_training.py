import torch
import torch.distributed as dist
from torch._utils import _flatten_dense_tensors, _unflatten_dense_tensors


class DDPContainer(torch.nn.Module):
    def __init__(self, module: torch.nn.Module):
        super().__init__()
        self.module = module
        for p in module.parameters():
            dist.broadcast(p.data, src=0)

    def forward(self, *inputs, **kwargs):
        return self.module(*inputs, **kwargs)

    def finish_gradient_synchronization(self):
        self._finish_gradient_packed()

    def _finish_gradient_naive(self):
        for p in self.module.parameters():
            if p.grad is not None:
                dist.all_reduce(p.grad, op=dist.ReduceOp.AVG, async_op=False)

    def _finish_gradient_packed(self):
        grad_tensors = []
        for p in self.module.parameters():
            if p.grad is not None:
                grad_tensors.append(p.grad)
        flat_tensor = _flatten_dense_tensors(grad_tensors)
        dist.all_reduce(flat_tensor, op=dist.ReduceOp.AVG, async_op=False)
        restored_tensors = _unflatten_dense_tensors(flat_tensor, grad_tensors)
        for dst, src in zip(grad_tensors, restored_tensors):
            dst.copy_(src)
