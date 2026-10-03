#include "gf128/gf128.hpp"

namespace gf128 {

// M_red is public and built once, so using the branching reference here is fine.
Mred make_mred() {
  Mred m{};
  for (int j = 0; j < 127; ++j) {
    u256 xk{};  // x^(128+j): bit j of the upper half
    (j < 64 ? xk.hi.lo : xk.hi.hi) = 1ull << (j % 64);
    m[j] = reduce_ref(xk);
  }
  return m;
}

u128 reduce_matrix(const Mred& m, u256 c) {
  u128 r = c.lo;
  for (int j = 0; j < 127; ++j) {
    std::uint64_t bit = ((j < 64 ? c.hi.lo : c.hi.hi) >> (j % 64)) & 1;
    std::uint64_t mask = 0 - bit;  // all ones when coefficient j of v_hi is 1
    r.lo ^= m[j].lo & mask;
    r.hi ^= m[j].hi & mask;
  }
  return r;
}

std::uint64_t tile(const Mred& m, int i, int k) {
  std::uint64_t a = 0;
  for (int b = 0; b < 8; ++b) {
    for (int t = 0; t < 8; ++t) {
      int row = 8 * i + b, col = 8 * k + t;
      if (col == 127) continue;  // M_red has 127 columns
      std::uint64_t w = row < 64 ? m[col].lo : m[col].hi;
      a |= ((w >> (row % 64)) & 1) << (8 * (7 - b) + t);
    }
  }
  return a;
}

Tiles make_tiles(const Mred& m) { return {tile(m, 0, 0), tile(m, 1, 0)}; }

static unsigned parity8(unsigned x) {  // portable __builtin_parity, no branches
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return x & 1;
}

std::uint8_t affine_byte(std::uint64_t a, std::uint8_t x) {
  std::uint8_t r = 0;
  for (int b = 0; b < 8; ++b) {
    auto row = static_cast<std::uint8_t>(a >> (8 * (7 - b)));
    r |= static_cast<std::uint8_t>(parity8(row & x) << b);
  }
  return r;
}

static u128 affine(u128 v, std::uint64_t a) {  // affine_byte on all 16 bytes
  u128 r{};
  for (int n = 0; n < 8; ++n) {
    r.lo |= std::uint64_t{affine_byte(a, static_cast<std::uint8_t>(v.lo >> (8 * n)))} << (8 * n);
    r.hi |= std::uint64_t{affine_byte(a, static_cast<std::uint8_t>(v.hi >> (8 * n)))} << (8 * n);
  }
  return r;
}

static u128 operator^(u128 x, u128 y) { return {x.lo ^ y.lo, x.hi ^ y.hi}; }

u128 reduce_affine(Tiles t, u256 c) {
  u128 v = c.hi;
  u128 d0 = affine(v, t.a0), d1 = affine(v, t.a1);
  u128 h{d1.hi >> 56, 0};  // byte 15 of d1 is x^128.. again: fold it once more
  u128 g0 = affine(h, t.a0), g1 = affine(h, t.a1);
  u128 s = d1 ^ g1;        // a1 output belongs one byte up
  s = {s.lo << 8, (s.hi << 8) | (s.lo >> 56)};
  return c.lo ^ d0 ^ g0 ^ s;
}

}  // namespace gf128
