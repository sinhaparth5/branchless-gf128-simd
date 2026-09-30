#pragma once

#include <array>
#include <cstdint>

namespace gf128 {

// Bit i holds the coefficient of x^i.
struct u128 { std::uint64_t lo, hi; };
struct u256 { u128 lo, hi; };

// Scalar shift-and-XOR reference (equation 6 of the paper). Branches on data:
// use it as a correctness oracle and baseline, never in constant-time code.
u256 clmul_ref(u128 a, u128 b);
u128 reduce_ref(u256 c);  // mod P(x) = x^128 + x^7 + x^2 + x + 1

inline u128 mul_ref(u128 a, u128 b) { return reduce_ref(clmul_ref(a, b)); }

// Reduction as a matrix product over GF(2) (equations 9-11 of the paper).
// Column j of M_red is x^(128+j) mod P(x), for j = 0..126.
using Mred = std::array<u128, 127>;
Mred make_mred();

// r = c_lo XOR (M_red * v_hi). Constant time: every column is visited, and
// each is selected with an all-ones/all-zeros mask from one bit of v_hi.
u128 reduce_matrix(const Mred& m, u256 c);

}  // namespace gf128
