#include "gf128/gf128.hpp"

namespace gf128 {

u256 clmul_ref(u128 a, u128 b) {
  std::uint64_t r[4] = {}, bb[4] = {b.lo, b.hi, 0, 0};
  for (int i = 0; i < 128; ++i) {
    if (((i < 64 ? a.lo >> i : a.hi >> (i - 64)) & 1) != 0)
      for (int k = 0; k < 4; ++k) r[k] ^= bb[k];
    for (int k = 3; k > 0; --k) bb[k] = (bb[k] << 1) | (bb[k - 1] >> 63);
    bb[0] <<= 1;
  }
  return {{r[0], r[1]}, {r[2], r[3]}};
}

u128 reduce_ref(u256 c) {
  std::uint64_t w[4] = {c.lo.lo, c.lo.hi, c.hi.lo, c.hi.hi};
  for (int i = 254; i >= 128; --i) {
    if (((w[i / 64] >> (i % 64)) & 1) == 0) continue;
    // Subtract P(x) * x^(i-128): clear bit i, XOR 0x87 in at bit i-128.
    int s = i - 128;
    w[i / 64] ^= 1ull << (i % 64);
    w[s / 64] ^= 0x87ull << (s % 64);
    if (s % 64 != 0) w[s / 64 + 1] ^= 0x87ull >> (64 - s % 64);
  }
  return {w[0], w[1]};
}

}  // namespace gf128
