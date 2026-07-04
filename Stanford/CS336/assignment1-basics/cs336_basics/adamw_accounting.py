from cs336_basics.accounting_utils import (
    Model,
    M,
    G,
    B,
    T,
)
from dataclasses import dataclass

"""
- Transformer block:
    - RMSNorms
    - MHA
    - QKV projection
    - SwiGLU: W1, W2, W3
- final RMSNorm
- output embedding
- cross-entropy logits

"""

@dataclass
class RunConfig:
    m: Model
    batch_size: int


def calc_rmsnorm_activation_size(r: RunConfig):
    # x: (B, S, d_model)
    return r.batch_size * r.m.context_length * r.m.d_model

def calc_mha_activation_size(r: RunConfig):
    # x: (B, S, d_model)
    # q, k, v: (B, num_heads, S, head_dim)
    q = r.batch_size * r.m.context_length * r.m.d_model
    k = q
    v = q
    # SDPA
    # qk.T: (B, num_heads, S, S)
    # softmax(qk.T)
    # softmax(qk.T) @ V: (B, num_heads, S, head_dim) -> (B, S, d_model)
    # out_proj: (B, S, d_model)
    qk = r.batch_size * r.m.num_heads * r.m.context_length * r.m.context_length
    softmax_probs = qk
    v_weighted = r.batch_size * r.m.num_heads * r.m.context_length * r.m.head_dim
    assert v_weighted == r.batch_size * r.m.context_length * r.m.d_model
    out_proj = v_weighted
    return q + k + v + qk + softmax_probs + v_weighted + out_proj

