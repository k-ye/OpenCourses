#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

int imin(int a, int b);

int imax(int a, int b);

unsigned umin(unsigned a, unsigned b);

unsigned umax(unsigned a, unsigned b);

long lmin(long a, long b);

long lmax(long a, long b);

long get_now_micros();

long get_now_nanos();

#endif  // TIME_UTIL_H