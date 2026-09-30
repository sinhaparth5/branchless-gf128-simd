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

// 8x8 tile of M_red covering output byte i and input byte k, in the qword
// layout GF2P8AFFINEQB expects: byte 7-b holds the row for output bit b.
std::uint64_t tile(const Mred& m, int i, int k);

// For P(x) = x^128 + x^7 + x^2 + x + 1 the only nonzero tiles are a0 on the
// diagonal (i == k) and a1 below it (i == k + 1). Bits pushed past byte 15
// fold back through the same two tiles once more.
struct Tiles { std::uint64_t a0, a1; };
Tiles make_tiles(const Mred& m);

// Scalar model of the AVX-512 reduction, running the same tile steps one
// byte at a time. Lets the tiling be tested on CPUs without GFNI.
std::uint8_t affine_byte(std::uint64_t a, std::uint8_t x);  // GF2P8AFFINEQB, imm = 0
u128 reduce_affine(Tiles t, u256 c);

// AVX-512 + GFNI + VPCLMULQDQ kernel: r[n] = a[n] * b[n] for n = 0..3.
// Only call it after checking the CPU supports those instruction sets.
void mul4_avx512(Tiles t, const u128* a, const u128* b, u128* r);

}  // namespace gf128
