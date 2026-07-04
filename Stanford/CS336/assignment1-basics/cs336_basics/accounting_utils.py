from dataclasses import dataclass


@dataclass
class Model:
    name: str
    vocab_size: int
    context_length: int
    num_layers: int
    d_model: int
    num_heads: int
    d_ff: int

    @property
    def head_dim(self) -> int:
        assert self.d_model % self.num_heads == 0
        return self.d_model // self.num_heads


M = 1000_000
G = 1000 * M
B = G
T = 1000 * G


def matmul_flops(m: int, n: int, p: int) -> int:
    return 2 * m * n * p
