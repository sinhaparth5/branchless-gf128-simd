# Roadmap

This roadmap combines the manuscript draft (`docs/research_paper.pdf`) and the research tracker (`docs/research_paper_tracker.pdf`, dated September 22, 2026). The target venues are IEEE Transactions on Computers and IEEE TCAS-I.

## Goals

1. Replace the conditional long-division reduction loop with a static binary projection matrix $`\mathbf{M}_{red} \in \text{GF}(2)^{128 \times 127}`$.
2. Map the matrix product onto SIMD registers as $`8 \times 8`$ bit-matrix transforms using `GF2P8AFFINEQB`, with `VPCLMULQDQ` for the carry-less product.
3. Show that execution time does not depend on input data, using `dudect` with a threshold of $`|t| < 4.5`$.

## Schedule

| Weeks | Deliverable | Status |
| ----- | ----------- | ------ |
| 1-2 | Finish the MIT 6.004 readings and derive $`\mathbf{M}_{red}`$ | In progress |
| 3-4 | Implement the AVX-512 `GF2P8AFFINEQB` kernel | Pending |
| 5-6 | Measure cycle counts with `perf stat` and run the `dudect` analysis | Pending |
| 7-8 | Finish the IEEE manuscript and upload the preprint to Preprints.org | Pending |

## Phase 1: Theory and matrix derivation (weeks 1-2)

The field is $`\text{GF}(2^{128}) \cong \text{GF}(2)[x]/\langle P(x) \rangle`$ with $`P(x) = x^{128} + x^7 + x^2 + x + 1`$.

- [x] Split the 255-bit carry-less product into $`c_{lo}`$ (bits 0-127) and $`v_{hi}`$ (bits 128-254), so that $`C_{raw}(x) = c_{lo}(x) + x^{128} \cdot v_{hi}(x)`$.
- [x] For each $`j`$ from 0 to 126, compute the residue $`R_j(x) = x^{128+j} \bmod P(x)`$. These residues are the columns of $`\mathbf{M}_{red}`$.
- [x] Check that $`r = c_{lo} \oplus (\mathbf{M}_{red} \cdot v_{hi})`$ matches the reference shift-and-XOR reduction.
- [ ] Complete the reading list below and write up the paper notes it calls for.

## Phase 2: SIMD kernel (weeks 3-4)

- [x] Write a scalar reference reduction (the loop in equation 6 of the paper) to use as a correctness oracle and a performance baseline.
- [x] Set up the CMake build (C++17).
- [x] Split $`\mathbf{M}_{red}`$ into $`8 \times 8`$ tiles in `GF2P8AFFINEQB` layout and check the tiled reduction with a scalar model of the instruction (`reduce_affine`).
- [x] Write the kernel `mul4_avx512`: 4 `VPCLMULQDQ` for the 255-bit product and 4 `GF2P8AFFINEQB` for the reduction, 4 multiplications per 512-bit register.
- [ ] Install Intel SDE and pass `test_kernel` under emulation.
- [ ] Pass `test_kernel` on real AVX-512/GFNI hardware.

## Phase 3: Evaluation (weeks 5-6)

- [ ] Branch behavior: use `perf stat` to confirm zero conditional branches and a 0% branch miss rate in the reduction. The scalar loop makes $`127 \times N`$ conditional checks for $`N`$ inputs, with a miss rate near 50% on random data.
- [ ] Latency: measure $`T_{exec}`$ and check the paper's claim that the reduction takes 2 to 3 cycles and `VPCLMULQDQ` takes about 3 cycles.
- [ ] Throughput: measure GB/s against OpenSSL and libsodium.
- [ ] Constant time: run `dudect` with a fixed class (uniform zero/one patterns) and a random class (pseudo-random field elements). Welch's t-test must give $`|t| < 4.5`$ over millions of traces.
- [ ] Penalty model: fit the pipeline-flush model $`T_{total} = T_{exec} + N_{miss} \cdot \Delta t_{flush}`$ to the scalar baseline.

## Phase 4: Manuscript (weeks 7-8)

- [ ] Replace the theoretical claims in Section V with measured results.
- [ ] Finish the IEEE draft and upload the preprint to Preprints.org.

## Reading list: MIT 6.004 Computation Structures

These sections support the hardware hazard analysis in the paper. Everything else in the 663-page notes is out of scope.

| Module | Pages | Concepts | Used in the paper for |
| ------ | ----- | -------- | --------------------- |
| L04: Combinational logic | 79-108 | Propagation delay ($`t_{PD}`$), XOR trees | $`O(1)`$ delay in GF($`2^m`$) compared with integer adders |
| L05-L06: Sequential logic and FSMs | 115-148 | Setup and hold times, FSM loops | Multi-cycle sequential loop hazards |
| L07-L08: Performance and pipelining | 168-205 | Latency, throughput, bottlenecks | Hardware cycle metrics |
| L15: Pipelining the Beta | 430-472 | Hazards, flushes, BTB misses | Control hazard and pipeline flush theory |
| L14: Memory hierarchy (skim) | 382-400 | Cache hit and miss penalties | Cache-timing side channels |

### Checklist

L04: Combinational logic
- [ ] Truth tables, Boolean algebra, logic gates, XOR networks (pp. 79-95)
- [ ] Propagation delay ($`t_{PD}`$) and contamination delay ($`t_{CD}`$) (pp. 100-108)
- [ ] Paper note: show that vector XOR has no carry-chain delay, so $`t_{PD,XOR} = O(1)`$.

L05-L06: Sequential logic and FSMs
- [ ] Setup ($`t_S`$) and hold ($`t_H`$) times, register timing constraints (pp. 115-122)
- [ ] FSM state transitions and next-state logic (pp. 135-148)
- [ ] Paper note: use FSM transition equations to show why the division loop creates sequential dependencies across 127 iterations.

L07-L08: Performance and design tradeoffs
- [ ] Latency and throughput metrics (pp. 168-175)
- [ ] Pipelining rules and structural bottlenecks (pp. 176-190)
- [ ] Paper note: define $`T_{exec}`$ and throughput (GB/s) for the OpenSSL and libsodium comparison.

L15: Pipelining the Beta processor
- [ ] Control hazards, data hazards, branch delay slots (pp. 430-445)
- [ ] Pipeline flushes and BTB misses (pp. 446-460)
- [ ] Paper note: write out the flush penalty model $`T_{total} = T_{exec} + N_{miss} \cdot \Delta t_{flush}`$.

Skip: L01-L03 (information, digital abstraction, CMOS), L10a-L12 (assembly, models of computation, compilers, stacks), L13 (building the Beta), and L16-L22 (virtual memory, OS, devices, interrupts, concurrency).
