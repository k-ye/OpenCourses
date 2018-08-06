import random

NUM_COLUMNS = 3
NUM_ROWS = 500
GEN_RANDOM = False
FILE_NAME = 'test_data.txt'

def gen_random_nums():
  return [random.randint(-10000, 10000) for _ in xrange(NUM_COLUMNS)]

def to_row(nums):
  return ','.join([str(x) for x in nums])

if __name__ == '__main__':
  r = 0
  with open(FILE_NAME, 'w') as f:
    for rno in xrange(NUM_ROWS):
      nums = None 
      if GEN_RANDOM:
        nums = gen_random_nums()
      else:
        base = rno * NUM_COLUMNS
        nums = [(base + i) for i in xrange(NUM_COLUMNS)]
      f.write(to_row(nums) + '\n') 
