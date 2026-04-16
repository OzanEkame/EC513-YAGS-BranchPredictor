# EC513 — YAGS Branch Predictor Implementation in gem5

**Course:** EC513 Computer Architecture, Spring 2026  
**Instructor:** Ajay Joshi  
**Team:** Ozan Ekame, William Borland, Brian Quijada, Fadi Kidess

## Overview

This project implements the **YAGS (Yet Another Global Scheme)** branch predictor in gem5 (v25.1). YAGS is a hybrid predictor that reduces destructive aliasing in global branch predictors by maintaining separate "taken" and "not-taken" caches that store only the exceptions to a biased prediction. This allows the predictor to handle branches with strong biases efficiently while still capturing branches that deviate from their expected behavior.

## Branch Predictor Description

YAGS builds on the observation that most branches are heavily biased toward either taken or not-taken. Instead of storing predictions for all branches in a single global table (which causes aliasing), YAGS uses:

- A **direction cache** (choice predictor) indexed by branch address, which provides a biased prediction (taken or not-taken)
- A **T-cache** (taken cache) that stores branches whose actual outcome contradicts a "not-taken" bias
- An **NT-cache** (not-taken cache) that stores branches whose actual outcome contradicts a "taken" bias
- Tags in each cache to reduce aliasing

By only caching the exceptions, YAGS significantly reduces the number of entries needed and minimizes conflicts between branches.

## Repository Structure

```
├── src/cpu/pred/          # YAGS implementation files
│   ├── yags.cc            # YAGS predictor implementation
│   ├── yags.hh            # YAGS predictor header
│   └── BranchPredictor.py # SimObject configuration (updated)
├── configs/               # Simulation configuration scripts
├── output/                # Experimental results and stats
└── README.md
```

## Benchmarks

Evaluated on 5 SPEC CPU2017 benchmarks:

| Benchmark | Description |
|-----------|-------------|
| 505.mcf_r | Network flow optimization |
| 520.omnetpp_r | Discrete event simulation |
| 523.xalancbmk_r | XML processing |
| 502.gcc_r | GNU C compiler |
| 525.x264_r | Video compression |

**Simulation parameters:** 1 billion instruction warmup, 1 billion instruction measurement phase.

## Comparison

YAGS is compared against the following predictors from HW5:

- TournamentBP
- LTAGE
- MultiperspectivePerceptronTAGE64KB

Metrics reported: **IPC** (instructions per cycle) and **branch misprediction rate**.

## How to Run

```bash
# Source the SPEC environment
source /projectnb/ec513/materials/HW2/spec-2017/sourceme

# Navigate to gem5 directory
cd /path/to/gem5

# Build gem5
scons build/X86/gem5.opt -j$(nproc)

# Run a benchmark with YAGS predictor
build/X86/gem5.opt configs/run_YAGS.py \
    --image ../disk-image/spec-2017/spec-2017-image/spec-2017 \
    --partition 1 \
    --benchmark 505.mcf_r \
    --size ref
```

## References

- Eden, A. N., & Mudge, T. (1998). *The YAGS Branch Prediction Scheme.* MICRO-31.
