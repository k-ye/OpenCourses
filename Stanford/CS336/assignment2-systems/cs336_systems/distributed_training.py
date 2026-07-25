import torch
import torch.distributed as dist
from torch._utils import _flatten_dense_tensors, _unflatten_dense_tensors
from typing import Literal, Type, Any
from collections import defaultdict


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
        self._param_to_ranks = {}
        super().__init__(params, kwargs)

    def step(self, closure=None):
        self._opt.step(closure)
        for pg in self.param_groups:
            for param in pg["params"]:
                owner_rank = self._param_to_ranks[param]
                dist.broadcast(param.data, src=owner_rank)

    def add_param_group(self, param_group):
        super().add_param_group(param_group)
        params = []
        for param in param_group["params"]:
            assigned_rank = self._param_count % self._world_size
            if self._rank == assigned_rank:
                params.append(param)
            self._param_to_ranks[param] = assigned_rank
            self._param_count += 1

        if not params:
            return

        param_group_shard = param_group.copy()
        param_group_shard["params"] = params
        if self._opt is None:
            self._opt = self._opt_cls([param_group_shard], **self._kwargs)
        else:
            self._opt.add_param_group(param_group_shard)


class FSDP(torch.nn.Module):
    def __init__(self, module: torch.nn.Module, compute_dtype: torch.dtype | None = None):
        super().__init__()
        self._module = module
        self._dtype = compute_dtype

        rank = dist.get_rank()
        world_size = dist.get_world_size()
        self._rank = rank

        self._params_shard = defaultdict(dict)
        self._params_meta = defaultdict(list)

        for m in module.modules():
            if not isinstance(m, (torch.nn.Linear, torch.nn.Embedding)):
                continue

            for name, p in m.named_parameters(recurse=False):
                shape = p.shape
                flat = p.detach().reshape(-1)
                numel = flat.numel()
                pad = (-numel) % world_size
                if pad:
                    flat = torch.cat([flat, flat.new_zeros(pad)])
                shard_size = flat.numel() // world_size
                shard = flat[rank * shard_size : (rank + 1) * shard_size].clone()
                shard = torch.nn.Parameter(shard)
                setattr(m, name, shard)

                self._params_shard[m][name] = shard
                self._params_meta[m].append((name, shape, pad))
            m.register_forward_pre_hook(self._forward_pre_hook)
            m.register_forward_hook(self._forward_post_hook)

    def forward(self, *inputs, **kwargs):
        return self._module(*input, **kwargs)

    def _forward_pre_hook(self, module: torch.nn.Module, args):
        for meta in self._params_meta[module]:
            name, shape = meta[0], meta[1]

            p = module.get_parameter(name)
            tensor_list = [torch.zeros_like(p) for _ in range(dist.get_world_size())]
            dist.all_gather(tensor_list, p)
            global_tensor = torch.cat(tensor_list, dim=0)
            weight = global_tensor[: shape.numel()].reshape(shape)

            del module._parameters[name]
            setattr(module, name, weight)

    def _forward_post_hook(self, module: torch.nn.Module, args, output):
        for meta in self._params_meta[module]:
            name = meta[0]
            delattr(module, name)
            setattr(module, name, self._params_shard[module][name])
