import regex as re
import multiprocessing as mp
from concurrent.futures import Future, ProcessPoolExecutor, as_completed
from collections import defaultdict

from .pretokenization_example import find_chunk_boundaries

TokenCountMap = defaultdict[tuple[bytes, ...], int]


def _pretokenize(input_path: str, begin: int, end: int, special_tokens: list[str]) -> TokenCountMap:
    special_tokens = sorted(special_tokens, key=lambda t: -len(t))
    re_special_tokens = "|".join([re.escape(t) for t in special_tokens])
    with open(input_path, "rb") as f:
        f.seek(begin)
        chunk = f.read(end - begin).decode("utf-8", errors="ignore")
        chunk_splits = re.split(re_special_tokens, chunk)
    PAT = r"""'(?:[sdmt]|ll|ve|re)| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+"""
    counts: TokenCountMap = defaultdict(int)
    for s in chunk_splits:
        pretokens: list[str] = re.findall(PAT, s)
        for tok in pretokens:
            key = tuple(bytes([b]) for b in tok.encode("utf-8"))
            counts[key] += 1
    return counts


def train_bpe_tokenizer(
    input_path: str,
    vocab_size: int,
    special_tokens: list[str] = None,
) -> tuple[dict[int, bytes], list[tuple[bytes, bytes]]]:
    if not special_tokens:
        raise RuntimeError("Expected non-empty special_tokens")
    num_workers = mp.cpu_count()
    desired_num_chunks = num_workers * 2
    with open(input_path, "rb") as f:
        boundaries = find_chunk_boundaries(f, desired_num_chunks, special_tokens[0].encode("utf-8"))

    token_counts: TokenCountMap = defaultdict(int)
    with ProcessPoolExecutor(max_workers=num_workers) as executor:
        futs: list[Future[TokenCountMap]] = []
        for begin, end in zip(boundaries[:-1], boundaries[1:]):
            f = executor.submit(_pretokenize, input_path, begin, end, special_tokens)
            futs.append(f)
        for f in as_completed(futs):
            counts = f.result()
            for k, v in counts.items():
                token_counts[k] += v
    # merging
    merge_vocab_size = vocab_size - len(special_tokens)
    vocabulary: dict[int, bytes] = {i: bytes([i]) for i in range(256)}
    merges: list[tuple[bytes, bytes]] = []
    while len(vocabulary) < merge_vocab_size:
        pair_counts: TokenCountMap = defaultdict(int)
        bp_to_tokens = defaultdict(set)
        for tok_tup in token_counts.keys():
            for a, b in zip(tok_tup[:-1], tok_tup[1:]):
                bp = (a, b)
                pair_counts[bp] += 1
                bp_to_tokens[bp].append(tok_tup)
        max_count = -1
        max_pairs = set()
        for pr, cnt in pair_counts.items():
            if cnt > max_count:
                max_count = cnt
                max_pairs.clear()
                max_pairs.add(pr)
            elif cnt == max_count:
                max_pairs.add(pr)
        max_pairs = sorted(max_pairs)
        merged_pair: tuple[bytes, bytes] = max_pairs[-1]
        merges.append(merged_pair)



