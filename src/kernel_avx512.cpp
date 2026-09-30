#include <immintrin.h>

#include "gf128/gf128.hpp"

namespace gf128 {

// One field multiplication per 128-bit lane. Every step mirrors reduce_affine.
void mul4_avx512(Tiles t, const u128* a, const u128* b, u128* r) {
  const __m512i x = _mm512_loadu_si512(a), y = _mm512_loadu_si512(b);
  const __m512i a0 = _mm512_set1_epi64(static_cast<long long>(t.a0));
  const __m512i a1 = _mm512_set1_epi64(static_cast<long long>(t.a1));

  // 255-bit carry-less product: lo + mid*x^64 + hi*x^128.
  __m512i lo = _mm512_clmulepi64_epi128(x, y, 0x00);
  __m512i hi = _mm512_clmulepi64_epi128(x, y, 0x11);
  __m512i mid = _mm512_xor_si512(_mm512_clmulepi64_epi128(x, y, 0x01),
                                 _mm512_clmulepi64_epi128(x, y, 0x10));
  lo = _mm512_xor_si512(lo, _mm512_bslli_epi128(mid, 8));
  __m512i v = _mm512_xor_si512(hi, _mm512_bsrli_epi128(mid, 8));

  // r = c_lo ^ M_red * v_hi.
  __m512i d0 = _mm512_gf2p8affine_epi64_epi8(v, a0, 0);
  __m512i d1 = _mm512_gf2p8affine_epi64_epi8(v, a1, 0);
  __m512i h = _mm512_bsrli_epi128(d1, 15);
  __m512i g0 = _mm512_gf2p8affine_epi64_epi8(h, a0, 0);
  __m512i g1 = _mm512_gf2p8affine_epi64_epi8(h, a1, 0);
  __m512i s = _mm512_bslli_epi128(_mm512_xor_si512(d1, g1), 1);
  __m512i out = _mm512_ternarylogic_epi64(lo, d0, g0, 0x96);  // lo ^ d0 ^ g0
  _mm512_storeu_si512(r, _mm512_xor_si512(out, s));
}

}  // namespace gf128
