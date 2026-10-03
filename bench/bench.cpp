// Throughput, latency and branch behavior of each reduction and multiply.
//
//   gf128_bench                  every variant this CPU can run
//   gf128_bench reduce_matrix    only the named variants
//
// Throughput runs independent operations over a small working set that stays
// in L1. Latency runs a chain in which each result feeds the next input.
// GB/s counts one 16-byte block per multiplication or reduction, as GHASH
// does. On Linux the hardware counters (cycles, branches, branch misses) cover
// only the timed throughput loop.

#include <cstdio>
#include <cstring>
#include <vector>

#include "common.hpp"
#include "perf_counters.hpp"

using namespace gf128;

namespace {

constexpr std::size_t kN = 256;  // 256 * 64 bytes of inputs, outputs and products

struct Data {
  std::vector<u128> a, b;
  std::vector<u256> c;  // clmul_ref(a, b)
};

const Mred& mred() {
  static const Mred m = make_mred();
  return m;
}
const Tiles& tiles() {
  static const Tiles t = make_tiles(mred());
  return t;
}

const Data& data() {
  static const Data d = [] {
    bench::Rng rng{0x9E3779B97F4A7C15ull};
    Data r;
    for (std::size_t i = 0; i < kN; ++i) {
      r.a.push_back(rng.next128());
      r.b.push_back(rng.next128());
      r.c.push_back(clmul_ref(r.a.back(), r.b.back()));
    }
    return r;
  }();
  return d;
}

using Reduce = u128 (*)(u256);
u128 do_reduce_ref(u256 c) { return reduce_ref(c); }
u128 do_reduce_matrix(u256 c) { return reduce_matrix(mred(), c); }
u128 do_reduce_affine(u256 c) { return reduce_affine(tiles(), c); }

template <Reduce F>
std::uint64_t reduce_tput(std::size_t reps) {
  const Data& d = data();
  std::uint64_t s = 0;
  for (std::size_t r = 0; r < reps; ++r)
    for (std::size_t i = 0; i < kN; ++i) s ^= bench::fold(F(d.c[i]));
  return s;
}

template <Reduce F>
std::uint64_t reduce_chain(std::size_t n) {
  u128 x = data().a[0];
  for (std::size_t i = 0; i < n; ++i) x = F({x, x});
  return bench::fold(x);
}

template <Reduce F>
bool reduce_check() {
  const Data& d = data();
  for (std::size_t i = 0; i < kN; ++i) {
    u128 r = F(d.c[i]), e = reduce_ref(d.c[i]);
    if (r.lo != e.lo || r.hi != e.hi) return false;
  }
  return true;
}

std::uint64_t mul_ref_tput(std::size_t reps) {
  const Data& d = data();
  std::uint64_t s = 0;
  for (std::size_t r = 0; r < reps; ++r)
    for (std::size_t i = 0; i < kN; ++i) s ^= bench::fold(mul_ref(d.a[i], d.b[i]));
  return s;
}

std::uint64_t mul_ref_chain(std::size_t n) {
  u128 x = data().a[0];
  const u128 h = data().b[0];
  for (std::size_t i = 0; i < n; ++i) x = mul_ref(x, h);
  return bench::fold(x);
}

bool mul_ref_check() { return true; }  // it is the reference

#if GF128_AVX512_KERNEL
std::uint64_t mul4_tput(std::size_t reps) {
  const Data& d = data();
  std::uint64_t s = 0;
  u128 r[4];
  for (std::size_t rep = 0; rep < reps; ++rep)
    for (std::size_t i = 0; i < kN; i += 4) {
      mul4_avx512(tiles(), &d.a[i], &d.b[i], r);
      s ^= bench::fold(r[0]) ^ bench::fold(r[1]) ^ bench::fold(r[2]) ^ bench::fold(r[3]);
    }
  return s;
}

std::uint64_t mul4_chain(std::size_t n) {  // n multiplications, 4 per call
  u128 x[4], h[4];
  for (int l = 0; l < 4; ++l) x[l] = data().a[l], h[l] = data().b[l];
  for (std::size_t i = 0; i < n; i += 4) mul4_avx512(tiles(), x, h, x);
  return bench::fold(x[0]);
}

bool mul4_check() {
  const Data& d = data();
  u128 r[4];
  for (std::size_t i = 0; i < kN; i += 4) {
    mul4_avx512(tiles(), &d.a[i], &d.b[i], r);
    for (int l = 0; l < 4; ++l) {
      u128 e = mul_ref(d.a[i + l], d.b[i + l]);
      if (r[l].lo != e.lo || r[l].hi != e.hi) return false;
    }
  }
  return true;
}
#endif

struct Variant {
  const char* name;
  std::uint64_t (*tput)(std::size_t reps);  // reps * kN operations
  std::uint64_t (*chain)(std::size_t n);    // n dependent operations
  bool (*check)();
  bool needs_avx512;
};

const Variant kVariants[] = {
    {"reduce_ref", reduce_tput<do_reduce_ref>, reduce_chain<do_reduce_ref>,
     reduce_check<do_reduce_ref>, false},
    {"reduce_matrix", reduce_tput<do_reduce_matrix>, reduce_chain<do_reduce_matrix>,
     reduce_check<do_reduce_matrix>, false},
    {"reduce_affine", reduce_tput<do_reduce_affine>, reduce_chain<do_reduce_affine>,
     reduce_check<do_reduce_affine>, false},
    {"mul_ref", mul_ref_tput, mul_ref_chain, mul_ref_check, false},
#if GF128_AVX512_KERNEL
    {"mul4_avx512", mul4_tput, mul4_chain, mul4_check, true},
#endif
};

volatile std::uint64_t g_sink;

// Doubles reps until one run takes at least 200 ms; returns that reps.
std::size_t calibrate(std::uint64_t (*f)(std::size_t)) {
  std::size_t reps = 1;
  for (;;) {
    double t0 = bench::now_ns();
    g_sink = g_sink ^ f(reps);
    if (bench::now_ns() - t0 >= 2e8) return reps;
    reps *= 2;
  }
}

void run(const Variant& v) {
  if (!v.check()) {
    std::printf("%-14s  WRONG RESULT, not timed\n", v.name);
    return;
  }
  const std::size_t reps = calibrate(v.tput);
  const double ops = static_cast<double>(reps * kN);

  bench::Counters pmu;
  const std::uint64_t s0 = bench::stamp();
  const double t0 = bench::now_ns();
  pmu.start();
  g_sink = g_sink ^ v.tput(reps);
  pmu.stop();
  const double ns = (bench::now_ns() - t0) / ops;
  const double ticks = static_cast<double>(bench::stamp() - s0) / ops;

  const std::size_t chain_n = reps * kN / 4;
  const double c0 = bench::now_ns();
  g_sink = g_sink ^ v.chain(chain_n);
  const double lat = (bench::now_ns() - c0) / static_cast<double>(chain_n);

  std::printf("%-14s %9.2f %8.3f %9.2f", v.name, ns, 16.0 / ns, lat);
#if GF128_BENCH_TSC
  std::printf(" %9.1f", ticks);
#else
  (void)ticks;
#endif
  if (pmu.ok()) {
    auto per = [&](int i) { return static_cast<double>(pmu.value(i)) / ops; };
    double br = per(bench::Counters::kBranches), miss = per(bench::Counters::kBranchMisses);
    std::printf(" %9.1f %9.1f %9.2f %7.2f%%", per(bench::Counters::kCycles), br, miss,
                br > 0 ? 100.0 * miss / br : 0.0);
  }
  std::printf("\n");
}

}  // namespace

int main(int argc, char** argv) {
  const bool avx512 = cpu_has_avx512_gfni();
  bench::Counters probe;

  std::printf("%-14s %9s %8s %9s", "variant", "ns/op", "GB/s", "lat ns");
#if GF128_BENCH_TSC
  std::printf(" %9s", "tsc/op");
#endif
  if (probe.ok()) std::printf(" %9s %9s %9s %8s", "cycles/op", "br/op", "miss/op", "miss");
  std::printf("\n");

  int ran = 0;
  for (const Variant& v : kVariants) {
    bool wanted = argc < 2;
    for (int i = 1; i < argc; ++i) wanted = wanted || std::strcmp(argv[i], v.name) == 0;
    if (!wanted) continue;
    if (v.needs_avx512 && !avx512) {
      std::printf("%-14s  skipped: CPU lacks AVX-512/GFNI/VPCLMULQDQ\n", v.name);
      continue;
    }
    run(v);
    ++ran;
  }
  if (!probe.ok()) std::printf("(hardware counters unavailable: Linux perf_event_open only)\n");
  return ran > 0 ? 0 : 1;
}
