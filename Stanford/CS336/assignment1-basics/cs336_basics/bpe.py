import regex as re
import os
from typing import BinaryIO
import multiprocessing as mp
from concurrent.futures import Future, ProcessPoolExecutor, as_completed
from collections import defaultdict

type BytePair = tuple[bytes, bytes]
type TokenCountMap = defaultdict[tuple[bytes, ...], int]


def find_chunk_boundaries(
    file: BinaryIO,
    desired_num_chunks: int,
    split_special_token: bytes,
) -> list[int]:
    """
    Chunk the file into parts that can be counted independently.
    May return fewer chunks if the boundaries end up overlapping.
    """
    assert isinstance(split_special_token, bytes), "Must represent special token as a bytestring"

    # Get total file size in bytes
    file.seek(0, os.SEEK_END)
    file_size = file.tell()
    file.seek(0)

    chunk_size = file_size // desired_num_chunks

    # Initial guesses for chunk boundary locations, uniformly spaced
    # Chunks start on previous index, don't include last index
    chunk_boundaries = [i * chunk_size for i in range(desired_num_chunks + 1)]
    chunk_boundaries[-1] = file_size

    mini_chunk_size = 4096  # Read ahead by 4k bytes at a time

    for bi in range(1, len(chunk_boundaries) - 1):
        initial_position = chunk_boundaries[bi]
        file.seek(initial_position)  # Start at boundary guess
        while True:
            mini_chunk = file.read(mini_chunk_size)  # Read a mini chunk

            # If EOF, this boundary should be at the end of the file
            if mini_chunk == b"":
                chunk_boundaries[bi] = file_size
                break

            # Find the special token in the mini chunk
            found_at = mini_chunk.find(split_special_token)
            if found_at != -1:
                chunk_boundaries[bi] = initial_position + found_at
                break
            initial_position += mini_chunk_size

    # Make sure all boundaries are unique, but might be fewer than desired_num_chunks
    return sorted(set(chunk_boundaries))


def pretokenize(input_path: str, begin: int, end: int, special_tokens: list[str]) -> TokenCountMap:
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


def replace_token(token: tuple[bytes, ...], pair: BytePair) -> tuple[bytes, ...]:
    res = []
    merged = pair[0] + pair[1]
    i, n = 0, len(token)
    while i < n:
        if i + 1 < n and token[i] == pair[0] and token[i + 1] == pair[1]:
            res.append(merged)
            i += 2
        else:
            res.append(token[i])
            i += 1
    return tuple(res)


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
            f = executor.submit(pretokenize, input_path, begin, end, special_tokens)
            futs.append(f)
        for f in as_completed(futs):
            counts = f.result()
            for k, v in counts.items():
                token_counts[k] += v
    # merging
    merge_vocab_size = vocab_size - len(special_tokens)
    vocabulary: dict[int, bytes] = {i: bytes([i]) for i in range(256)}
    merges: list[BytePair] = []
    while len(vocabulary) < merge_vocab_size:
        pair_counts: defaultdict[BytePair, int] = defaultdict(int)
        bp_to_tokens = defaultdict(set)
        for tok_tup, cnt in token_counts.items():
            for i, bp in enumerate(zip(tok_tup[:-1], tok_tup[1:])):
                pair_counts[bp] += cnt
                bp_to_tokens[bp].add(tok_tup)

        max_count = -1
        max_pairs = set()
        for pr, cnt in pair_counts.items():
            if cnt > max_count:
                max_count = cnt
                max_pairs.clear()
                max_pairs.add(pr)
            elif cnt == max_count:
                max_pairs.add(pr)

        if not max_pairs:
            break
        max_pairs: list[BytePair] = sorted(max_pairs)
        pair_to_merge = max_pairs[-1]
        merges.append(pair_to_merge)
        merged_pair = pair_to_merge[0] + pair_to_merge[1]
        vocab_id = len(vocabulary)
        vocabulary[vocab_id] = merged_pair

        total_count_before = sum(token_counts.values())
        new_token_counts: TokenCountMap = defaultdict(int)
        for tok, cnt in token_counts.items():
            if tok in bp_to_tokens[pair_to_merge]:
                new_tok = replace_token(tok, pair_to_merge)
                new_token_counts[new_tok] += cnt
            else:
                new_token_counts[tok] += cnt
        total_count_after = sum(new_token_counts.values())
        assert total_count_before == total_count_after
        token_counts = new_token_counts
    return vocabulary, merges