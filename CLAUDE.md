# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Branchless, constant-time vector acceleration of GF(2^128) finite field arithmetic via AVX-512/GFNI matrix projection. Licensed Apache-2.0.

## Commands

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure      # all tests
ctest --test-dir build -R reference             # one test by name
```

## Layout

- `include/gf128/gf128.hpp`: public API. `u128`/`u256` store bit i as the coefficient of x^i.
- `src/reference.cpp`: scalar shift-and-XOR `clmul_ref`/`reduce_ref`. This version branches on data on purpose. It is the correctness check and performance baseline for the SIMD kernel, so never call it from constant-time code.
- `src/matrix.cpp`: `make_mred` builds the 127 columns of M_red (x^(128+j) mod P) and `reduce_matrix` computes `c_lo ^ M_red*v_hi` with masks instead of branches. This is the portable form the SIMD kernel must match.
- `tests/`: plain executables registered with CTest, sharing `tests/check.hpp` (`CHECK`, fixed-seed `rnd128`). No test framework is used. To add a test, create `tests/test_<name>.cpp` and add `<name>` to the `foreach` in `CMakeLists.txt`.
- `ROADMAP.md`: phases and checklist taken from the manuscript and tracker PDFs in `docs/`.

The AVX-512/GFNI kernel, the `perf`/`dudect` benchmarks, and the ARM fallbacks described in the README do not exist yet.

## Constraints implied by the project goal

- **Constant-time:** no secret-dependent branches, loop bounds, or memory indices. Use masks and SIMD selects, not `if` or ternaries on secret data.
- **Target ISA:** AVX-512 plus GFNI (`vgf2p8affineqb`, `vgf2p8mulb`). Code and tests need hardware or an emulator (for example Intel SDE) that supports these instructions.
