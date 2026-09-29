# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Branchless, constant-time vector acceleration of GF(2^128) finite field arithmetic via AVX-512/GFNI matrix projection. Licensed Apache-2.0.

## Status

The repository has no source code yet. The README describes the planned setup: C++17, CMake ≥ 3.18, GCC ≥ 11 or Clang ≥ 12, and an out-of-source build (`mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)`). It also plans verification with Linux `perf` counters and `dudect` leakage tests, plus ARM NEON/SVE2 fallbacks. None of this exists yet, so update this file once `CMakeLists.txt` and sources are added.

## Constraints implied by the project goal

- **Constant-time:** no secret-dependent branches, loop bounds, or memory indices. Use masks and SIMD selects, not `if` or ternaries on secret data.
- **Target ISA:** AVX-512 plus GFNI (`vgf2p8affineqb`, `vgf2p8mulb`). Code and tests need hardware or an emulator (for example Intel SDE) that supports these instructions.
