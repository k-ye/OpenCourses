import math

import torch
from einops import rearrange


class Linear(torch.nn.Module):
    def __init__(
        self, in_features: int, out_features: int, device: torch.device | None = None, dtype: torch.dtype | None = None
    ):
        super().__init__()
        sigma_sqr = 2.0 / (in_features + out_features)
        sigma = math.sqrt(sigma_sqr)
        weight = torch.empty(size=(out_features, in_features), dtype=dtype, device=device)
        torch.nn.init.trunc_normal_(weight, mean=0, std=sigma, a=-3.0 * sigma, b=3.0 * sigma)
        self.weight = torch.nn.Parameter(weight)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return x @ self.weight.T


class Embedding(torch.nn.Module):
    def __init__(
        self,
        num_embeddings: int,
        embedding_dim: int,
        device: torch.device | None = None,
        dtype: torch.dtype | None = None,
    ):
        super().__init__()
        weight = torch.empty(size=(num_embeddings, embedding_dim), dtype=dtype, device=device)
        torch.nn.init.trunc_normal_(weight, mean=0, std=1.0, a=-3.0, b=3.0)
        self.weight = torch.nn.Parameter(weight)

    def forward(self, token_ids: torch.Tensor) -> torch.Tensor:
        return self.weight[token_ids]


class RMSNorm(torch.nn.Module):
    def __init__(
        self, d_model: int, eps: float = 1e-5, device: torch.device | None = None, dtype: torch.dtype | None = None
    ):
        super().__init__()
        self.eps = eps
        weight = torch.ones(d_model, device=device, dtype=dtype)
        self.weight = torch.nn.Parameter(weight)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        in_dtype = x.dtype
        x = x.to(torch.float32)
        # rms = einops.reduce(x * x, "b s d_model -> b s 1", "mean")
        rms = torch.mean(x * x, dim=-1, keepdim=True)
        rms = torch.sqrt(rms + self.eps)
        # g = rearrange(self.g, "d -> 1 1 d")
        g = self.weight
        result = x * g / rms
        return result.to(in_dtype)


class SwiGLU(torch.nn.Module):
    def __init__(self, d_model: int, d_ff: int, device: torch.device | None = None, dtype: torch.dtype | None = None):
        super().__init__()

        self.w1 = Linear(d_model, d_ff, device, dtype)
        self.w2 = Linear(d_ff, d_model, device, dtype)
        self.w3 = Linear(d_model, d_ff, device, dtype)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        a: torch.Tensor = self.w1(x)
        silu = a * torch.sigmoid(a)
        b: torch.Tensor = self.w3(x)
        return self.w2(silu * b)


class RotaryPositionalEmbedding(torch.nn.Module):
    def __init__(self, theta: float, d_k: int, max_seq_len: int, device: torch.device | None = None):
        super().__init__()

        assert d_k % 2 == 0, f"d_k must be even, got {d_k}"
        self.d_k = d_k
        position_indices = torch.arange(0, max_seq_len, device=device)
        position_indices = rearrange(position_indices, "d -> d 1")
        pair_indices = torch.arange(0, (d_k // 2), device=device)
        pair_indices = rearrange(pair_indices, "d -> 1 d")

        angles = position_indices / theta ** ((2 * pair_indices) / d_k)
        cos = torch.cos(angles)
        sin = torch.sin(angles)
        self.register_buffer("cos_table", cos, persistent=False)
        self.register_buffer("sin_table", sin, persistent=False)

    def forward(self, x: torch.Tensor, token_positions: torch.Tensor) -> torch.Tensor:
        cos = self.cos_table[token_positions]
        sin = self.sin_table[token_positions]
        x_even, x_odd = x[..., 0::2], x[..., 1::2]
        rot_even = x_even * cos - x_odd * sin
        rot_odd = x_even * sin + x_odd * cos
        res = torch.empty_like(x)
        res[..., 0::2] = rot_even
        res[..., 1::2] = rot_odd
        return res


def softmax(x: torch.Tensor, dim: int) -> torch.Tensor:
    max_vals = torch.max(x, dim=dim, keepdim=True).values
    shifted = x - max_vals
    exp = torch.exp(shifted)
    denom = torch.sum(exp, dim=dim, keepdim=True)
    return exp / denom


def scaled_dot_product_attention(
    Q: torch.Tensor, K: torch.Tensor, V: torch.Tensor, mask: torch.Tensor | None = None
) -> torch.Tensor:
    d_k = K.shape[-1]
    # Q @ K.transpose(-2, -1)
    scores = Q @ rearrange(K, "... s d -> ... d s") / math.sqrt(d_k)
    if mask is not None:
        scores = scores.masked_fill(~mask, -torch.inf)
    attn = softmax(scores, dim=-1)
    return attn @ V


class MultiheadSelfAttention(torch.nn.Module):
    def __init__(
        self,
        d_model: int,
        num_heads: int,
        rope: RotaryPositionalEmbedding | None = None,
        device: torch.device | None = None,
        dtype: torch.dtype | None = None,
    ):
        super().__init__()

        assert d_model % num_heads == 0
        self.head_dim = d_model // num_heads
        self.num_heads = num_heads
        self.q_proj = Linear(d_model, d_model, device, dtype)
        self.k_proj = Linear(d_model, d_model, device, dtype)
        self.v_proj = Linear(d_model, d_model, device, dtype)
        self.output_proj = Linear(d_model, d_model, device, dtype)

        self.rope = rope
        if rope is not None:
            assert self.head_dim == rope.d_k

    def forward(self, x: torch.Tensor, token_positions: torch.Tensor | None = None) -> torch.Tensor:
        # x: (batch seq_len d_model)
        seq_len = x.shape[1]
        q: torch.Tensor = self.q_proj(x)
        pat = "... s (h d) -> ... h s d"
        q = rearrange(q, pat, h=self.num_heads)
        k: torch.Tensor = self.k_proj(x)
        k = rearrange(k, pat, h=self.num_heads)
        v: torch.Tensor = self.v_proj(x)
        v = rearrange(v, pat, h=self.num_heads)
        if self.rope is not None:
            assert token_positions is not None
            q = self.rope.forward(q, token_positions)
            k = self.rope.forward(k, token_positions)
        causal_mask = torch.tril(torch.ones((seq_len, seq_len), device=x.device, dtype=torch.bool))
        out = scaled_dot_product_attention(q, k, v, causal_mask)
        out = rearrange(out, "... h s d -> ... s (h d)")
        out = self.output_proj(out)
        return out


class TransformerBlock(torch.nn.Module):
    def __init__(
        self,
        d_model: int,
        num_heads: int,
        d_ff: int,
        rope: RotaryPositionalEmbedding | None = None,
        device: torch.device | None = None,
        dtype: torch.dtype | None = None,
    ):
        super().__init__()
        self.ln1 = RMSNorm(d_model=d_model, device=device, dtype=dtype)
        self.ln2 = RMSNorm(d_model=d_model, device=device, dtype=dtype)
        self.attn = MultiheadSelfAttention(d_model=d_model, num_heads=num_heads, rope=rope, device=device, dtype=dtype)
        self.ffn = SwiGLU(d_model=d_model, d_ff=d_ff, device=device, dtype=dtype)

    def forward(self, x: torch.Tensor, token_positions: torch.Tensor | None = None) -> torch.Tensor:
        a1 = self.attn.forward(self.ln1.forward(x), token_positions)
        a1 = x + a1
        a2 = self.ffn.forward(self.ln2.forward(a1))
        return a1 + a2
