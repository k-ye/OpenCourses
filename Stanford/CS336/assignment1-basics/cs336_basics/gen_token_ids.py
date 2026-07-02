import numpy as np

import argparse
from pathlib import Path
import pickle
from cs336_basics.bpe import Tokenizer

LOG_INTERVAL = 1_000_000


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--train-data", type=Path, required=True)
    parser.add_argument("--val-data", type=Path, required=True)
    parser.add_argument("--bpe-vocab", type=Path, required=True)
    parser.add_argument("--bpe-merges", type=Path, required=True)

    return parser.parse_args()


def get_token_ids_path(data_path: Path) -> Path:
    base_name = data_path.name.split(".", 1)[0]
    res = data_path.with_name(f"{base_name}_token_ids.npy")
    return res


def encode_to_npy(tokenizer: Tokenizer, data_path: Path) -> Path:
    def count_token_ids() -> int:
        count = 0
        with data_path.open("r", encoding="utf-8") as f:
            for _ in tokenizer.encode_iterable(f):
                count += 1
                if count > 0 and count % LOG_INTERVAL == 0:
                    print(f"count={count}")
        return count

    out_path = get_token_ids_path(data_path)
    num_tokens = count_token_ids()

    arr = np.lib.format.open_memmap(
        out_path,
        mode="w+",
        dtype=np.uint16,
        shape=(num_tokens,),
    )

    i = 0
    with data_path.open("r", encoding="utf-8") as f:
        for token_id in tokenizer.encode_iterable(f):
            arr[i] = token_id
            i += 1
            if i > 0 and i % LOG_INTERVAL == 0:
                print(f"tokenized at i={i}")
    arr.flush()
    return out_path


def main():
    args = parse_args()

    with open(args.bpe_vocab, "rb") as f:
        vocab = pickle.load(f)
    with open(args.bpe_merges, "rb") as f:
        merges = pickle.load(f)

    tokenizer = Tokenizer(vocab, merges, special_tokens=["<|endoftext|>"])

    encode_to_npy(tokenizer, args.train_data)
    encode_to_npy(tokenizer, args.val_data)


if __name__ == "__main__":
    main()
