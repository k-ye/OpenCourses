
def gen_helper(arr, sz, start, cur, result):
  if len(cur) == sz:
    t = tuple(cur)
    result.add(t)
    return
  for i in xrange(start, len(arr)):
    cur.append(arr[i])
    gen_helper(arr, sz, i + 1, cur, result)
    cur.pop()
    gen_helper(arr, sz, i + 1, cur, result)


def gen_subset(arr, sz):
  arr = list(arr)
  result = set()
  cur = []
  gen_helper(arr, sz, 0, cur, result)
  return result

if __name__ == '__main__':
  N = 9
  sz = 5
  arr = set(range(N))
  subsets = gen_subset(arr, sz)
  for s in subsets:
    print s
  print '|subsets| = %d' % len(subsets)
