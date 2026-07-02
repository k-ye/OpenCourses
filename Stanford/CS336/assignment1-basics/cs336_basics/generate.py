import argparse
from pathlib import Path
import torch
from cs336_basics.gpt import TransformerLM, softmax
from cs336_basics.bpe import Tokenizer
from cs336_basics.cli_utils import add_device_dtype_args, pick_device, pick_dtype


def parse_args():
    parser = argparse.ArgumentParser("Generate a sentence based on a given LM and BPE")

    parser.add_argument("--ckpt", type=Path, required=True)
    parser.add_argument("--vocab", type=Path, required=True)
    parser.add_argument("--merges", type=Path, required=True)
    parser.add_argument("-p", "--prompt", type=str, required=True)

    parser.add_argument("--max-new-tokens", type=int, default=256)
    parser.add_argument("--temperature", type=float, default=0.8)
    parser.add_argument("--top-p", type=float, default=0.9)

    add_device_dtype_args(parser)

    return parser.parse_args()


def main():
    args = parse_args()

    if args.temperature <= 0:
        raise ValueError("temperature must be positive")
    if not (0 < args.top_p <= 1):
        raise ValueError("top_p must be in (0, 1]")

    device = pick_device(args.device)
    dtype = pick_dtype(device, args.dtype)

    EOT = "<|endoftext|>"
    special_token = [EOT]
    tokenizer = Tokenizer.from_files(args.vocab, args.merges, special_token)
    model = TransformerLM.from_dir(args.ckpt, device, dtype)
    model.eval()
    context_length = model.context_length

    EOT_TOKEN_ID = tokenizer.encode(EOT)
    assert len(EOT_TOKEN_ID) == 1
    EOT_TOKEN_ID = EOT_TOKEN_ID[0]
    generated = torch.tensor([tokenizer.encode(args.prompt)], device=device)

    printed_text = tokenizer.decode(generated[0].tolist())
    print(printed_text, end="", flush=True)

    with torch.inference_mode():
        for _ in range(args.max_new_tokens):
            model_input = generated[:, -context_length:]
            logits = model.forward(model_input)[0, -1, :]
            probs = softmax(logits / args.temperature, -1)
            # top-p filter
            sorted_probs, sorted_indices = torch.sort(probs, descending=True)
            cum_probs = torch.cumsum(sorted_probs, dim=-1)

            cutoff_idx = torch.searchsorted(cum_probs, args.top_p)
            keep_count = cutoff_idx.item() + 1

            kept_probs = sorted_probs[:keep_count]
            kept_indices = sorted_indices[:keep_count]
            kept_probs = kept_probs / kept_probs.sum()
            sampled_local_idx = torch.multinomial(kept_probs, num_samples=1)
            next_token_id = kept_indices[sampled_local_idx]

            if next_token_id.item() == EOT_TOKEN_ID:
                break

            generated = torch.cat([generated, next_token_id.view(1, 1)], dim=1)

            current_text = tokenizer.decode(generated[0].tolist())
            new_text = current_text[len(printed_text) :]
            print(new_text, end="", flush=True)
            printed_text = current_text


if __name__ == "__main__":
    main()
