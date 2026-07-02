import argparse
import torch
import random
import numpy as np


def add_device_dtype_args(parser: argparse.ArgumentParser):
    parser.add_argument("--device", type=str, default="cuda", choices=["cpu", "cuda", "mps"])
    parser.add_argument("--dtype", type=str, default="bfloat16", choices=["float32", "bfloat16", "float16"])


def pick_device(dev_str: str) -> torch.device:
    if dev_str == "cuda":
        if not torch.cuda.is_available():
            dev_str = "cpu"
    dev = torch.device(dev_str)
    return dev


def pick_dtype(device: torch.device, dtype_str: str) -> torch.dtype:
    DTYPES = {
        "float32": torch.float32,
        "bfloat16": torch.bfloat16,
        "float16": torch.float16,
    }
    if device.type == "cpu":
        dtype_str = "float32"
    return DTYPES[dtype_str]


def set_seed(seed: int):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed(seed)
        torch.cuda.manual_seed_all(seed)
