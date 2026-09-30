#include "check.hpp"

using namespace gf128;

int main() {
  const Mred m = make_mred();
  const Tiles t = make_tiles(m);

  // Tile encoding round-trips through the instruction model: tile (0,0) applied
  // to a single set bit t must give the low byte of column t of M_red.
  for (int b = 0; b < 8; ++b)
    CHECK(affine_byte(t.a0, static_cast<std::uint8_t>(1u << b)) == (m[b].lo & 0xFF));

  // The two-diagonal structure the kernel relies on.
  for (int i = 0; i < 16; ++i)
    for (int k = 0; k < 15; ++k)
      CHECK(tile(m, i, k) == (i == k ? t.a0 : i == k + 1 ? t.a1 : 0));

  for (int n = 0; n < 10000; ++n) {
    u256 c{rnd128(), rnd128()};
    c.hi.hi &= ~(1ull << 63);
    CHECK(eq(reduce_affine(t, c), reduce_ref(c)));
  }
  return report();
}
