#ifndef CS207_UTIL_HPP
#define CS207_UTIL_HPP

/**
 * @file Util.hpp
 * Common helper code for CS207
 *
 * @brief Some common utilities for use in CS207
 */

#include <iostream>
#include <sstream>
#include <chrono>
#include <unistd.h>
#include <cassert>
#include <cstdlib>
#include <cmath>

#include <random>
#include <array>

namespace CS207 {

static std::mt19937 default_generator;

/** Return a uniform random double in [a,b] */
double random(double a, double b) {
  std::uniform_real_distribution<double> dist(a, b);
  return dist(default_generator);
}

/** Return a uniform random double in [0,1] */
double random() {
  return random(0,1);
}

/** @brief Clock class useful for timing code segments.
 *
 * @code
 * CS207::Clock clock;
 * // code to time
 * double time = clock.seconds();
 * @endcode
 */
class Clock {
 public:
  typedef std::chrono::high_resolution_clock clock;
  typedef typename clock::time_point         time_point;
  typedef typename clock::duration           duration;
  typedef typename duration::rep             tick_type;

  Clock() : starttime_(clock::now()) {}
  /** Start this clock */
  void start() {
    starttime_ = clock::now();
  }
  /** Get the duration this clock has been running */
  duration elapsed() const {
    return clock::now() - starttime_;
  }
  /** Get the number of seconds this clock has been running */
  double seconds() const {
    typedef std::chrono::duration<double> units;
    return std::chrono::duration_cast<units>(elapsed()).count();
  }
 private:
  time_point starttime_;
};

/** Forces a thread to sleep for t seconds. Accurate to microseconds. */
int sleep(double t) {
  return usleep(t*1e6);
}

/** Read a line from @a s, parse it as @a N values of type @a T
 * @param[in,out]  s  input stream to read from
 * @param[out]     v  std::array returned if the line in @a s parses
 */
template <typename T, std::size_t N>
std::istream& operator>>(std::istream& s, std::array<T,N>& v) {
  for (auto&& a : v) s >> a;
  return s;
}

/** Read a line from @a s, parse it as type T, and store it in @a value.
 * Ignores blank lines and lines that start with '#'.
 *
 * @param[in]   s      input stream
 * @param[out]  value  value returned if the line in @a s parses
 *
 * If the line doesn't parse correctly, then @a s is set to the "failed" state.
 */
template <typename T>
std::istream& getline_parsed(std::istream& s, T& value)
{
  // Get a line from the file
  std::string str;
  do {
    getline(s, str);
  } while (s && (str.empty() || str[0] == '#'));
  std::istringstream is(str);
  is >> value;
  if (is.fail())
    s.setstate(std::istream::failbit);
  return s;
}

} // end namespace CS207



/** Derive operator>, operator<=, and operator>= from a class's operator<.
 *
 * Usage:
 * class MyClass : private less_than_comparable<MyClass> {
 *   bool operator<(const MyClass& c) { ... }
 * };
 */
template <typename T>
struct less_than_comparable {
  friend bool operator> (const T& a, const T& b) { return   b < a;  }
  friend bool operator<=(const T& a, const T& b) { return !(b < a); }
  friend bool operator>=(const T& a, const T& b) { return !(a < b); }
};

/** Derive operator!= from a class's operator==.
 *
 * Usage:
 * class MyClass : private equality_comparable<MyClass> {
 *   bool operator==(const MyClass& c) { ... }
 * };
 */
template <typename T>
struct equality_comparable {
  friend bool operator!=(const T& a, const T& b) { return !(a == b); }
};

/** Derive operator!=, operator>, operator<=, and operator>=
 * from a class's operator< and operator==.
 *
 * Usage:
 * class MyClass : private equality_comparable<MyClass> {
 *   bool operator< (const MyClass& c) { ... }
 *   bool operator==(const MyClass& c) { ... }
 * };
 */
template <typename T>
struct totally_ordered
    : less_than_comparable<T>, equality_comparable<T> {
};

#endif
