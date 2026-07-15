# Noxim3D — Research Findings: DP vs bufferlevel

Rolling record of the DP-vs-BL selection study on odd-even-balanced routing.
Engineering/perf notes are in [PERFORMANCE.md](PERFORMANCE.md).

Fixed conditions unless stated: routing `oddevenbalanced`, traffic `transpose1`,
Poisson injection, buffer 16, seeds {2, 6, 10}.

---

## Experimental method

- **DP-aware timing** (derived from mesh, single source of truth in
  [NoximDefs.h](NoximDefs.h)):
  - `DP_DWELL = diameter + 3`, `diameter = (X-1)+(Y-1)+(Z-1)`
  - `DP_CYCLE = 2 · nodes · DP_DWELL` (converge + settle)
  - `CINTERVAL = DP_CYCLE` (congestion cost recomputed once per DP reconfiguration)
  - `WARMUP = 3 · DP_CYCLE`, `SIM = 20 · DP_CYCLE`
  - Rationale: warmup must cover DP field warm-up (~1 DP_CYCLE) + network fill;
    3× is comfortably above the real settling time (measured ~2–3× for these sizes).
- **Workflow:** coarse 1-seed knee-finder → 3-seed fine sweep centred on the knee.
- **Caveat learned the hard way:** a coarse grid can *step over a sharp peak*.
  It agreed with the fine sweep for 8×8×3 (peak near a coarse point) but badly
  under-reported 4×4×3 (peak at 0.036 fell between coarse points 0.035/0.040,
  giving +10.9% instead of the true +33%). Always fine-sweep before trusting a peak.
- **NaN gotcha (fixed):** before the odd-even-balanced fix, NaN delay/throughput
  meant a *routing stall*, not a warmup problem — the buggy `routingOddEven1()`
  stalled the network so no packets were received in the measured interval →
  divide-by-zero. Long warmup only exposed it earlier. See
  [PERFORMANCE.md](PERFORMANCE.md) §1. If NaN reappears, suspect routing/deadlock,
  not the timing.
- **DP convergence check:** the cost-to-go field reaches exact hop-distance costs
  (100·hops + accumulated buffer-occupancy congestion) within `hop_distance`
  cycles and holds stable through the dwell window — verified via `-DDP_DEBUG`.

## Reproduction

Runner: [`noximrun_buffer_sweep_parallel.bash`](noximrun_buffer_sweep_parallel.bash)
(or the sequential [`noximrun_buffer_sweep.bash`](noximrun_buffer_sweep.bash)).
Timing is auto-derived from the mesh; everything not set below is left at Noxim
defaults. Invocation template:

```
DIMX=<X> DIMY=<Y> DIMZ=3 \
PIR_LIST="<grid>" \
SEEDS="2 6 10" \                 # coarse knee-finder for 8x8x3 used SEEDS="2"
BUFFER_LIST="16" \
WARMUP_DP_CYCLES=3 SIM_DP_CYCLES=20 \
ROUTING=oddevenbalanced TRAFFIC=transpose1 \
OUTDIR=results_knee_<mesh> JOBS=8 \
bash noximrun_buffer_sweep_parallel.bash
```

Read `OUTDIR/summary_compare.csv` for the DP-vs-BL means (delay reduction %).

Per-mesh PIR grids actually used (coarse locates the knee; the fine sweep gives
the tabulated peak):

| mesh | coarse knee-finder | fine sweep (3 seeds) |
|------|--------------------|----------------------|
| 4×4×3 | `0.030 0.035 0.040 0.045 0.050 0.055 0.060` | `0.036 0.037 0.038 0.039 0.041 0.042 0.043 0.044` (reuses coarse 0.035/0.040/0.045) |
| 5×5×3 | — (swept directly across the knee) | `0.020 0.022 0.024 0.025 0.026 0.028 0.030` |
| 6×6×3 | `0.012 0.015 0.018 0.020 0.022 0.025 0.028 0.030` | `0.018 0.019 0.020 0.021 0.022 0.024 0.026` |
| 7×7×3 | `0.010 0.013 0.015 0.017 0.019 0.021 0.024` | *(not yet run — see Open items)* |
| 8×8×3 | `0.008 0.010 0.012 0.014 0.016 0.018 0.020` | `0.012 0.013 0.014 0.015 0.016 0.017 0.018` |

Seeds: all sweeps used `SEEDS="2 6 10"` **except** the 8×8×3 coarse knee-finder,
which used `SEEDS="2"` (single seed — the 1-seed knee-finder was only needed once
the meshes got large enough to be slow). The 7×7×3 peak is therefore coarse-grid
(3-seed) and still wants a fine sweep to confirm.

## Headline result — size × parity (Z = 3 series)

Peak = best DP delay reduction vs BL near the knee. "Past-knee" = behaviour once
past the congestion knee into saturation.

| mesh | X/Y parity | diameter | knee (PIR) | peak DP benefit | quality | past-knee |
|------|-----------|----------|-----------|-----------------|---------|-----------|
| 4×4×3 | even | 8  | ~0.036 | **+33.1%** @0.036 | fine | reverses (~0.042) |
| 5×5×3 | odd  | 10 | ~0.025 | **+64%**   @0.022–0.025 | fine | wins through |
| 6×6×3 | even | 12 | ~0.020 | **+68.2%** @0.021 | fine | reverses (~0.024) |
| 7×7×3 | odd  | 14 | ~0.014 | **+82.4%** @0.015 | coarse | wins wide (to ~0.021) |
| 8×8×3 | even | 16 | ~0.014 | **+75.8%** @0.014 | fine (3-seed) | reverses (~0.018) |

### Two robust conclusions

1. **Peak DP benefit scales with size/diameter within a parity class.**
   - Even series: +33% (d8) → +68% (d12) → +76% (d16).
   - Odd series: +64% (d10) → +82% (d14).
   - Odd meshes also sit *above* the even trend at comparable diameter (a parity
     boost): odd d14 (+82%) tops even d16 (+76%).

2. **X/Y parity governs past-knee behaviour.**
   - **Odd × odd** meshes (5, 7) sustain DP's win over a *wide* band past the knee.
   - **Even × even** meshes (4, 6, 8) reverse to a DP *loss* just past the knee,
     then converge back toward tied in deep saturation (both policies overloaded).

### Why (mechanism, partly hypothesis)

DP's value is global, multi-hop congestion awareness; BL sees only one hop.
That value grows with (a) path length / diameter, (b) congestion depth, and
(c) path diversity. Larger meshes have more of all three → bigger peak. The
odd/even split is a property of the **odd-even-balanced routing**, whose turn
legality branches on coordinate parity — odd×odd geometries give DP more usable
path diversity past the knee, so it keeps winning; even geometries run out of
alternative paths in saturation and DP's rerouting churn then hurts.

*Confounds controlled:* the size series holds Z = 3 and X = Y, isolating X/Y
parity. The apparent "5×5×3 is an outlier / size doesn't scale" seen early on was
an artifact of comparing a coarse 6×6×3 peak to a fine 5×5×3 peak — it dissolved
once both had fine data.

## Below the knee

At low PIR both policies behave similarly; DP can be marginally *worse* (its
coarser per-cycle direction updates cost a few cycles of latency with no
congestion to route around). DP only pays off from the knee onward.

## Vertical scaling (Z series)

Does increasing **Z** (vertical depth) help DP the way increasing X/Y does? Test:
fix X=Y=5 (odd), raise Z. 5×5×5 has diameter 12 — the *same* diameter as planar 6×6×3.

| mesh | diam | Z parity | peak DP benefit | quality |
|------|------|----------|-----------------|---------|
| 5×5×5 | 12 | odd | **+70.6%** @0.014 | fine, 5-seed |

- **Vertical ≈ planar for DP's peak:** 5×5×5 (+70.6%) ≈ 6×6×3 (+68.2%) at equal
  diameter — diameter drives the peak regardless of *which* dimension supplies it
  (diameter hypothesis in dimension-agnostic form). Node counts differ (125 vs 108),
  so the cross-comparison is interpretive; the 5×5×Z series itself is the clean control.
- **Sharp-peak Z signature (lesson):** the 5×5×5 knee peak is very narrow. The coarse
  1-seed grid (0.012/0.015) stepped over it (read +17% then ~0%) and *mis*-suggested
  "Z suppresses DP"; only the fine 0.001-spaced 5-seed sweep revealed +70.6% at 0.014.
  Vertical congestion transitions abruptly — always fine-sweep in Z.
- Past-knee behaviour of 5×5×5 (odd X/Y) not yet fully mapped — open.

## DP settle window (`-dpsettle`) — less settle is better

`dp_settle` is the idle "hold" phase where DP routes on a frozen converged field
before re-probing/reconfiguring. Now runtime-tunable: **`-dpsettle N`** →
settle = N·dp_pass, dp_cycle = (1+N)·dp_pass (align `-cinterval` to dp_cycle).
**Default is now 0.**

DP-vs-BL delay reduction at the knee, by settle multiple:

| settle k | 3×3×3 @0.05 | 5×5×5 @0.014 |
|----------|-------------|-------------|
| **0** | **+28.8%** | **+74.0%** |
| 1 (old default) | +16.3% | +72.1% |
| 2 | +14.2% | — |
| 3 | −30.0% | — |
| 4 | −42.9% | — |

- **`settle=0` (continuous reconvergence) is best-or-tied everywhere tested; more
  settle only degrades DP** (it routes on an increasingly stale field).
- **Benefit is size-dependent:** small/fast meshes gain a lot (3×3×3 ~doubles the
  reduction), large meshes marginal (5×5×5 +1.9pp). Freshness matters most when
  congestion mixes fast relative to the converge sweep.
- **Implication:** the size/parity/Z results above (all run at settle=1) *understate*
  DP, most at the small end. Large-mesh trends move <2pp, so the parity/Z conclusions
  still hold — the small end just nudges up.
- **Not `CINTERVAL`:** that sets the congestion-sampling period (`cost_to_go`), not the
  reconfigure cadence. Settle lives in the compiled `dp_cycle()/dp_pass()/dp_settle()`
  ([NoximDefs.h](NoximDefs.h)); see [PERFORMANCE.md](PERFORMANCE.md).

## Routing-variant study: turn exclusivity ↔ saturation throughput

Three `oddevenbalanced` (OEB) variants tested, **all deadlock-free** (each is a turn-set
subset/superset of the published baseline), differing only in vertical↔planar turn
**exclusivity** in [`routingOddEvenBalanced`](TRouter.cpp):

| variant | up branch | down branch | planar/vertical coupling |
|---------|-----------|-------------|--------------------------|
| **baseline** (published) | exclusive | coexisting | medium (asymmetric) |
| **modified** (UP-always) | coexisting | coexisting | **max** |
| **modified2** | exclusive | exclusive | **min** |

**Coupling governs saturation throughput, monotonically.** transpose1, 4×4×3, PIR 0.035,
settle=0, absolute **BL delay: modified 290 > baseline 85 > modified2 23** (DP: 135 / 69 / 25).
Less planar↔vertical coexistence ⇒ less flow coupling ⇒ shallower back-pressure trees ⇒ knee
pushed to higher PIR. modified collapses the knee (11–24× worse than baseline on random @0.041);
modified2 pushes it far out (delay still 102 at 0.045 where baseline is 528, modified 1753).

**DP-vs-BL % is misleading in isolation.** modified's headline "+53% DP reduction" came from
degrading BL faster than DP — *both* were 2–3× slower than baseline absolutely. modified2 shows
*negative* DP-vs-BL at these PIRs because it is now **below-knee** there. Lesson: quote **absolute
delay** and re-locate each routing's **own knee** before trusting a DP reduction %.

**Tension — best routing ≠ best DP substrate.** modified2 (min coupling) is the fastest routing
but strips the path diversity DP exploits, so DP has little to optimize (went negative). "Fastest
routing" and "largest DP-vs-BL gap" diverge — directly relevant to choosing routing for DNN
traffic ([STAGE2.md](STAGE2.md)).

**Scope:** measured on **4×4×3 + transpose1 only** (random run lost to a script typo). OEB is
parity/size dependent (above), so generality is unverified. Binaries on disk (gitignored):
`noxim`=modified2, `noxim_ref`=modified, `noxim_base`=baseline. The DP legality mirror
`can_turnOddEvenBalanced` ([DPNode.cpp](DPNode.cpp)) matches the router modulo the documented
source-independence terms (`cz==sz`/`c0==s0`/`c1==s1` dropped/proxied).

### modified2 across sizes — knee shifts right, DP benefit is size-dependent

Knee sweep of modified2 vs the FINDINGS baseline (transpose1, buffer 16, seeds {2,6,10},
`-dpsettle 1`, DP-aware timing per mesh). modified2's knee moves to **higher PIR** on every mesh,
and its DP-vs-BL peak is **size-dependent** (partial — 8×8×3 / 5×5×5 pending):

| mesh | baseline peak | modified2 peak (settle=1) | note |
|------|--------------|---------------------------|------|
| 4×4×3 | +33.1% @0.036 | **negative everywhere** | DP benefit killed (mesh too small) |
| 5×5×3 | +64% @0.024 | +64.5% @0.030 | preserved, knee shifted right |
| 6×6×3 | +68.2% @0.021 | ≥+20% @0.026 (undersampled) | grid gap 0.026–0.032 misses the peak |
| 7×7×3 | +82.4% @0.015 | +74.4% @0.021 | mostly preserved, knee shifted |

**"modified2 kills DP's benefit" (from 4×4×3 alone) does NOT generalize** — it holds only on the
*smallest* mesh; on larger meshes DP's win is preserved, just relocated to a higher knee. Good for
DNN traffic (large meshes). Caveat: the extended PIR grid jumped past the shifted knee (gap
~0.026–0.032), so mid-size peaks here are **lower bounds** pending infill.

### settle under modified2 (5×5×3) — settle=0 best, but match the window

Clean settle comparison, **matched sim window** (sim=39000 for both; cinterval aligned per settle:
settle=0→975, settle=1→1950):

| | peak red% @0.030 | BL delay | DP delay |
|---|---|---|---|
| settle=1 | +64.5% | 343.8 | 121.9 |
| **settle=0** | **+71.5%** | 343.8 (identical) | **98.0** |

**settle=0 wins by ~7 pp** — BL is identical (bufferlevel ignores settle); settle=0's DP delay is
~20% lower (fresher congestion field from more frequent reconfiguration). Confirms "settle=0 best"
for modified2. **Methodology caveat:** compare settles over the **same sim window** — settle=0's
FINDINGS-native timing gives it *half* the sim (dp_cycle halves), deflating its BL and fabricating
a false settle=1 "win" (the raw native-timing numbers showed +44% vs +64%, an artifact).
modified2+settle=0 (+71.5%) also **exceeds** the baseline 5×5×3 peak (+64%, settle=1) — a genuine
DP gain on this mesh. Consequence: the settle=1 knee sweep above **understates** modified2 by
~5–7 pp/mesh.

## Open items

- **Routing-variant generality** — modified2 (both-exclusive OEB) only tested on transpose1
  4×4×3; sweep {random, transpose, hotspot, bit-reversal} × {odd/even, Z-heavy meshes} to each
  routing's **own knee**, reporting absolute delay **and** DP-vs-BL, before adopting a routing
  for DNN traffic. Decide the target metric first (latency/throughput vs DP gap).
- **Finish the modified2 knee sweep** — 8×8×3 + 5×5×5 pending; re-run at **settle=0** (true
  numbers, ~5–7 pp higher) and **infill PIR 0.026–0.032** to pin the mid-size peaks (6×6×3, 7×7×3).
- **7×7×3 peak is coarse-only** (+82.4%) — fine-sweep 0.013–0.016 to confirm.
- **Deep-saturation reversal point of odd meshes** — 5×5×3 only swept to 1.2×
  knee; push further to locate where it finally reverses.
- **Isolate DP-vs-routing:** re-run one odd/even pair under a parity-agnostic
  routing (e.g. `fullyadaptive`) to confirm the parity split is routing-induced.
- **Mechanism test:** instrument per-link load-balance (coefficient of variation)
  to directly test the path-diversity explanation for the even-mesh reversal.

## Log

- Size × parity (Z=3): 4×4×3, 5×5×3, 6×6×3, 7×7×3, 8×8×3.
- Z series: 5×5×5 (diameter 12, vs planar 6×6×3).
- Settle sweeps: 3×3×3 and 5×5×5 at the knee (k=0..4) → `settle=0` best; default set to 0.
- Raw CSVs in `results_knee_*` (gitignored — regenerate). Peaks/knees as tabulated above.
