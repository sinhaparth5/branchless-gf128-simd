# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Branchless, constant-time vector acceleration of GF(2^128) finite field arithmetic via AVX-512/GFNI matrix projection. Licensed Apache-2.0.

## Commands

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --config Release        # --config matters for multi-config generators (Visual Studio)
ctest --test-dir build -C Release --output-on-failure   # all tests
ctest --test-dir build -C Release -R reference           # one test by name
cmake -B build -DGF128_SDE=/path/to/sde64      # run test_kernel under Intel SDE (sde.exe on Windows)
```

Supported targets are macOS (arm64 and x86-64), Linux and Windows (MSVC `cl`, clang-cl, MinGW). CMake works out the target arch from `CMAKE_OSX_ARCHITECTURES`, then MSVC's `CMAKE_CXX_COMPILER_ARCHITECTURE_ID`, then `CMAKE_SYSTEM_PROCESSOR`. It builds `kernel_avx512.cpp` and defines `GF128_AVX512_KERNEL=1` (PUBLIC) only for x86-64. You can override this with `-DGF128_AVX512=OFF`. A macOS universal build (`arm64;x86_64`) leaves the kernel out.

`test_kernel` returns 77, which CTest reports as skipped, in two cases: when the kernel isn't built (non-x86), and when `cpu_has_avx512_gfni()` is false (unless the test runs under SDE). The project's own dev machine is an arm64 Mac, so the kernel never runs there. `test_affine` is the closest local check because it tests `reduce_affine`, the byte-by-byte model of the kernel.

## Layout

- `include/gf128/gf128.hpp`: public API. `u128`/`u256` store bit i as the coefficient of x^i.
- `src/reference.cpp`: scalar shift-and-XOR `clmul_ref`/`reduce_ref`. This version branches on data on purpose. It is the correctness check and performance baseline for the SIMD kernel, so never call it from constant-time code.
- `src/matrix.cpp`: `make_mred` builds the 127 columns of M_red (x^(128+j) mod P) and `reduce_matrix` computes `c_lo ^ M_red*v_hi` with masks instead of branches. This is the portable form the SIMD kernel must match.
  It also has `tile`/`make_tiles` (8x8 tiles of M_red in GF2P8AFFINEQB qword layout, where byte 7-b is the row for output bit b), `affine_byte` (a scalar model of the instruction), and `reduce_affine` (the kernel's exact steps done byte by byte). For this P(x) only two tile diagonals are nonzero (`a0`, `a1`), plus one extra fold of byte 15.
- `src/kernel_avx512.cpp`: `mul4_avx512`, 4 multiplications per zmm. It is the only file compiled with `-mavx512f -mavx512bw -mgfni -mvpclmulqdq` (set per file in CMake; MSVC `cl` needs no flags). Keep ISA flags off every other file so the rest runs on any CPU. Each step mirrors `reduce_affine`, so change the two together. The `mul4_avx512` declaration is behind `#if GF128_AVX512_KERNEL`, so callers must guard on that macro too.
- `src/cpu.cpp`: `cpu_has_avx512_gfni()`, runtime detection using CPUID and XCR0. On macOS it reads `hw.optional.avx512f` because the OS turns on AVX-512 state lazily. It always returns false off x86.
- Portability: no compiler builtins (`__builtin_*`) or `__int128` in shared code, because MSVC has neither. Write portable bit tricks instead, as `parity8` in `matrix.cpp` does.
- `bench/`: Phase 3 tools, built unless `-DGF128_BUILD_BENCH=OFF`. Use a Release build.
  - `gf128_bench [variant...]` reports ns/op, GB/s (16 bytes per op) and chain latency. On x86 it also reports TSC ticks/op. On Linux it also reports cycles, branches and branch misses per op from `perf_event_open`, counted only over the timed loop (needs `perf_event_paranoid` <= 2).
  - `gf128_ct [target...] [--measurements N] [--batch B] [--fixed zero|ones]` is the dudect-style leakage test. It exits 1 only when a constant-time target reaches |t| >= 4.5. `reduce_ref` and `mul_ref` are expected to leak; that shows the harness can detect a leak.
  - To add a variant, add an entry to `kVariants` (bench.cpp) or `kTargets` (ct.cpp). Guard AVX-512 entries with `#if GF128_AVX512_KERNEL` and set `needs_avx512`.
- `tests/`: plain executables registered with CTest, sharing `tests/check.hpp` (`CHECK`, fixed-seed `rnd128`). No test framework is used. To add a test, create `tests/test_<name>.cpp` and add `<name>` to the `foreach` in `CMakeLists.txt`.
- `ROADMAP.md`: phases and checklist taken from the manuscript and tracker PDFs in `docs/`.

The ARM NEON/SVE2 fallbacks described in the README do not exist yet. Comparisons against OpenSSL and libsodium have not been done yet either.

## Constraints implied by the project goal

- **Constant-time:** no secret-dependent branches, loop bounds, or memory indices. Use masks and SIMD selects, not `if` or ternaries on secret data.
- **Target ISA:** AVX-512 plus GFNI (`vgf2p8affineqb`, `vgf2p8mulb`). Code and tests need hardware or an emulator (for example Intel SDE) that supports these instructions.
