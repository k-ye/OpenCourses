import regex as re
import os
from typing import BinaryIO
from re import Pattern
from collections.abc import Iterable, Iterator
from collections.abc import Generator
import multiprocessing as mp
from concurrent.futures import Future, ProcessPoolExecutor, as_completed
from collections import defaultdict
from tqdm import tqdm

type BytePair = tuple[bytes, bytes]
type TokenSequence = tuple[bytes, ...]
type TokenCountMap = defaultdict[TokenSequence, int]

PRETOKENIZATION_PAT = r"""'(?:[sdmt]|ll|ve|re)| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+"""
PRETOKENIZATION_PAT_RE = re.compile(PRETOKENIZATION_PAT)


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


def pretokenize_worker(input_path: str | os.PathLike, begin: int, end: int, special_tokens: list[str]) -> TokenCountMap:
    special_tokens = sorted(special_tokens, key=lambda t: -len(t))
    re_special_tokens = "|".join([re.escape(t) for t in special_tokens])
    with open(input_path, "rb") as f:
        f.seek(begin)
        chunk = f.read(end - begin).decode("utf-8", errors="ignore")
        chunk_splits = re.split(re_special_tokens, chunk)
    counts: TokenCountMap = defaultdict(int)
    for s in chunk_splits:
        for match in re.finditer(PRETOKENIZATION_PAT_RE, s):
            key = tuple(bytes([b]) for b in match.group().encode("utf-8"))
            counts[key] += 1
    return counts


def replace_token(token: TokenSequence, pair: BytePair) -> TokenSequence:
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


def get_byte_pairs(token: TokenSequence) -> Generator[BytePair]:
    yield from zip(token[:-1], token[1:])


def train_bpe_tokenizer(
    input_path: str | os.PathLike,
    vocab_size: int,
    special_tokens: list[str] = None,
    *,
    show_progress: bool = False,
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
            f = executor.submit(pretokenize_worker, input_path, begin, end, special_tokens)
            futs.append(f)
        for f in as_completed(futs):
            counts = f.result()
            for k, v in counts.items():
                token_counts[k] += v
    # merging
    vocabulary: dict[int, bytes] = {i: bytes([i]) for i in range(256)}
    merges: list[BytePair] = []

    def add_to_vocab(p: bytes):
        vocab_id = len(vocabulary)
        vocabulary[vocab_id] = p

    # TODO(k-ye): rename to bp_counts
    pair_counts: defaultdict[BytePair, int] = defaultdict(int)
    bp_to_tokens: defaultdict[BytePair, set[TokenSequence]] = defaultdict(set)
    for token, cnt in token_counts.items():
        for bp in get_byte_pairs(token):
            pair_counts[bp] += cnt
            bp_to_tokens[bp].add(token)

    num_merges = vocab_size - len(special_tokens) - len(vocabulary)
    merge_steps = range(num_merges)
    if show_progress:
        merge_steps = tqdm(merge_steps, desc="Merging BPE pairs")
    for _ in merge_steps:
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
        # pick the pair to merge
        max_pairs: list[BytePair] = sorted(max_pairs)
        pair_to_merge = max_pairs[-1]
        merges.append(pair_to_merge)
        merged_pair = pair_to_merge[0] + pair_to_merge[1]
        add_to_vocab(merged_pair)

        total_count_before = sum(token_counts.values())
        tokens_to_update = tuple(bp_to_tokens[pair_to_merge])  # freeze the set
        for token in tokens_to_update:
            old_cnt = token_counts[token]
            new_token = replace_token(token, pair_to_merge)
            # remove the old byte pairs
            bp_dedup: set[BytePair] = set()
            for bp in get_byte_pairs(token):
                pair_counts[bp] -= old_cnt
                bp_dedup.add(bp)
                assert pair_counts[bp] >= 0
                if pair_counts[bp] == 0:
                    del pair_counts[bp]
            for bp in bp_dedup:
                bp_to_tokens[bp].remove(token)
                if len(bp_to_tokens[bp]) == 0:
                    del bp_to_tokens[bp]
            # add the new byte pairs
            for bp in get_byte_pairs(new_token):
                pair_counts[bp] += old_cnt
                bp_to_tokens[bp].add(new_token)

            token_counts[new_token] += old_cnt
            token_counts[token] -= old_cnt
            if token_counts[token] == 0:
                del token_counts[token]

        total_count_after = sum(token_counts.values())
        assert total_count_before == total_count_after
    for tok in special_tokens:
        add_to_vocab(tok.encode("utf-8"))
    return vocabulary, merges


class Tokenizer:
    def __init__(self, vocab: dict[int, bytes], merges: list[BytePair], special_tokens: list[str] | None = None):
        self.vocab = vocab
        self.merges = merges
        self.merge_ranks = {bp: i for i, bp in enumerate(self.merges)}
        self.special_tokens: set[str] = set()
        self.re_special_tokens: Pattern[str] | None = None
        if special_tokens:
            special_tokens = sorted(special_tokens, key=len, reverse=True)
            self.special_tokens = set(special_tokens)
            # using a capture group "(abc|def|gh)", so that special tokens are preserved in
            # the result of re.split()
            self.re_special_tokens = re.compile("(" + "|".join([re.escape(t) for t in special_tokens]) + ")")
            vocab_vals = set(vocab.values())
            for st in self.special_tokens:
                st_b = st.encode("utf-8")
                if st_b not in vocab_vals:
                    self.vocab[len(self.vocab)] = st_b
                    vocab_vals.add(st_b)
        self.vocab_reverse = {v: k for k, v in vocab.items()}

    @classmethod
    def from_cls(
        cls,
        vocab_filepath: str | os.PathLike,
        merges_filepath: str | os.PathLike,
        special_tokens: list[str] | None = None,
    ):
        # TODO: Implement when we need it
        raise NotImplementedError

    def encode(self, text: str) -> list[int]:
        if self.re_special_tokens is None:
            text_splits = [text]
        else:
            text_splits = re.split(self.re_special_tokens, text)
        res: list[int] = []
        for word in text_splits:
            if word in self.special_tokens:
                res.append(self.vocab_reverse[word.encode("utf-8")])
                continue
            for match in re.finditer(PRETOKENIZATION_PAT_RE, word):
                token = tuple(bytes([b]) for b in match.group().encode("utf-8"))
                merged = self._merge(token)
                for t in merged:
                    res.append(self.vocab_reverse[t])
        return res

    def encode_iterable(self, iterable: Iterable[str]) -> Iterator[int]:
        # TODO: cross-boundary words
        for s in iterable:
            yield from self.encode(s)

    def decode(self, ids: list[int]) -> str:
        byte_seq: list[bytes] = []
        for i in ids:
            byte_seq.append(self.vocab[i])
        text_bytes = b"".join(byte_seq)
        return text_bytes.decode("utf-8", errors="replace")

    def _merge(self, token: TokenSequence) -> TokenSequence:
        while True:
            best_rank = -1
            merge_idx = -1
            pair_to_merge = None
            for i, tbp in enumerate(get_byte_pairs(token)):
                if tbp in self.merge_ranks:
                    rank = self.merge_ranks[tbp]
                    if best_rank == -1 or rank < best_rank:
                        best_rank = rank
                        merge_idx = i
                        pair_to_merge = tbp
            if merge_idx == -1:
                break
            assert pair_to_merge is not None
            token = token[:merge_idx] + (pair_to_merge[0] + pair_to_merge[1],) + token[(merge_idx + 2) :]
        return token
