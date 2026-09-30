#include "check.hpp"

using gf128::u128;
using gf128::mul_ref;

int main() {
  const u128 one{1, 0}, x{2, 0}, x127{0, 1ull << 63};

  CHECK(eq(mul_ref(x127, x), u128{0x87, 0}));  // x^128 = x^7 + x^2 + x + 1
  CHECK(eq(mul_ref(x127, x127), mul_ref(mul_ref(x127, x), u128{0, 1ull << 62})));

  for (int n = 0; n < 1000; ++n) {
    u128 a = rnd128(), b = rnd128(), c = rnd128();
    CHECK(eq(mul_ref(a, one), a));
    CHECK(eq(mul_ref(a, b), mul_ref(b, a)));
    CHECK(eq(mul_ref(a, add(b, c)), add(mul_ref(a, b), mul_ref(a, c))));
    CHECK(eq(mul_ref(mul_ref(a, b), c), mul_ref(a, mul_ref(b, c))));
  }
  return report();
}
