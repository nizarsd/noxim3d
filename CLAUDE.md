# Noxim3D — NoC for DNN

SystemC-based cycle-accurate Network-on-Chip simulator (fork of Noxim), extended for 3D NoC
research comparing routing/selection policies under DNN-style traffic.

## Research focus (Stage 1)

Comparing two selection policies layered on odd-even balanced routing:
- **DP**: distributed selection unit using a multi-hop congestion cost-to-go field.
- **bufferlevel (BL)**: local heuristic based on immediate neighbour buffer occupancy.

DP's advantage appears at/above the congestion knee; below it, DP ≈ BL (sometimes
marginally worse). Key finding from the size series (Z=3): **peak DP benefit scales
with mesh diameter within a parity class, and X/Y parity governs past-knee behaviour**
— odd×odd meshes win through saturation, even×even meshes reverse just past the knee.

**See [FINDINGS.md](FINDINGS.md) for the full DP-vs-BL results, method, and open items.**
(The older `stage1-log.txt` is superseded by FINDINGS.md.)

## Traffic injection (Stage 2, in progress)

DNN trace generation (Stage 2 of the execution plan) surfaced a format mismatch: the
project proposal assumed a per-packet flit-level trace format, but the actual traffic
input mechanism ([`TGlobalTrafficTable`](TGlobalTrafficTable.cpp)) is a **statistical**
descriptor table — rows of `src dst pir por t_on t_off t_period` — consumed via
per-cycle Bernoulli draws in `TProcessingElement::txProcess` against a cumulative PIR,
not literal per-packet replay. Built-in `TRAFFIC_RANDOM` (uniform destination, global
PIR, whole-run only — not phase-switchable) is also available as a distribution
alongside `TRAFFIC_TABLE_BASED` ([`NoximDefs.h`](NoximDefs.h)).

### Two hard limits of the table mechanism (code-as-written)

1. **Packet size is never read from the table.** `canShot()` calls
   `packet.make(local_id, dst, now, getRandomSize())` — the profiled per-layer output
   volume cannot be expressed at any `pir`/window setting. The format has no size column.
2. **One destination per source per cycle.** `getCumulativePirPor()` picks a single dst
   by weighted random draw among rows active that cycle, so a source cannot fan out to
   two dsts simultaneously (e.g. a ResNet skip connection firing to both the next
   sequential tile and the merge tile).

Timing itself is *not* a blocker: a narrow window (`t_on=C-1, t_off=C+1, pir=1.0`) forces
near-deterministic single-shot firing at cycle C, with `t_period` = inference-pass length
for repeats.

### Three candidate approaches

- **(a) Statistical approximation** — DNN traffic as ordinary table rows. Lossy (no real
  size, no fan-out), but **zero simulator changes**.
- **(b-lite) Table + fixed packet size** — keep the whole table mechanism; change
  `canShot()` to read a new size column instead of `getRandomSize()` (~5 lines + parser
  field). Carries real per-layer size. Still approximate: `pir`×Bernoulli means *packet
  count* and *exact cycle* within the window remain random (total volume is an
  expectation, not exact), size is quantised to **flits** (profiled `comm_bytes` ÷ flit
  size, integer-rounded), and the one-dst-per-cycle limit still applies.
- **(b-full) Dependency-gated dataflow firing** — per-PE state machine: track required
  inputs, fire only once all have *actually arrived* in-sim, so congestion delays
  propagate downstream. The architecturally correct model for a layer DAG, and the only
  option where routing improvements show up in true end-to-end inference latency. Large
  `TProcessingElement` change.

### Model-by-model fit

| Model | Fit under (b-lite) | Notes |
|-------|--------------------|-------|
| VGG-16 | Full fit | Purely sequential, one src→one dst per layer; FC burst is just high `pir` over a window. No structural gap. |
| ResNet-50 | Mostly fits | Skip-connection fan-out served by weighted draw, not guaranteed simultaneous — OK for volume, imperfect for synchronised burst. |
| Transformer | Poor fit | Attention is all-to-all/collective in one window; b-lite scatters it as pairwise traffic. Needs `TRAFFIC_RANDOM` or ring/tree collective rows — a separate modelling path. |

**(b-full) fixes the CNN cases (ResNet, VGG) exactly, but does *not* dissolve the
transformer problem** — attention is a collective, not a DAG dependency, so it still needs
explicit all-to-all edges (O(N²) rows) or a collective primitive either way.

### Current lean (not yet committed)

**(b-lite) for CNNs now** — unblocks Stage 2/3 for ResNet-50 + VGG-16 at ~5 lines of
change, consistent with Stage 2's "get something running" goal. **Defer (b-full)** until
Stage 5/6 evidence shows arrival-coupled timing is actually needed to demonstrate the RL
agent's benefit. **Transformer handled separately** as a synthetic pattern regardless of
which option is chosen.

Not yet implemented — logged here before Stage 2 code is written.

## Correctness & performance

- **odd-even-balanced + DP legality** (`a698e05`): DP's turn legality
  ([`can_turnOddEvenBalanced`](DPNode.cpp) + `*_DPStrict` helpers) mirrors the
  router's `routingOddEvenBalanced` but source-independent (DP has no packet-source
  state); correct 3D vertical exclusivity; falls back to 2D odd-even when `Z==1`.
- **DP perf optimizations** (this session, behaviour-preserving / bit-identical):
  gated unused NoP output, and cached the topology-static DP turn-legality.
- **Parallel sweeps:** [`noximrun_buffer_sweep_parallel.bash`](noximrun_buffer_sweep_parallel.bash)
  (`JOBS` knob; deterministic → identical to sequential).
- **Limit:** `DPSIZE = 260` caps mesh size; > 260 nodes overflow DP arrays (raise & rebuild).

**See [PERFORMANCE.md](PERFORMANCE.md) for profiling, fixes, and validation.**

## Build

```
# Edit Makefile.defs: set SYSTEMC to your systemc-2.3.3 install path
make
```

Produces the `noxim` binary. `.o` files and the binary are gitignored — rebuild locally.

## Key source files

- [TRouter.cpp](TRouter.cpp) / [TRouter.h](TRouter.h) — router, routing algorithms, selection policies
- [DPNode.cpp](DPNode.cpp) / [DPNode.h](DPNode.h) — DP cost-to-go computation unit
- [TNoC.cpp](TNoC.cpp) / [TNoC.h](TNoC.h) — top-level NoC topology/wiring
- [TGlobalStats.cpp](TGlobalStats.cpp) — delay/throughput/energy stats collection
- [TPower.cpp](TPower.cpp) — power modeling
- [TProcessingElement.cpp](TProcessingElement.cpp) — traffic generation/injection per node
- `TRouter_old_can_turn.cpp`, `TRouterTCandNormal.cpp` — earlier routing variants kept for reference

## Running experiments

- [noximrun.bash](noximrun.bash), [noximrun_buffer_sweep.bash](noximrun_buffer_sweep.bash) — main
  experiment sweep scripts (PIR sweep, buffer-size sweep)
- [noximrun_buffer_sweep_parallel.bash](noximrun_buffer_sweep_parallel.bash) — parallel sweep
  (set `JOBS`; env overrides `DIMX/DIMY/DIMZ`, `PIR_LIST`, `SEEDS`, `BUFFER_LIST`, `OUTDIR`)
- `traffics/` — synthetic traffic pattern definitions (transpose, ami25/49, mpeg, mms, tele, ...)
- Results land in `results_*/` directories (gitignored — regenerate rather than commit)
- **DP-aware timing** (auto-derived by the sweep scripts): `DP_CYCLE = 2·nodes·(diameter+3)`,
  `CINTERVAL = DP_CYCLE`, `WARMUP = 3·DP_CYCLE`, `SIM = 20·DP_CYCLE`. Workflow: coarse 1-seed
  knee-finder → 3-seed fine sweep at the knee. Knee PIR *drops* as the mesh grows.

## Git

- Repo-local identity is set (`user.name`/`user.email`), not global.
- `.gitignore` excludes build artifacts (`*.o`, `noxim` binary), `results_*/`, simulation dumps
  (`.ptrace`, `.steady`, `.init`, etc.), `.svn/`, and Windows Zone.Identifier files.
- Remote: `origin` → `https://github.com/nizarsd/noxim3d.git`.
