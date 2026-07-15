# Noxim3D — Stage 2: DNN Traffic Characterization & Representation Decision

Design/decision doc for Stage 2. **No code yet** — this fixes the modeling choices
before the converter is written. Companion to [FINDINGS.md](FINDINGS.md) (Stage 1
DP-vs-BL results) and [PERFORMANCE.md](PERFORMANCE.md).

**Bottom line up front:** represent DNN traffic as **DNN-derived statistical table
rows with phase windows** (option **a**, extended to use `t_on/t_off/t_period`),
*not* a new packet-replay mode (option b). Target a **Transformer encoder block**
as the primary characterization subject, with a small CNN as a contrast/sanity case.
Rationale below.

---

## 1. Scope

"Traffic characterization" = turn a DNN's inference (or training-step) communication
into something Noxim can inject, and decide *how faithfully* to encode it. Three
sub-problems, only the third of which is decided here:

1. **Profile** — per-layer tensor volumes + the layer→layer dependency graph
   (PyTorch forward/backward hooks; deterministic, exact bytes per edge).
2. **Map** — assign layers/tiles to X×Y×Z mesh node IDs. The load-bearing modeling
   choice; deferred to the converter design, but constrained by `DPSIZE = 260` nodes.
3. **Represent** — encode the mapped traffic in a form the simulator consumes.
   **This doc decides #3** and recommends the subject model for #1.

---

## 2. The constraint envelope: what Noxim actually consumes

From the injection path ([`TProcessingElement::canShot`](TProcessingElement.cpp:134)
→ [`TGlobalTrafficTable::getCumulativePirPor`](TGlobalTrafficTable.cpp:101)):

- **Injection is statistical, memoryless.** Each node, each cycle, draws
  `rand()/RAND_MAX < threshold`. `threshold` is the *cumulative PIR* over all table
  rows for which the node is the source and the current cycle is inside the flow's
  active window. A hit picks the destination by the cumulative-probability ladder.
  Inter-arrival is geometric → Poisson-like. **There is no notion of "send this exact
  tensor now" — only "inject toward dst at average rate pir".**
- **A table row is already phase-capable.** Row format
  `src dst pir por t_on t_off t_period`; the gate is
  `r = ccycle % t_period; on iff t_on < r < t_off`
  ([TGlobalTrafficTable.cpp:115](TGlobalTrafficTable.cpp:115)). So a flow can be
  windowed and periodic. **The existing `traffics/*.txt` don't use this** — they set
  `t_on=0, t_off=10000, t_period=10001` (always-on). Phase structure is available and
  currently unexploited. (The [CLAUDE.md](CLAUDE.md) "whole-run only" caveat applies
  to `TRAFFIC_RANDOM`, not to the table.)
- **Packet size is decoupled from the flow.** `packet.make(..., getRandomSize())` —
  size is a uniform draw between `min/max_packet_size`, *not* derived from the tensor
  being modeled. Tensor volume must therefore be encoded as **rate**, not as size,
  unless the converter also emits/controls size.
- **Open-loop.** Injection never reacts to congestion (only to the static `t_*`
  windows and PE throttling). No node ever *waits for data to arrive* before sending.

So the expressive vocabulary is: **{per-(src,dst) average rate} × {piecewise-constant
periodic on/off windows} × {random packet size}, injected as independent Bernoulli
trials.**

---

## 3. Characterization: what DNN traffic looks like

**Spatial (who talks to whom).** Entirely determined by the layer→node mapping. Two
regimes:
- *One layer per node* → a near-linear dependency chain (node i → i+1). Sparse,
  almost no path contention. Uninteresting for a congestion study.
- *Tiled / model-parallel* (a layer spread over many nodes) → the inter-layer
  transfer becomes a structured **many-to-many** exchange (scatter/reduce of a matmul).
  This is where NoC congestion, hotspots, and path diversity actually appear — i.e.
  exactly the regime in which Stage 1 found DP's advantage (diameter- and
  parity-governed, [FINDINGS.md](FINDINGS.md)).
  - **CNN**: activation halos + channel-reduction → mostly structured/neighbor +
    reduction traffic. Congestion only shows up under tiling.
  - **Transformer block**: QKV projections, the attention matrix (all-to-all across
    the sequence/head tiling), and the output projection give genuinely dense
    many-to-many exchange — the richest congestion stimulus of the common models.

**Temporal (when).** DNN communication is **strongly phased and bursty**:
- A tiled layer computes, then transmits its whole output in a **burst**, then a
  dependency barrier, then the next layer. Bursts are *deterministic and correlated*
  (a tensor moves as a contiguous block), not memoryless.
- Across a batch/pipeline, several layers can be active at once (pipeline parallelism)
  — softening the strict barrier into overlapping phase windows.
- Net: highly **non-stationary**, with sharp on/off structure at layer granularity.

---

## 4. Expressiveness gap — table vs DNN

| DNN traffic property | Statistical table (a) | Faithful for our question? |
|---|---|---|
| src×dst volume matrix (spatial hotspots) | ✅ one row per non-zero edge, `pir ∝ volume` | **Yes** — the spatial congestion structure is preserved exactly |
| Load level / where the knee sits | ✅ scale all `pir` by a global factor → sweepable like the Stage-1 PIR sweeps | **Yes**, and *better* — keeps the reproducible sweep methodology |
| Phase / layer sequencing | ⚠️ approximable via `t_on/t_off/t_period` windows (coarse, piecewise-constant) | **Mostly** — captures "flow X hot during phase P", not exact edges |
| Burst *shape* (contiguous block vs elevated-probability window) | ❌ Bernoulli smears a burst into a Poisson stream at elevated rate | **Lossy** — but see §5: this loss is largely irrelevant to a congestion metric |
| Causal dependency (L+1 waits for L) | ❌ flows fire independently on a static clock | **Lossy for both a and b** — see §5 |
| Exact packet ordering / per-packet timing | ❌ | Not needed for aggregate delay/throughput |
| Tensor→packet size | ⚠️ random size unless converter controls it | Fixable in the converter (encode volume as rate; optionally fix size) |

---

## 5. The decisive argument: replay fidelity is illusory under congestion

Option (b) — a packet-replay `TRAFFIC_*` mode reading pre-recorded
`(node, t, dst, size)` events — *looks* more faithful, but the fidelity it adds is
mostly **not usable for this study**:

- The recorded timestamps come from *one particular execution* on *some* hardware.
  Replaying them **open-loop** fires events at those fixed times regardless of the
  simulated network's congestion. Under load, the simulated NoC's real timeline
  diverges from the recorded one, so the "faithful" timestamps are already wrong — you
  get a deterministic-but-arbitrary schedule, not the true reactive DNN behavior.
- True fidelity would require **dependency-gated replay** (node emits L+1's tensor
  only after L's data *arrives in simulation*) — i.e. a closed-loop execution model.
  That is a much larger change (event dependency graph, per-node barriers, credit of
  arrivals back to the traffic generator) and a different research contribution than
  "DP vs BL selection." Neither (a) nor naive (b) captures it.
- Replay would also **break the Stage-1 timing discipline.** The DP-aware
  `WARMUP/SIM/CINTERVAL` are derived from mesh geometry as a *steady-state* measurement
  window ([FINDINGS.md](FINDINGS.md) §method). A finite trace has no steady state and
  no natural warmup; every result would need a new, trace-specific justification, and
  cross-mesh comparison (the whole Stage-1 axis) gets muddier.

Since the research metric is **aggregate delay/throughput of DP vs BL near the
congestion knee**, what must be faithful is the **spatial congestion structure and the
load level** — both of which the table reproduces exactly — plus *coarse* phase
structure, which the windows approximate. Burst micro-shape and exact timing wash out
in the aggregate. So (b) pays real engineering + methodological cost for fidelity the
metric can't see.

---

## 6. Options & cost

| | (a) DNN-derived statistical table + windows | (b) packet-replay mode |
|---|---|---|
| Simulator change | **none** (converter is external Python) | new `TRAFFIC_*`, event reader, size handling, timing redesign |
| Reproducible load sweep | ✅ scale `pir` (reuses Stage-1 harness) | ✗ trace is a fixed operating point |
| Fits DP-aware timing | ✅ steady-state, as-is | ✗ needs per-trace warmup story |
| Spatial hotspots | ✅ exact | ✅ exact |
| Phase structure | ⚠️ coarse (windows) | ✅ fine (but open-loop, see §5) |
| Causal dependency | ✗ | ✗ (unless closed-loop — big lift) |
| Risk to Stage-1 comparability | low | high |

---

## 7. Recommendation

**Adopt option (a): a DNN-derived statistical traffic table that uses the
`t_on/t_off/t_period` windows.** Concretely, the (future) converter should:

1. Profile the model → per-edge tensor bytes + a coarse phase index per edge.
2. Map layers/tiles → node IDs (≤ 260 nodes).
3. Emit one row per non-zero `(src,dst)` edge with `pir ∝ bytes / phase_duration`,
   and `t_on/t_off/t_period` set from the edge's active phase window.
4. Expose a **global load scale** so the DP-vs-BL knee can be swept exactly as in
   Stage 1 — the DNN sets the *shape* (spatial pattern + phases), the scale sets the
   *level*.
5. Document it explicitly as a **piecewise-constant, memoryless approximation** of the
   DNN trace (this doc is that disclosure).

Keep option (b) as **future work**, and note that its only scientifically meaningful
form is *dependency-gated closed-loop replay* with an end-to-end **makespan/latency**
metric — a distinct study from the current selection-policy comparison.

**Recommended subject model: a Transformer encoder block** (small BERT/ViT block) as
primary — its attention gives the dense many-to-many exchange that produces the
congestion depth and path diversity the Stage-1 mechanism (diameter × parity) is about,
so it's the model most likely to *exercise* the DP-vs-BL difference. Add a **small CNN**
(a few conv layers, tiled) as a contrast case — sparser, more structured traffic — to
show whether the parity/diameter story generalizes across traffic shapes. Skip full
networks initially; a single representative block already produces a clean, mappable
src×dst matrix and keeps the node count under `DPSIZE`.

---

## 8. Choosing the routing algorithm for DNN traffic — read before deciding ⚠

A routing-variant side study ([FINDINGS.md](FINDINGS.md) → "Routing-variant study")
found that OEB **turn exclusivity trades off directly against DP's benefit**, and this
governs which routing to run under ResNet50 / VGG16 / the transformer (BERT-base) traffic:

- Lowest-coupling routing (**modified2**, both branches exclusive) has the **best absolute
  latency/throughput** but the **worst DP-vs-BL gap** — it strips path diversity, so DP goes
  *negative*. The fastest routing is the **worst substrate for the DP study**.
- Higher path diversity (baseline, or more-adaptive routings) gives DP more to exploit but
  lower saturation throughput.

**So decide the target metric FIRST:** absolute latency/throughput (favours modified2) vs the
DP-vs-BL advantage that is the Stage-1 story (favours diversity). Do **not** assume the fastest
routing is the right one. Also: modified2's funnel-then-shaft restriction may hurt **bursty /
phased** DNN traffic (the very structure Stage 2 introduces) and Z-heavy or hotspot-concentrating
patterns. Only measured on transpose1 4×4×3 so far — **sweep each DNN pattern × mesh to its own
knee**, reporting absolute delay *and* DP-vs-BL, before committing.

## 9. Open items / what to build next

- **Converter (Stage 2 code)** implementing §7.1–7.5: `profile → map → table`.
- **Mapping policy** — decide layer/tile → node assignment (the deferred #2). Options:
  linear, tile-blocked, or congestion-aware; this dominates the spatial pattern and
  deserves its own short note.
- **Validation** — inject a known toy DNN, confirm the resulting table's steady-state
  hotspot map matches the profiled volume matrix, then run one DP-vs-BL sweep to see
  whether the knee/parity behavior from synthetic traffic ([FINDINGS.md](FINDINGS.md))
  persists under DNN-shaped traffic.
- **Packet size** — decide whether the converter also fixes packet size per flow
  (tensor-accurate) or leaves `getRandomSize()`; affects burst granularity.
