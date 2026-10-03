// Timing-leakage test in the style of dudect (Reparaz, Balasch, Verbauwhede,
// "Dude, is my code constant time?", DATE 2017).
//
//   gf128_ct                         every target this CPU can run
//   gf128_ct reduce_matrix           only the named targets
//   options: --measurements N (default 1000000), --batch B (default 16),
//            --fixed zero|ones (default zero)
//
// Each measurement times a batch of B operations whose inputs all come from
// one class: fixed (every operand word is the --fixed pattern) or random.
// The class of each measurement is chosen at random, so drift and noise hit
// both classes alike. Welch's t-test compares the two timing distributions,
// once on all measurements and once per upper-tail crop at a set of
// percentiles, as dudect does. A max |t| of 4.5 or more means the timing
// depends on the data.
//
// The *_ref targets branch on data and should be flagged. They show that the
// harness can see a leak on this machine. Exit status is 1 when a target that
// is meant to be constant time is flagged.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "common.hpp"

using namespace gf128;

namespace {

struct Item { u128 a, b; };  // reduce targets read it as u256 {lo = a, hi = b}

const Mred& mred() {
  static const Mred m = make_mred();
  return m;
}
const Tiles& tiles() {
  static const Tiles t = make_tiles(mred());
  return t;
}

std::uint64_t t_reduce_ref(const Item* in, std::size_t n) {
  std::uint64_t s = 0;
  for (std::size_t i = 0; i < n; ++i) s ^= bench::fold(reduce_ref({in[i].a, in[i].b}));
  return s;
}
std::uint64_t t_reduce_matrix(const Item* in, std::size_t n) {
  std::uint64_t s = 0;
  for (std::size_t i = 0; i < n; ++i) s ^= bench::fold(reduce_matrix(mred(), {in[i].a, in[i].b}));
  return s;
}
std::uint64_t t_reduce_affine(const Item* in, std::size_t n) {
  std::uint64_t s = 0;
  for (std::size_t i = 0; i < n; ++i) s ^= bench::fold(reduce_affine(tiles(), {in[i].a, in[i].b}));
  return s;
}
std::uint64_t t_mul_ref(const Item* in, std::size_t n) {
  std::uint64_t s = 0;
  for (std::size_t i = 0; i < n; ++i) s ^= bench::fold(mul_ref(in[i].a, in[i].b));
  return s;
}
#if GF128_AVX512_KERNEL
std::uint64_t t_mul4_avx512(const Item* in, std::size_t n) {  // n is a multiple of 4
  std::uint64_t s = 0;
  u128 a[4], b[4], r[4];
  for (std::size_t i = 0; i < n; i += 4) {
    for (int l = 0; l < 4; ++l) a[l] = in[i + l].a, b[l] = in[i + l].b;
    mul4_avx512(tiles(), a, b, r);
    s ^= bench::fold(r[0]) ^ bench::fold(r[1]) ^ bench::fold(r[2]) ^ bench::fold(r[3]);
  }
  return s;
}
#endif

struct Target {
  const char* name;
  std::uint64_t (*run)(const Item*, std::size_t);
  bool constant_time;  // false for the branching references
  bool needs_avx512;
};

const Target kTargets[] = {
    {"reduce_ref", t_reduce_ref, false, false},
    {"reduce_matrix", t_reduce_matrix, true, false},
    {"reduce_affine", t_reduce_affine, true, false},
    {"mul_ref", t_mul_ref, false, false},
#if GF128_AVX512_KERNEL
    {"mul4_avx512", t_mul4_avx512, true, true},
#endif
};

// Online Welch t-test (Welford's running mean and variance per class).
struct Welch {
  double n[2] = {}, mean[2] = {}, m2[2] = {};
  void push(int c, double x) {
    n[c] += 1;
    double d = x - mean[c];
    mean[c] += d / n[c];
    m2[c] += d * (x - mean[c]);
  }
  double t() const {
    if (n[0] < 2 || n[1] < 2) return 0;
    double se = std::sqrt(m2[0] / (n[0] - 1) / n[0] + m2[1] / (n[1] - 1) / n[1]);
    return se > 0 ? (mean[0] - mean[1]) / se : 0;
  }
};

constexpr int kCrops = 20;
constexpr std::size_t kChunk = 10000;     // measurements per batch of prepared inputs
constexpr double kMinSamples = 10000;     // crops with fewer samples are not reported
constexpr double kThreshold = 4.5;

volatile std::uint64_t g_sink;

struct Options {
  std::size_t measurements = 1000000;
  std::size_t batch = 16;
  std::uint64_t pattern = 0;
};

struct Result {
  double max_t = 0;
  int at = -1;  // -1 = no crop, else crop index
  double mean[2] = {};
};

Result test(const Target& tg, const Options& o, bench::Rng& rng) {
  const std::size_t batch = tg.needs_avx512 ? (o.batch + 3) / 4 * 4 : o.batch;
  const u128 fixed{o.pattern, o.pattern};
  std::vector<Item> in(kChunk * batch);
  std::vector<int> cls(kChunk);
  std::vector<double> dt(kChunk);
  std::vector<Welch> w(1 + kCrops);
  std::vector<double> crop(kCrops);

  // Chunk 0 is a warm-up: it sets the crop percentiles and is not counted.
  const std::size_t chunks = (o.measurements + kChunk - 1) / kChunk;
  for (std::size_t ch = 0; ch <= chunks; ++ch) {
    for (std::size_t i = 0; i < kChunk; ++i) {
      cls[i] = static_cast<int>(rng.next() & 1);
      for (std::size_t k = 0; k < batch; ++k) {
        Item& it = in[i * batch + k];
        if (cls[i] == 0) it = {fixed, fixed};
        else it = {rng.next128(), rng.next128()};
      }
    }
    for (std::size_t i = 0; i < kChunk; ++i) {
      const std::uint64_t t0 = bench::stamp();
      g_sink = g_sink ^ tg.run(&in[i * batch], batch);
      dt[i] = static_cast<double>(bench::stamp() - t0);
    }
    if (ch == 0) {
      std::vector<double> s(dt);
      std::sort(s.begin(), s.end());
      for (int k = 0; k < kCrops; ++k) {
        double p = 1 - std::pow(0.5, 10.0 * (k + 1) / kCrops);
        crop[k] = s[static_cast<std::size_t>(p * static_cast<double>(s.size() - 1))];
      }
      continue;
    }
    for (std::size_t i = 0; i < kChunk; ++i) {
      w[0].push(cls[i], dt[i]);
      for (int k = 0; k < kCrops; ++k)
        if (dt[i] < crop[k]) w[1 + k].push(cls[i], dt[i]);
    }
  }

  Result r;
  for (int k = 0; k <= kCrops; ++k) {
    if (w[k].n[0] + w[k].n[1] < kMinSamples) continue;
    double t = std::fabs(w[k].t());
    if (t > r.max_t) r.max_t = t, r.at = k - 1;
  }
  r.mean[0] = w[0].mean[0] / static_cast<double>(batch);
  r.mean[1] = w[0].mean[1] / static_cast<double>(batch);
  return r;
}

bool parse(int argc, char** argv, Options& o, std::vector<const char*>& names) {
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    const char* v = i + 1 < argc ? argv[i + 1] : nullptr;
    if (std::strcmp(a, "--measurements") == 0 && v) {
      o.measurements = std::strtoull(v, nullptr, 10), ++i;
    } else if (std::strcmp(a, "--batch") == 0 && v) {
      o.batch = std::strtoull(v, nullptr, 10), ++i;
    } else if (std::strcmp(a, "--fixed") == 0 && v && std::strcmp(v, "zero") == 0) {
      o.pattern = 0, ++i;
    } else if (std::strcmp(a, "--fixed") == 0 && v && std::strcmp(v, "ones") == 0) {
      o.pattern = ~std::uint64_t{0}, ++i;
    } else if (a[0] == '-') {
      return false;
    } else {
      names.push_back(a);
    }
  }
  return o.measurements > 0 && o.batch > 0;
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  std::vector<const char*> names;
  if (!parse(argc, argv, o, names)) {
    std::fprintf(stderr,
                 "usage: gf128_ct [target...] [--measurements N] [--batch B] "
                 "[--fixed zero|ones]\n");
    return 2;
  }
  const bool avx512 = cpu_has_avx512_gfni();
  std::printf("%zu measurements of %zu ops, fixed class = %s, time in %s per op\n",
              o.measurements, o.batch, o.pattern ? "ones" : "zero", bench::stamp_unit());
  std::printf("%-14s %10s %10s %8s %7s  %s\n", "target", "fixed", "random", "max|t|", "crop",
              "verdict");

  bench::Rng rng{0x2545F4914F6CDD1Dull};
  bool failed = false;
  for (const Target& tg : kTargets) {
    bool wanted = names.empty();
    for (const char* n : names) wanted = wanted || std::strcmp(n, tg.name) == 0;
    if (!wanted) continue;
    if (tg.needs_avx512 && !avx512) {
      std::printf("%-14s  skipped: CPU lacks AVX-512/GFNI/VPCLMULQDQ\n", tg.name);
      continue;
    }
    const Result r = test(tg, o, rng);
    const bool leak = r.max_t >= kThreshold;
    const char* verdict = leak ? (tg.constant_time ? "LEAK" : "leak (expected: branching reference)")
                               : (tg.constant_time ? "ok" : "no leak seen (harness may lack power)");
    char at[16] = "all";  // crop percentile the max |t| came from
    if (r.at >= 0)
      std::snprintf(at, sizeof at, "p%.1f", 100 * (1 - std::pow(0.5, 10.0 * (r.at + 1) / kCrops)));
    std::printf("%-14s %10.2f %10.2f %8.2f %7s  %s\n", tg.name, r.mean[0], r.mean[1], r.max_t, at,
                verdict);
    std::fflush(stdout);
    failed = failed || (leak && tg.constant_time);
  }
  return failed ? 1 : 0;
}
