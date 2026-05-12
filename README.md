# YAGS Branch Predictor Implementation in gem5

**EC 513 — Computer Architecture | Boston University | Spring 2026**  
**Authors:** Brian Quijada, Fadi Kidess, Ozan Ekame Pekgoz, Will Borland  
**Instructor:** Prof. Ajay Joshi

> Based on: A. N. Eden and T. Mudge, "The YAGS Branch Prediction Scheme," in *Proceedings of the 31st Annual ACM/IEEE International Symposium on Microarchitecture (MICRO-31)*, Dec. 1998, pp. 69–77.

---
## Report & Presentation

- [Final Report (PDF)](EC513 YAGS project report - Brian Quijada, Fadi Kidess, Ozan Ekame Pekgoz, Will Borland.pdf)
- [Presentation Slides (PDF)](YAGS_Presentation.pdf)
---

## Overview

Branch prediction is critical for modern out-of-order processors — stalling the pipeline until a branch resolves would severely hurt performance. The central problem with global branch predictors is **PHT aliasing**: two different branches mapping to the same Pattern History Table entry. When aliasing is destructive (the two branches have conflicting histories), mispredictions follow.

YAGS (Yet Another Global Scheme) addresses this by splitting prediction into two components:

- **Choice PHT** — a standard bimodal predictor indexed by `PC XOR global history`. Captures each branch's dominant bias (mostly taken or mostly not-taken).
- **Tagged direction caches** — two small caches (one for taken exceptions, one for not-taken exceptions) that store only the cases where a branch defies its dominant bias. Each entry holds a tag from the LSBs of the branch address, virtually eliminating aliasing between consecutive branches.

**Prediction flow (example: branch biased toward taken):**
1. Consult choice PHT → predicts taken
2. Check not-taken cache for a tag match
3. Cache hit → use cache prediction (this is an exception to the bias)
4. Cache miss → use choice PHT prediction (bias holds)

This design reduces aliasing in two ways: the choice PHT handles strongly biased branches cleanly, and the tagged direction caches prevent different branches from corrupting each other's exception entries.

---

## Implementation

YAGS is implemented as a new `BPredUnit` subclass in gem5, alongside existing predictors such as `BiModeBP` and `TournamentBP`.

**Key files:**
- `src/yags.hh` — class declaration, data structure definitions
- `src/yags.cc` — prediction logic, update logic, cache lookup and writeback

**Data structures:**

| Structure | Type | Size |
|---|---|---|
| Choice PHT | Vector of 2-bit saturating counters | 8192 entries |
| Taken cache | Vector of `{tag, 2-bit counter, valid}` | 8192 entries |
| Not-taken cache | Vector of `{tag, 2-bit counter, valid}` | 8192 entries |
| Tags | 13-bit LSBs of branch PC | — |

**Implementation notes:**
- Branch PC is shifted by `instShiftAmt` before indexing to account for 4-byte alignment (without this, the 2 LSBs are always zero and 75% of cache entries would be unused)
- Cache indexing uses modulo to prevent out-of-bounds access — an initial segmentation fault was caused by address overflow before this was added
- Tag width and cache depth are parameterizable via `BranchPredictor.py`

---

## Evaluation

Simulations were run in gem5 using the O3CPU model on five SPEC CPU 2017 benchmarks, compared against five existing gem5 predictors.

**Benchmarks:**

| Benchmark | Workload Type |
|---|---|
| 505.mcf_r | Route planning, integer arithmetic |
| 520.omnetpp_r | Network simulation, unpredictable branches |
| 523.xalancbmk_r | XML to HTML conversion, more predictable |
| 502.gcc_r | C compiler, complex control flow |
| 525.x264_r | Video compression |

**Misprediction Rate (%) — lower is better:**

| Predictor | 505.mcf_r | 520.omnetpp_r | 523.xalancbmk_r | 502.gcc_r | 525.x264_r |
|---|---|---|---|---|---|
| **YAGS** | **6.5%** | **6.47%** | **6.46%** | **5.35%** | **5.33%** |
| mpp8kb | 6.4% | 6.4% | 6.5% | 6.4% | 6.4% |
| gshare | 6.5% | 6.5% | 6.5% | 6.5% | 6.5% |
| tournament | 4.6% | 4.5% | 4.6% | 4.6% | 4.6% |
| tage | 3.1% | 3.1% | 3.1% | 3.1% | 3.1% |
| localBP | 5.5% | 5.6% | 5.6% | 5.5% | 5.6% |

**IPC — higher is better:**

| Predictor | 505.mcf_r | 520.omnetpp_r | 523.xalancbmk_r | 502.gcc_r | 525.x264_r |
|---|---|---|---|---|---|
| **YAGS** | **0.363** | **0.363** | **0.363** | **0.363** | **0.361** |
| mpp8kb | 0.484 | 0.484 | 0.484 | 0.484 | 0.484 |
| gshare | 0.481 | 0.482 | 0.482 | 0.481 | 0.482 |
| tournament | 0.493 | 0.493 | 0.492 | 0.492 | 0.492 |
| tage | 0.501 | 0.501 | 0.501 | 0.501 | 0.501 |
| local | 0.488 | 0.489 | 0.489 | 0.488 | 0.489 |

YAGS misprediction rates are consistent with the paper's reported ~6% at comparable cache sizes. The IPC results did not behave as expected — values below 1.0 on an O3CPU and near-identical numbers across benchmarks indicate an issue with the simulation setup, likely in the CPU core switch logic after warmup. This is discussed in the report.

---

## Known Limitations

- Simulations used 1 billion warmup instructions instead of the intended 5 billion due to time constraints on BU's SCC cluster. Insufficient warmup likely contributed to the IPC anomaly.
- The CPU core switch after warmup may not have been correctly configured, possibly causing warmup-phase results to be recorded instead of measurement-phase results.
- Future work: sweep tag width and direction cache size ratio; validate core switch logic; rerun with full 5B warmup.

--

## Reference

A. N. Eden and T. Mudge, "The YAGS Branch Prediction Scheme," in *Proceedings of the 31st Annual ACM/IEEE International Symposium on Microarchitecture*, Dec. 1998, pp. 69–77. doi: 10.1109/MICRO.1998.742770.
