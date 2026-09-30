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

}  // namespace gf128
