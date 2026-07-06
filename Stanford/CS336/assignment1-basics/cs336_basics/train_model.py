from cs336_basics.gpt import TransformerLM
from cs336_basics.optimizer import AdamW, calc_lr_cosine_schedule
from cs336_basics.train_utils import calc_cross_entropy, do_gradient_clipping, get_batch, save_checkpoint
from cs336_basics.cli_utils import add_device_dtype_args, pick_device, pick_dtype, set_seed

import argparse
import logging
import os
import pathlib
import torch
import random
import numpy as np
import json
import regex as re


def setup_logging(log_path: pathlib.Path):
    formatter = logging.Formatter(
        fmt="%(asctime)s %(levelname)s %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    console_handler = logging.StreamHandler()
    console_handler.setFormatter(formatter)

    file_handler = logging.FileHandler(log_path)
    file_handler.setFormatter(formatter)

    logging.basicConfig(
        level=logging.INFO,
        handlers=[console_handler, file_handler],
        force=True,
    )


def parse_args():
    parser = argparse.ArgumentParser("Train CS336 language model")

    parser.add_argument(
        "--train-data", required=True, type=pathlib.Path, help="Path to the training dataset token sequence .npy"
    )
    parser.add_argument(
        "--val-data", required=True, type=pathlib.Path, help="Path to the validation dataset token sequence .npy"
    )
    parser.add_argument("--out-dir", required=True, type=pathlib.Path)
    parser.add_argument("--seed", type=int, default=0)

    add_device_dtype_args(parser)
    parser.add_argument("--max-iters", type=int, default=10_000)
    parser.add_argument("--eval-interval", type=int, default=100)
    parser.add_argument("--eval-iters", type=int, default=10)
    parser.add_argument("--log-interval", type=int, default=10)
    parser.add_argument("--checkpoint-interval", type=int, default=1000)

    parser.add_argument("--vocab-size", type=int, default=10_000)
    parser.add_argument("--context-length", type=int, default=256)
    parser.add_argument("--batch-size", type=int, default=64)

    parser.add_argument("--d-model", type=int, default=512)
    parser.add_argument("--d-ff", type=int, default=1344)
    parser.add_argument("--rope-theta", type=float, default=10_000.0)
    parser.add_argument("--num-layers", type=int, default=4)
    parser.add_argument("--num-heads", type=int, default=16)

    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--lr-min", type=float, default=1e-4)
    parser.add_argument("--lr-warmup-iters", type=int, default=100)
    parser.add_argument("--lr-cosine-cycle-iters", type=int, default=-1)
    parser.add_argument("--grad-clipping", type=float, default=1.0)
    parser.add_argument("--adamw-weight-decay", type=float, default=0.01)
    parser.add_argument("--adamw-beta1", type=float, default=0.9)
    parser.add_argument("--adamw-beta2", type=float, default=0.999)
    parser.add_argument("--adamw-eps", type=float, default=1e-8)

    parser.add_argument("--use-muon", action="store_true", default=False)
    parser.add_argument("--muon-lr", type=float, default=1e-2)
    parser.add_argument("--muon-lr-min", type=float, default=1e-3)

    args = parser.parse_args()
    if args.lr_cosine_cycle_iters == -1:
        args.lr_cosine_cycle_iters = args.max_iters
    return args


def validate_args(args):
    if args.lr_cosine_cycle_iters <= args.lr_warmup_iters:
        raise ValueError(f"lr_cosine_cycle_iters={args.lr_cosine_cycle_iters} too small")

    if args.d_model % args.num_heads != 0:
        raise ValueError("Bad num_heads")


def partition_opt_params_for_muon(model: torch.nn.Module) -> tuple[list[torch.nn.Parameter], list[torch.nn.Parameter]]:
    muon_params: list[torch.nn.Parameter] = []
    others_params: list[torch.nn.Parameter] = []
    muon_pat = re.compile(r"layers\.\d+\.(attn|ffn)")
    for name, p in model.named_parameters():
        for_muon = muon_pat.match(name) is not None
        # print(f"param: name={name} shape={p.shape} for_muon={for_muon}")
        if for_muon:
            assert p.ndim == 2
            muon_params.append(p)
        else:
            others_params.append(p)
    return muon_params, others_params


def main():
    args = parse_args()

    out_dir: pathlib.Path = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    setup_logging(out_dir / "train.log")

    args_path = out_dir / "args.json"
    with args_path.open("w") as f:
        json.dump(vars(args), f, indent=2, default=str, sort_keys=True)

    validate_args(args)

    set_seed(args.seed)

    device = pick_device(args.device)
    dtype = pick_dtype(device, args.dtype)

    torch.set_float32_matmul_precision("high")
    model = TransformerLM(
        vocab_size=args.vocab_size,
        context_length=args.context_length,
        d_model=args.d_model,
        num_layers=args.num_layers,
        num_heads=args.num_heads,
        d_ff=args.d_ff,
        rope_theta=args.rope_theta,
        device=device,
        dtype=dtype,
        model_dir=out_dir,
    )

    if args.use_muon:
        muon_params, others_params = partition_opt_params_for_muon(model)
        muon_opt = torch.optim.Muon(
            muon_params,
            lr=args.muon_lr,
        )
        adamw_opt = AdamW(
            params=others_params,
            lr=args.lr,
            weight_decay=args.adamw_weight_decay,
            betas=(args.adamw_beta1, args.adamw_beta2),
            eps=args.adamw_eps,
        )
    else:
        muon_opt = None
        adamw_opt = AdamW(
            params=model.parameters(),
            lr=args.lr,
            weight_decay=args.adamw_weight_decay,
            betas=(args.adamw_beta1, args.adamw_beta2),
            eps=args.adamw_eps,
        )

    train_data = np.load(args.train_data, mmap_mode="r")
    val_data = np.load(args.val_data, mmap_mode="r")

    logging.info("Loaded training dataset, shape=%s dtype=%s", train_data.shape, train_data.dtype)
    logging.info("Loaded validation dataset, shape=%s dtype=%s", val_data.shape, val_data.dtype)

    model = torch.compile(model)
    for it in range(args.max_iters):
        lr = calc_lr_cosine_schedule(
            it=it,
            max_learning_rate=args.lr,
            min_learning_rate=args.lr_min,
            warmup_iters=args.lr_warmup_iters,
            cosine_cycle_iters=args.lr_cosine_cycle_iters,
        )
        muon_lr = calc_lr_cosine_schedule(
            it=it,
            max_learning_rate=args.muon_lr,
            min_learning_rate=args.muon_lr_min,
            warmup_iters=args.lr_warmup_iters,
            cosine_cycle_iters=args.lr_cosine_cycle_iters,
        )
        for group in adamw_opt.param_groups:
            group["lr"] = lr
        if muon_opt:
            for group in muon_opt.param_groups:
                group["lr"] = muon_lr

        x, y = get_batch(train_data, args.batch_size, args.context_length, str(device))

        adamw_opt.zero_grad()
        if muon_opt:
            muon_opt.zero_grad()
        logits = model(x)
        loss = calc_cross_entropy(logits, y)
        loss.backward()

        do_gradient_clipping(model.parameters(), args.grad_clipping)
        adamw_opt.step()
        if muon_opt:
            muon_opt.step()

        if it % args.log_interval == 0:
            logging.info("iter=%d train_loss=%.4f learning_rate=%g", it, loss.item(), lr)

        if it % args.eval_interval == 0:
            model.eval()
            val_losses = []
            with torch.no_grad():
                for _ in range(args.eval_iters):
                    x_val, y_val = get_batch(val_data, args.batch_size, args.context_length, str(device))
                    val_logits = model(x_val)
                    val_loss = calc_cross_entropy(val_logits, y_val)
                    val_losses.append(val_loss.item())
            avg_val_loss = sum(val_losses) / len(val_losses)
            logging.info("iter=%d validation_loss=%.4f", it, avg_val_loss)
            model.train()

        if it > 0 and it % args.checkpoint_interval == 0:
            ckpt_path = out_dir / f"ckpt_iter{it}.pt"
            save_checkpoint(model, adamw_opt, it, ckpt_path, muon_optimizer=muon_opt)
            logging.info("saved checkpoint: %s", ckpt_path)

    final_path = out_dir / "ckpt_final.pt"
    save_checkpoint(model, adamw_opt, args.max_iters, final_path, muon_optimizer=muon_opt)
    logging.info("saved final checkpoint: %s", final_path)


if __name__ == "__main__":
    main()
