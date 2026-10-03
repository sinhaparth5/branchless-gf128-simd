#pragma once

#include <chrono>
#include <cstdint>

#include "gf128/gf128.hpp"

#if defined(__x86_64__) || (defined(_M_X64) && !defined(_M_ARM64EC))
#define GF128_BENCH_TSC 1
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <emmintrin.h>
#include <x86intrin.h>
#endif
#endif

namespace bench {

// Timestamp for short intervals. On x86-64 it is the TSC, which counts
// reference cycles at a fixed rate, not core cycles. Elsewhere it is
// steady_clock nanoseconds, which is coarse on some systems (about 42 ns on
// Apple Silicon), so callers time batches of operations.
inline std::uint64_t stamp() {
#if GF128_BENCH_TSC
  _mm_lfence();
  std::uint64_t t = __rdtsc();
  _mm_lfence();
  return t;
#else
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        std::chrono::steady_clock::now().time_since_epoch())
                                        .count());
#endif
}

inline const char* stamp_unit() {
#if GF128_BENCH_TSC
  return "TSC ticks";
#else
  return "ns";
#endif
}

inline double now_ns() {
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// xorshift64. Benchmarks do not need cryptographic randomness.
struct Rng {
  std::uint64_t s;
  std::uint64_t next() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
  }
  gf128::u128 next128() {
    std::uint64_t lo = next();
    return {lo, next()};
  }
};

inline std::uint64_t fold(gf128::u128 r) { return r.lo ^ r.hi; }

}  // namespace bench
