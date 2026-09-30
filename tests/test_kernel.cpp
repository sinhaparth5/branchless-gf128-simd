#include "check.hpp"

using namespace gf128;

int main() {
  if (!__builtin_cpu_supports("avx512f") || !__builtin_cpu_supports("avx512bw") ||
      !__builtin_cpu_supports("gfni") || !__builtin_cpu_supports("vpclmulqdq")) {
    std::puts("skipped: CPU lacks AVX-512/GFNI/VPCLMULQDQ (run under Intel SDE)");
    return 77;
  }
  const Tiles t = make_tiles(make_mred());
  for (int n = 0; n < 10000; ++n) {
    u128 a[4], b[4], r[4];
    for (int l = 0; l < 4; ++l) a[l] = rnd128(), b[l] = rnd128();
    mul4_avx512(t, a, b, r);
    for (int l = 0; l < 4; ++l) CHECK(eq(r[l], mul_ref(a[l], b[l])));
  }
  return report();
}
