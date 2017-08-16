#include "util.h"

#include <sys/time.h>
#include <time.h>

#define MIN_BODY return ((a < b) ? a : b)
#define MAX_BODY return ((a > b) ? a : b)

int imin(int a, int b) { MIN_BODY; }

int imax(int a, int b) { MAX_BODY; }

unsigned umin(unsigned a, unsigned b) { MIN_BODY; }

unsigned umax(unsigned a, unsigned b) { MAX_BODY; }

long lmin(long a, long b) { MIN_BODY; }

long lmax(long a, long b) { MAX_BODY; }

#undef MIN_BODY
#undef MAX_BODY

long get_now_micros() {
  struct timeval now;
  gettimeofday(&now, NULL);
  long result = now.tv_sec * 1000000 + now.tv_usec;
  return result;
}

long get_now_nanos() { return get_now_micros() * 1000; }