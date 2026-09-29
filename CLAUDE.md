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
- `tests/`: plain executables registered with CTest. No test framework is used.
- `ROADMAP.md`: phases and checklist taken from the manuscript and tracker PDFs in `docs/`.

The AVX-512/GFNI kernel, the `perf`/`dudect` benchmarks, and the ARM fallbacks described in the README do not exist yet.

## Constraints implied by the project goal

- **Constant-time:** no secret-dependent branches, loop bounds, or memory indices. Use masks and SIMD selects, not `if` or ternaries on secret data.
- **Target ISA:** AVX-512 plus GFNI (`vgf2p8affineqb`, `vgf2p8mulb`). Code and tests need hardware or an emulator (for example Intel SDE) that supports these instructions.
