import argparse
import logging
import torch

from cs336_basics.cli_utils import pick_device, pick_dtype, add_device_dtype_args
from cs336_basics.gpt import TransformerLM
from cs336_basics.optimizer import AdamW
from cs336_basics.accounting_utils import Model


MODEL_SIZE_SMALL = "small"
MODEL_SIZE_MEDIUM = "medium"
MODEL_SIZE_LARGE = "large"
MODEL_SIZE_XL = "xl"
MODEL_SIZE_10B = "10b"


def setup_logging():
    formatter = logging.Formatter(
        fmt="%(asctime)s %(levelname)s %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    console_handler = logging.StreamHandler()
    console_handler.setFormatter(formatter)

    logging.basicConfig(
        level=logging.INFO,
        handlers=[console_handler],
        force=True,
    )


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--model-size",
        type=str,
        choices=[
            MODEL_SIZE_SMALL,
            MODEL_SIZE_MEDIUM,
            MODEL_SIZE_LARGE,
            MODEL_SIZE_XL,
            MODEL_SIZE_10B,
        ],
        default=MODEL_SIZE_LARGE,
    )
    parser.add_argument("--vocab-size", type=int, default=10_000)
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--context-length", type=int, default=512)
    add_device_dtype_args(parser)
    parser.add_argument("--mode", type=str, choices=["fwd", "fwdbwd", "full"], default="full")
    parser.add_argument("--warmup-steps", type=int, default=5)
    parser.add_argument("--measure-steps", type=int, default=10)
    return parser.parse_args()


def get_model_size(args) -> Model:
    models = [
        Model(
            name=MODEL_SIZE_SMALL,
            vocab_size=args.vocab_size,
            context_length=args.context_length,
            d_model=768,
            d_ff=3072,
            num_layers=12,
            num_heads=12,
        ),
        Model(
            name=MODEL_SIZE_MEDIUM,
            vocab_size=args.vocab_size,
            context_length=args.context_length,
            d_model=1024,
            d_ff=4096,
            num_layers=24,
            num_heads=16,
        ),
        Model(
            name=MODEL_SIZE_LARGE,
            vocab_size=args.vocab_size,
            context_length=args.context_length,
            d_model=1280,
            d_ff=5120,
            num_layers=36,
            num_heads=20,
        ),
        Model(
            name=MODEL_SIZE_XL,
            vocab_size=args.vocab_size,
            context_length=args.context_length,
            d_model=2560,
            d_ff=10240,
            num_layers=32,
            num_heads=32,
        ),
        Model(
            name=MODEL_SIZE_10B,
            vocab_size=args.vocab_size,
            context_length=args.context_length,
            d_model=4608,
            d_ff=12288,
            num_layers=50,
            num_heads=36,
        ),
    ]
    for m in models:
        if m.name == args.model_size:
            return m
    raise ValueError(f"unknown model size: {args.model_size}")


def get_random_batch(batch_size: int, ms: Model, device: torch.device) -> tuple[torch.Tensor, torch.Tensor]:
    tokens = torch.randint(low=0, high=ms.vocab_size, size=(batch_size, ms.context_length + 1), device=device, dtype=torch.long)
    x = tokens[:, : ms.context_length]
    y = tokens[:, 1:]
    return (x, y)


def main():
    setup_logging()
    args = parse_args()

    device = pick_device(args.device)
    dtype = pick_dtype(device, args.dtype)

    ms = get_model_size(args)
    model = TransformerLM(
        vocab_size=ms.vocab_size,
        context_length=ms.context_length,
        d_model=ms.d_model,
        num_layers=ms.num_layers,
        num_heads=ms.num_heads,
        d_ff=ms.d_ff,
        rope_theta=10000.0,
        device=device,
        dtype=dtype,
    )
    adamw_opt = AdamW(
        params=model.parameters(),
        lr=1e-3,
        weight_decay=0.01,
        betas=(0.9, 0.999),
        eps=1e-8,
    )
