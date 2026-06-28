
import multiprocessing as mp
from concurrent.futures import ProcessPoolExecutor

from .pretokenization_example import find_chunk_boundaries

def _pretokenize(
      input_path: str,
      begin: int,
      end: int,
      special_tokens: list[str]
):
   special_tokens.sort(key=lambda t: -len(t))
   
   pass

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
      boundaries = find_chunk_boundaries(f, desired_num_chunks, special_tokens[0])
    
    with ProcessPoolExecutor(max_workers=num_workers) as executor:
       futs = []
       for i in range(len(boundaries)-1):
          f = executor.submit(_pretokenize, input_path, boundaries[i], boundaries[i+1], special_tokens)
          futs.append(f)

    