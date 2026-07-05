## 3.5

### `transformer_accounting`

See [`transformer_accounting.py`](cs336_basics/transformer_accounting.py)

(a)

GPT2-XL size: `1640452800` params, or `1.64B`. RoPE is not taken into account.

Memory rqeuired when using `float32`: `total * sizeof(float32)` ~= `6.56 GB`

(b)

Total: `3.5168` TFLOPs

(c)

It's the FFN within each transformer block.

- FFN per layer: 0.04215 TFLOPs
- FFN all layers: 2.023 TFLOPs
- total forward: 3.517 TFLOPs

So FFN is about 57.5% of the full forward-pass matmul FLOPs.

(d)

```
gpt2-small
  params: total=0.16B layers=0.08B per-block=7.08M layers%=52.39%
  FLOPs:
    total=0.29T (layers=0.21T pct=72.90%)
    block=17.72G (mha=8.05G pct=45.45% ffn=9.66G pct=54.55%)
    mha=8.05G (proj=4.83G pct=60.00% sdpa=3.22G pct=40.00%)
gpt2-medium
  params: total=0.41B layers=0.30B per-block=12.65M layers%=74.68%
  FLOPs:
    total=0.83T (layers=0.72T pct=87.30%)
    block=30.20G (mha=12.88G pct=42.67% ffn=17.31G pct=57.33%)
    mha=12.88G (proj=8.59G pct=66.67% sdpa=4.29G pct=33.33%)
gpt2-large
  params: total=0.84B layers=0.71B per-block=19.83M layers%=84.73%
  FLOPs:
    total=1.79T (layers=1.65T pct=92.63%)
    block=45.97G (mha=18.79G pct=40.88% ffn=27.18G pct=59.12%)
    mha=18.79G (proj=13.42G pct=71.43% sdpa=5.37G pct=28.57%)
gpt2-xl
  params: total=1.64B layers=1.48B per-block=30.83M layers%=90.20%
  FLOPs:
    total=3.52T (layers=3.35T pct=95.32%)
    block=69.84G (mha=27.68G pct=39.64% ffn=42.15G pct=60.36%)
    mha=27.68G (proj=20.97G pct=75.76% sdpa=6.71G pct=24.24%)
```

- Dense projections and FFN scale roughly like `T * d_model^2`.
- Attention score matmuls QK^T and Attn @ V scale like `T^2 * d_model`.

FFN is the largest contributor. As d_model grows, FFN and linear projection FLOPs take a larger share, whie attention matmuls take a smaller share.

(e)

```
context=1024
  FLOPs:
    total=3.52T (layers=3.35T pct=95.32%)
    block=69.84G (mha=27.68G pct=39.64% ffn=42.15G pct=60.36%)
    mha=27.68G (proj=20.97G pct=75.76% sdpa=6.71G pct=24.24%)
context=16384
  FLOPs:
    total=133.58T (layers=130.94T pct=98.03%)
    block=2727.98G (mha=2053.53G pct=75.28% ffn=674.44G pct=24.72%)
    mha=2053.53G (proj=335.54G pct=16.34% sdpa=1717.99G pct=83.66%)
```

Clearly, as `context_length` increases, SDPA takes a larger part of the total FLOPs.

## 4.3

### `adamw_accounting`

(a)

See [`adamw_accounting.py`](cs336_basics/adamw_accounting.py).

(b)

max batch size is `3`

(c)

`14` FLOPs per parameter in one AdamW step.

(d)

FLOPS pct: fwd=33.26% bwd=66.52% optimizer=0.22%

It takes 17088.1 seconds (4.7 hours) to train.
