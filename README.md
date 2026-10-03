# branchless-gf128-simd

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![C%2B%2B Standard](https://img.shields.io/badge/C%2B%2B-17-green.svg)](https://en.cppreference.com/w/cpp/17)
[![ISA Support](https://img.shields.io/badge/ISA-AVX--512%20%7C%20GFNI-orange.svg)]()

This is the companion code for the manuscript *"Hardware-Software Co-Design of a Branchless SIMD Vector Extension for Finite Field Arithmetic over GF(2^128)"*.

## Overview

Reduction over the binary extension field $\text{GF}(2^{128})$ is usually done in one of two ways. Conditional branching loops leak timing and are exposed to Branch Target Buffer flushes. RAM lookup tables are exposed to L1 cache side-channel attacks.

This library reduces without branches, in constant time, using a linear projection matrix $\mathbf{M}_{red} \in \text{GF}(2)^{128 \times 127}$. The matrix is tiled into $8 \times 8$ byte-affine transformations, so the whole reduction runs in vector registers using Intel AVX-512 and GFNI instructions (`VPCLMULQDQ` and `GF2P8AFFINEQB`).

## Features

- The reduction loop has no branch instructions (`js`, `jne`) and no memory lookup tables, so it runs in constant time.
- The $128 \times 127$ binary matrix products are computed in registers without touching the L1 data cache.
- Timing behavior is checked with Linux `perf` event counters and statistical leakage tests (`dudect`).
- The main target is x86-64 with AVX-512/GFNI, with fallback mappings for ARM NEON and SVE2.

## Hardware requirements

The native AVX-512/GFNI kernel needs a processor with both instruction sets:

- Intel: Ice Lake (3rd Gen Xeon), Tiger Lake (11th Gen Core), Sapphire Rapids, or newer.
- AMD: Zen 4 (EPYC 9004 / Ryzen 7000), Zen 5, or newer.

## Building

### Prerequisites

- GCC 11+, Clang 12+, or Visual Studio 2022 (MSVC or clang-cl)
- CMake 3.20+
- A Linux kernel with `perf_event_open` access

### Compiling

```bash
git clone https://github.com/sinhaparth5/branchless-gf128-simd.git
cd branchless-gf128-simd
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --config Release
ctest --test-dir build -C Release --output-on-failure
```

This works the same way on Linux, macOS and Windows (Visual Studio, clang-cl or MinGW). The AVX-512 kernel is built only for x86-64 targets. Elsewhere, for example on Apple Silicon, the portable scalar and matrix code still builds and is tested, and the kernel test is reported as skipped.

## Citation

If you use this code, please cite:

```bibtex
@article{sinha2026branchless,
  title={Hardware-Software Co-Design of a Branchless SIMD Vector Extension for Finite Field Arithmetic over GF(2^128)},
  author={Sinha, Parth},
  journal={IEEE Transactions on Computers (Under Review)},
  year={2026}
}
```
