#include <cstdio>

#include "gf128/gf128.hpp"

using gf128::u128;
using gf128::mul_ref;

static int fails = 0;
#define CHECK(c)                                                  \
  do {                                                            \
    if (!(c)) {                                                   \
      std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); \
      ++fails;                                                    \
    }                                                             \
  } while (0)

static bool eq(u128 a, u128 b) { return a.lo == b.lo && a.hi == b.hi; }
static u128 add(u128 a, u128 b) { return {a.lo ^ b.lo, a.hi ^ b.hi}; }

static u128 rnd() {
  static std::uint64_t s = 0x9E3779B97F4A7C15ull;
  auto next = [] { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; };
  std::uint64_t lo = next();
  return {lo, next()};
}

int main() {
  const u128 one{1, 0}, x{2, 0}, x127{0, 1ull << 63};

  CHECK(eq(mul_ref(x127, x), u128{0x87, 0}));  // x^128 = x^7 + x^2 + x + 1
  CHECK(eq(mul_ref(x127, x127), mul_ref(mul_ref(x127, x), u128{0, 1ull << 62})));

  for (int n = 0; n < 1000; ++n) {
    u128 a = rnd(), b = rnd(), c = rnd();
    CHECK(eq(mul_ref(a, one), a));
    CHECK(eq(mul_ref(a, b), mul_ref(b, a)));
    CHECK(eq(mul_ref(a, add(b, c)), add(mul_ref(a, b), mul_ref(a, c))));
    CHECK(eq(mul_ref(mul_ref(a, b), c), mul_ref(a, mul_ref(b, c))));
  }

  if (fails == 0) std::puts("ok");
  return fails == 0 ? 0 : 1;
}
