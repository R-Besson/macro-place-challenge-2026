# Macro Placement with a Hole-Based Packing Legalizer

My entry (**"Besson-PLR"**) to the [Partcl / Hudson River Trading Macro Placement Challenge 2026](https://github.com/partcleda/macro-place-challenge-2026). This repository is a fork of the official challenge repository. **My code is in [`submissions/GeometricLegalizer/`](submissions/GeometricLegalizer/)**. Everything else (the evaluator, benchmarks and baselines) belongs to the challenge organizers. Their original README is in [CHALLENGE.md](CHALLENGE.md).

## The problem

Macro placement means positioning large fixed-size blocks (SRAMs, IP blocks) on a chip canvas. Hard macros must not overlap at all, and the placement should keep wirelength, density and routing congestion low. Submissions are ranked by the average **proxy cost** over 17 IBM benchmarks (lower is better):

```
Proxy Cost = 1.0 × Wirelength + 0.5 × Density + 0.5 × Congestion
```

## Approach

The idea is to treat **legalization as a packing problem**. A global placer decides roughly where each macro should go. Then my [hole-based strip packing algorithm](https://github.com/R-Besson/strip-packing-cpp-rewritten), adapted to aim for target positions, places the macros so that none overlap and each one stays as close as possible to its target.

1. **Force-directed global placement** ([`FD.py`](submissions/GeometricLegalizer/FD.py)). Each hard macro is pulled toward the centroid of the pins it connects to, with a weak pull toward the canvas center. Overlapping macros push each other apart, and that repulsion grows over 400 iterations.
2. **Packing legalizer** (C++, [`src/packer.cpp`](submissions/GeometricLegalizer/src/packer.cpp), exposed to Python through pybind11). Macros are sorted by descending area, given a small halo, and placed one at a time into the free-space "hole" that lets each one sit closest to its force-directed target. The holes are updated after every placement, exactly as in the strip packer, which guarantees zero overlaps.
3. **Refinement**. A greedy pass moves macros locally when that lowers their half-perimeter wirelength (HPWL). Soft macros are then moved, over several rounds, to the average centroid of the nets they belong to.

[`placer.py`](submissions/GeometricLegalizer/placer.py) is a simpler variant that skips steps 1 and 3 and legalizes the initial positions directly.

## Results

The leaderboard entry was the force-directed version (`FD.py`), submitted on May 21.

| Placer | Avg proxy cost (17 IBM benchmarks) | Overlaps | Runtime |
| --- | --- | --- | --- |
| Greedy row placer (example) | 2.2109 | 0 | — |
| Simulated annealing (baseline) | 2.1251 | 0 | — |
| **Besson-PLR (this entry)** | **1.6186** | **0** | **~7.3 s per benchmark** |
| RePlAce (baseline) | 1.4578 | 0 | — |

Rank: 109th on the leaderboard. The entry has zero overlaps on every benchmark, a proxy cost about 24% lower than the simulated-annealing baseline, and a runtime of seconds per benchmark, where most top entries take minutes to an hour. It does not beat RePlAce. The leading entries reach about 0.97.

## Build & run

Follow the challenge setup in [SETUP.md](SETUP.md), then build the C++ legalizer and evaluate:

```bash
cd submissions/GeometricLegalizer
python setup.py build_ext --inplace   # builds the macro_packer extension (needs pybind11)
cd ../..
uv run evaluate submissions/GeometricLegalizer/FD.py --all
```

## Possible improvements

- Use an analytical global placer (like RePlAce) in place of the simple force-directed step, and keep the packing legalizer.
- Make the legalizer take connectivity into account when it chooses a hole, not only distance to the target.
- Tune the halo size and the macro ordering for each benchmark.