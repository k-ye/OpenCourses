from cs336_basics.accounting_utils import (
    Model,
    M,
    G,
    B,
    T,
)
from cs336_basics.transformer_accounting import calc_model_size
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


def calc_rmsnorm_activation_size(r: RunConfig) -> int:
    # x: (B, S, d_model)
    return r.batch_size * r.m.context_length * r.m.d_model


def calc_mha_activation_size(r: RunConfig) -> int:
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


def calc_ffn_activation_size(r: RunConfig) -> int:
    # x: (B, S, d_model)
    w1 = r.batch_size * r.m.context_length * r.m.d_ff
    silu = w1
    w3 = r.batch_size * r.m.context_length * r.m.d_ff
    # gated = SiLU(w1) * w3, element-wise product
    gated = w3
    w2 = r.batch_size * r.m.context_length * r.m.d_model
    return w1 + silu + w3 + gated + w2


def calc_transformer_block_activation_size(r: RunConfig) -> int:
    # NOT accounting for the residual sums, i.e.
    #  y = x + MHA(RMSNorm(x))
    #  z = y + FFN(RMSNorm(y))
    ln1 = calc_rmsnorm_activation_size(r)
    a1 = calc_mha_activation_size(r)
    ln2 = calc_rmsnorm_activation_size(r)
    a2 = calc_ffn_activation_size(r)
    return ln1 + a1 + ln2 + a2


def calc_transformer_lm_activation_size(r: RunConfig) -> int:
    initial_token_emb = r.batch_size * r.m.context_length * r.m.d_model
    layers = r.m.num_layers * calc_transformer_block_activation_size(r)
    ln_final = calc_rmsnorm_activation_size(r)
    lm_head = r.batch_size * r.m.context_length * r.m.vocab_size
    return initial_token_emb + layers + ln_final + lm_head


def calc_cross_entropy_logits_activation_size(r: RunConfig) -> int:
    return r.batch_size * r.m.context_length * r.m.vocab_size


def calc_total_activation_size(r: RunConfig) -> int:
    return calc_transformer_lm_activation_size(r) + calc_cross_entropy_logits_activation_size(r)


def calc_params_size(m: Model) -> int:
    return calc_model_size(m)


def calc_gradients_size(m: Model) -> int:
    return calc_params_size(m)


def calc_adamw_states_size(m: Model) -> int:
    return 2 * calc_params_size(m)


def task_b():

    gpt2_xl = Model(
        name="gpt2-xl",
        vocab_size=50_257,
        context_length=1024,
        num_layers=48,
        d_model=1600,
        num_heads=25,
        d_ff=4288,
    )
    r = RunConfig(m=gpt2_xl, batch_size=1)

    ram = 80 * 1024 * 1024 * 1024
    dtype_size = 4  # f32
    activation_size = calc_total_activation_size(r)
    params_size = calc_params_size(gpt2_xl)
    grads_size = calc_gradients_size(gpt2_xl)
    optim_states_size = calc_adamw_states_size(gpt2_xl)
    print(
        f"sizes: activation={activation_size / B:.2f}B params={params_size / B:.2f}B gradients={grads_size / B:.2f}B optimizer_states={optim_states_size / B:.2f}B"
    )

    ram_for_activations = ram - (grads_size + optim_states_size + activation_size) * dtype_size
    res = ram_for_activations / dtype_size / activation_size
    print(f"Max batch size is {int(res)} ({res:.2f})")


if __name__ == "__main__":
    task_b()
