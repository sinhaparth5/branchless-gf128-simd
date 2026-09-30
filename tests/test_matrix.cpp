#include "check.hpp"

using namespace gf128;

int main() {
  const Mred m = make_mred();
  const u128 x{2, 0};

  // Column 0 is x^128 mod P, and each next column is the previous one times x.
  CHECK(eq(m[0], u128{0x87, 0}));
  for (int j = 0; j + 1 < 127; ++j) CHECK(eq(m[j + 1], mul_ref(m[j], x)));

  for (int n = 0; n < 10000; ++n) {
    // Any 255-bit value (bit 255 never appears in a 128x128 product).
    u256 c{rnd128(), rnd128()};
    c.hi.hi &= ~(1ull << 63);
    CHECK(eq(reduce_matrix(m, c), reduce_ref(c)));

    // Real products.
    u128 a = rnd128(), b = rnd128();
    CHECK(eq(reduce_matrix(m, clmul_ref(a, b)), mul_ref(a, b)));
  }
  return report();
}
