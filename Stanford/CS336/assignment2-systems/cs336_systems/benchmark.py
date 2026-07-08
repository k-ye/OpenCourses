import argparse
import logging
import torch
import timeit
import statistics as stats
from collections import OrderedDict
import torch.cuda.nvtx as nvtx

from cs336_basics.cli_utils import pick_device, pick_dtype, add_device_dtype_args
from cs336_basics.gpt import TransformerLM
from cs336_basics.optimizer import AdamW
from cs336_basics.train_utils import calc_cross_entropy, do_gradient_clipping
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


def mean_stdev(xs: list[float]) -> tuple[float, float]:
    return stats.mean(xs), stats.stdev(xs)


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
    learning_rate = 1e-3
    adamw_opt = AdamW(
        params=model.parameters(),
        lr=learning_rate,
        weight_decay=0.01,
        betas=(0.9, 0.999),
        eps=1e-8,
    )

    total_steps = args.warmup_steps + args.measure_steps
    fwd_durations = []
    bwd_durations = []
    opt_durations = []

    FWD = "fwd"
    BWD = "bwd"
    OPT = "opt"

    should_run_bwd = args.mode != "fwd"
    should_run_opt = args.mode == "full"

    def run_iter(x: torch.Tensor, y: torch.Tensor) -> OrderedDict:
        for group in adamw_opt.param_groups:
            group["lr"] = learning_rate

        stats = OrderedDict()
        adamw_opt.zero_grad()

        start_ts = timeit.default_timer()
        with nvtx.range("forward"):
            logits = model(x)
        torch.cuda.synchronize()
        fwd_duration = timeit.default_timer() - start_ts
        stats[FWD] = fwd_duration

        if not should_run_bwd:
            return stats

        loss = calc_cross_entropy(logits, y)
        start_ts = timeit.default_timer()
        with nvtx.range("backward"):
            loss.backward()
        torch.cuda.synchronize()
        bwd_duration = timeit.default_timer() - start_ts
        stats[BWD] = bwd_duration

        if not should_run_opt:
            return stats

        do_gradient_clipping(model.parameters(), 1.0)
        start_ts = timeit.default_timer()
        with nvtx.range("optimizer"):
            adamw_opt.step()
        torch.cuda.synchronize()
        opt_duration = timeit.default_timer() - start_ts
        stats[OPT] = opt_duration

        return stats

    for it in range(total_steps):
        x, y = get_random_batch(args.batch_size, ms, device)
        is_measure = it >= args.warmup_steps
        label = "measure" if is_measure else "warmup"
        with nvtx.range(label):
            stats = run_iter(x, y)

        line = [f"iter={it}:"] + [f"{k}={v:.3f}s" for k, v in stats.items()]
        line = " ".join(line)
        logging.info(line)

        if is_measure:
            fwd_durations.append(stats[FWD])
            if BWD in stats:
                bwd_durations.append(stats[BWD])
            if OPT in stats:
                opt_durations.append(stats[OPT])

    phase_durations = OrderedDict()
    phase_durations[FWD] = fwd_durations
    if should_run_bwd:
        phase_durations[BWD] = bwd_durations
    if should_run_opt:
        phase_durations[OPT] = opt_durations
    print(f"Benchmark with model_size={ms.name} mode={args.mode}")
    for phase, durations in phase_durations.items():
        mean, std = mean_stdev(durations)
        print(f"{phase}: {mean:.3f}s ± {std:.3f}s")


if __name__ == "__main__":
    main()
