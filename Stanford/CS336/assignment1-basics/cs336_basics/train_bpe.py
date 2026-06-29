from cs336_basics.bpe import train_bpe_tokenizer
import argparse
from pathlib import Path
import pickle

def parse_args():
    parser = argparse.ArgumentParser("Train BPE")
    parser.add_argument("-i", "--input", type=Path, required=True, help="Input path to the corpus")
    parser.add_argument("--vocab-size", type=int, required=True)
    return parser.parse_args()

def main():
    args = parse_args()
    input_path: Path = args.input
    if not input_path.is_file():
        raise FileNotFoundError(f"Invalid input: {input_path}")
    vocab, merges = train_bpe_tokenizer(input_path, args.vocab_size, ["<|endoftext|>"])

    base_name = input_path.name.split(".", 1)[0]
    vocab_path = input_path.with_name(f"{base_name}_vocab.pickle")
    with open(vocab_path, "wb") as f:
        pickle.dump(vocab, f)
    merges_path = input_path.with_name(f"{base_name}_merges.pickle")
    with open(merges_path, "wb") as f:
        pickle.dump(merges, f)


if __name__ == '__main__':
    main()
