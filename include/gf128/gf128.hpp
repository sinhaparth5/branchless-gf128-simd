#pragma once

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

}  // namespace gf128
