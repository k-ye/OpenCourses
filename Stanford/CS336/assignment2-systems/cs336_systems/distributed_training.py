import torch
import torch.distributed as dist
from torch._utils import _flatten_dense_tensors, _unflatten_dense_tensors
from typing import Literal, Type, Any


class DDPContainer(torch.nn.Module):
    def __init__(self, module: torch.nn.Module, policy: Literal["naive", "packed", "async"] = "async"):
        super().__init__()
        self.module = module
        self.policy = policy
        self.handles = []

        def reduce_grad_async(p: torch.Tensor):
            handle = dist.all_reduce(p.grad, op=dist.ReduceOp.AVG, async_op=True)
            self.handles.append(handle)

        for p in module.parameters():
            dist.broadcast(p.data, src=0)
            if p.requires_grad and policy == "async":
                p.register_post_accumulate_grad_hook(reduce_grad_async)

    def forward(self, *inputs, **kwargs):
        return self.module(*inputs, **kwargs)

    def finish_gradient_synchronization(self):
        if self.policy == "naive":
            self._finish_gradient_naive()
        elif self.policy == "packed":
            self._finish_gradient_packed()
        else:
            self._finish_gradient_async()

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

    def _finish_gradient_async(self):
        for h in self.handles:
            h.wait()
        self.handles.clear()


class ShardedOptimizer(torch.optim.Optimizer):
    def __init__(self, params, optimizer_cls: Type[torch.optim.Optimizer], **kwargs: Any):
        self._opt_cls = optimizer_cls
        self._kwargs = kwargs
        self._opt = None
        # sharding
        self._param_count = 0
        self._rank = dist.get_rank()
        self._world_size = dist.get_world_size()
        super().__init__(params, kwargs)

    def add_param_group(self, param_group):
        super().add_param_group(param_group)
        params = []
        for param in param_group["params"]:
            if (self._param_count % self._world_size) == self._rank:
                params.append(param)
            self._param_count += 1

        if not params:
            return

        param_group_shard = {k: v for k, v in param_group.items()}
        param_group_shard["params"] = params
        if self._opt is None:
            self._opt = self._opt_cls(
                [
                    param_group_shard,
                ],
                **self._kwargs,
            )
        else:
            self._opt.add_param_group(param_group_shard)
