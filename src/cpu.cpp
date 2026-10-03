#include "gf128/gf128.hpp"

#if defined(__x86_64__) || (defined(_M_X64) && !defined(_M_ARM64EC))
#define GF128_X86_64 1
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#endif

namespace gf128 {

#if GF128_X86_64
namespace {

struct Regs { unsigned a, b, c, d; };

Regs cpuid(unsigned leaf, unsigned sub) {
#if defined(_MSC_VER) && !defined(__clang__)
  int r[4];
  __cpuidex(r, static_cast<int>(leaf), static_cast<int>(sub));
  return {static_cast<unsigned>(r[0]), static_cast<unsigned>(r[1]),
          static_cast<unsigned>(r[2]), static_cast<unsigned>(r[3])};
#else
  Regs r{};
  if (!__get_cpuid_count(leaf, sub, &r.a, &r.b, &r.c, &r.d)) return {};
  return r;
#endif
}

// XCR0: which register states the OS saves on context switch.
unsigned long long xgetbv0() {
#if defined(_MSC_VER) && !defined(__clang__)
  return _xgetbv(0);
#else
  unsigned lo, hi;
  __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
  return (static_cast<unsigned long long>(hi) << 32) | lo;
#endif
}

bool bit(unsigned r, int n) { return ((r >> n) & 1) != 0; }

}  // namespace

bool cpu_has_avx512_gfni() {
  if (cpuid(0, 0).a < 7) return false;
  const Regs l1 = cpuid(1, 0), l7 = cpuid(7, 0);
  if (!bit(l7.b, 16) || !bit(l7.b, 30) ||  // AVX512F, AVX512BW
      !bit(l7.c, 8) || !bit(l7.c, 10))     // GFNI, VPCLMULQDQ
    return false;
  if (!bit(l1.c, 27)) return false;  // OSXSAVE, needed before XGETBV
#if defined(__APPLE__)
  // macOS turns on the AVX-512 register state lazily, so XCR0 lacks it until
  // the first AVX-512 instruction. The kernel reports support through sysctl.
  int on = 0;
  size_t len = sizeof on;
  if (sysctlbyname("hw.optional.avx512f", &on, &len, nullptr, 0) != 0 || on == 0) return false;
  return (xgetbv0() & 0x6) == 0x6;  // SSE and AVX state
#else
  return (xgetbv0() & 0xE6) == 0xE6;  // SSE, AVX, opmask, ZMM0-15 high, ZMM16-31
#endif
}
#else
bool cpu_has_avx512_gfni() { return false; }
#endif

}  // namespace gf128
