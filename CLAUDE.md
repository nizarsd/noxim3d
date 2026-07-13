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

**Open design question:** profiled DNN traffic (PyTorch-hook output) is a concrete
sequence of per-packet events (exact src/dst/timing per layer), which the statistical
table can only approximate (piecewise-constant PIR over `t_on`/`t_off` windows). Before
writing the Stage 2 converter, decide between:
  (a) approximate DNN traces as statistical table rows (lossy, no simulator changes), or
  (b) add a new trace-replay traffic mode to Noxim (e.g. a new `TRAFFIC_*` distribution
      that reads an explicit per-node event list) to preserve trace fidelity.

Not yet decided or implemented — logged here before Stage 2 code is written.

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
