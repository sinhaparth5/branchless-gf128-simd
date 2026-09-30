#pragma once

#include <cstdint>
#include <cstdio>

#include "gf128/gf128.hpp"

inline int fails = 0;
#define CHECK(c)                                                  \
  do {                                                            \
    if (!(c)) {                                                   \
      std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); \
      ++fails;                                                    \
    }                                                             \
  } while (0)

inline int report() {
  if (fails == 0) std::puts("ok");
  return fails == 0 ? 0 : 1;
}

inline bool eq(gf128::u128 a, gf128::u128 b) { return a.lo == b.lo && a.hi == b.hi; }
inline gf128::u128 add(gf128::u128 a, gf128::u128 b) { return {a.lo ^ b.lo, a.hi ^ b.hi}; }

// xorshift64, fixed seed so failures reproduce.
inline std::uint64_t rnd64() {
  static std::uint64_t s = 0x9E3779B97F4A7C15ull;
  s ^= s << 13; s ^= s >> 7; s ^= s << 17;
  return s;
}
inline gf128::u128 rnd128() {
  std::uint64_t lo = rnd64();
  return {lo, rnd64()};
}
