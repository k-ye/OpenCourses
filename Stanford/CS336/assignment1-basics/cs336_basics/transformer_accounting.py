from cs336_basics.accounting_utils import (
    Model,
    matmul_flops,
    M,
    G,
    B,
    T,
)


def calc_model_size(m: Model) -> int:
    token_embeddings = m.vocab_size * m.d_model
    # transformer block
    ln1 = m.d_model
    ln2 = m.d_model
    attn_q_proj = m.d_model * m.d_model
    attn_k_proj = m.d_model * m.d_model
    attn_v_proj = m.d_model * m.d_model
    attn_output_proj = m.d_model * m.d_model
    attn = attn_q_proj + attn_k_proj + attn_v_proj + attn_output_proj
    ffn_w1 = m.d_model * m.d_ff
    ffn_w2 = m.d_ff * m.d_model
    ffn_w3 = m.d_model * m.d_ff
    ffn = ffn_w1 + ffn_w2 + ffn_w3
    block = ln1 + ln2 + attn + ffn
    layers = block * m.num_layers
    # output
    ln_final = m.d_model
    lm_head = m.d_model * m.vocab_size

    total = token_embeddings + layers + ln_final + lm_head
    total_b = total / B
    block_m = block / M
    layers_b = layers / B
    print(
        f"  params: total={total_b:.2f}B layers={layers_b:.2f}B per-block={block_m:.2f}M layers%={layers / total * 100:.2f}%"
    )
    return total


def calc_forward_flops(m: Model) -> int:
    # X: ... seq_len d_model
    qkv_flops = matmul_flops(m.d_model, m.d_model, m.context_length) * 3
    # no RoPE
    # softmax(Q @ K.T) @ V
    sdpa_flops = matmul_flops(m.context_length, m.d_model, m.context_length) + matmul_flops(
        m.context_length, m.context_length, m.d_model
    )
    output_proj_flops = matmul_flops(m.context_length, m.d_model, m.d_model)
    mha_flops = qkv_flops + sdpa_flops + output_proj_flops
    # rms ignored
    ffn_flops = 2 * matmul_flops(m.context_length, m.d_model, m.d_ff) + matmul_flops(
        m.context_length, m.d_ff, m.d_model
    )
    block_flops = mha_flops + ffn_flops
    layers_flops = m.num_layers * block_flops

    # lm head
    lm_head_flops = matmul_flops(m.context_length, m.d_model, m.vocab_size)
    total_flops = layers_flops + lm_head_flops

    total_t = total_flops / T
    layers_t = layers_flops / T
    block_g = block_flops / G
    mha_g = mha_flops / G
    ffn_g = ffn_flops / G

    mha_proj_flops = qkv_flops + output_proj_flops
    mha_proj_g = mha_proj_flops / G
    mha_sdpa_g = sdpa_flops / G

    print("  FLOPs:")
    print(f"    total={total_t:.2f}T (layers={layers_t:.2f}T pct={layers_flops / total_flops * 100:.2f}%)")
    print(
        f"    block={block_g:.2f}G (mha={mha_g:.2f}G pct={mha_flops / block_flops * 100:.2f}% ffn={ffn_g:.2f}G pct={ffn_flops / block_flops * 100:.2f}%)"
    )
    proj_pct = mha_proj_flops / mha_flops
    print(
        f"    mha={mha_g:.2f}G (proj={mha_proj_g:.2f}G pct={proj_pct * 100:.2f}% sdpa={mha_sdpa_g:.2f}G pct={(1 - proj_pct) * 100:.2f}%)"
    )
    return total_flops


def calc_d_ff(d_model: int) -> int:
    return (((d_model * 8 // 3) + 63) // 64) * 64


def main():
    gpt2_small = Model(
        name="gpt2-small",
        vocab_size=50_257,
        context_length=1024,
        num_layers=12,
        d_model=768,
        num_heads=12,
        d_ff=calc_d_ff(768),
    )

    gpt2_medium = Model(
        name="gpt2-medium",
        vocab_size=50_257,
        context_length=1024,
        num_layers=24,
        d_model=1024,
        num_heads=16,
        d_ff=calc_d_ff(1024),
    )

    gpt2_large = Model(
        name="gpt2-large",
        vocab_size=50_257,
        context_length=1024,
        num_layers=36,
        d_model=1280,
        num_heads=20,
        d_ff=calc_d_ff(1280),
    )

    gpt2_xl = Model(
        name="gpt2-xl",
        vocab_size=50_257,
        context_length=1024,
        num_layers=48,
        d_model=1600,
        num_heads=25,
        d_ff=4288,
    )

    for m in [gpt2_small, gpt2_medium, gpt2_large, gpt2_xl]:
        print(m.name)
        calc_model_size(m)
        calc_forward_flops(m)
    return

    gpt2_xl.context_length = 1024
    print("context=1024")
    calc_forward_flops(gpt2_xl)

    gpt2_xl.context_length = 16_384
    print("context=16384")
    calc_forward_flops(gpt2_xl)


if __name__ == "__main__":
    main()
