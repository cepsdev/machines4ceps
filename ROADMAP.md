# Roadmap

The organising thesis of this roadmap is the one ceps has been heading towards for a
while:

> Let the AI write the state machine, let the tooling verify it.

ceps is unusually well placed for that. It is a text format with a formal execution
semantics, partial programs are legal, and LLMs generate valid, rich ceps without
difficulty. The generation half of the loop already works.

**The verification half is the bottleneck, and the gap is not expressiveness — it is the
quality of what the tool reports back.** That observation sets the ordering below.

---

## Why the ordering is what it is

An automated consumer — an agent, a CI job, a test harness — needs four things from a
verification tool:

1. output it can **parse** without heuristics,
2. output that is **complete**, so a replay reconstructs the run,
3. output that is **honest**, so a green result means the thing was actually checked,
4. a way to tell **"the model is wrong"** apart from **"the tool is broken"**.

ceps currently falls short on all four, and the defects responsible are concentrated in
exactly that path:

| Defect | What an automated consumer sees |
|---|---|
| [D3](DEFECTS.md#d3) | the trace is missing its first line, unless the model opts into coverage |
| [D4](DEFECTS.md#d4) | the trace contains a microstep that never happened |
| [D7](DEFECTS.md#d7) | which transition fired is not reported at all, only the state delta |
| [D5](DEFECTS.md#d5) | `Transition Coverage: -nan ( -nan% )` |
| [D6](DEFECTS.md#d6) | coverage silently overstated — `SM`-endpoint edges are not counted |
| [D8](DEFECTS.md#d8) | `--report_format_json` is accepted and does nothing |

Six of the eleven known defects sit in the feedback channel. A human eyeballing one trace
does not notice; a machine iterating against it cannot function, because a wrong number
and a broken tool are indistinguishable.

Hence: **fix the instrument before running the experiment.** `c{}` makes ceps more
expressive, and ceps is already expressive. A trustworthy machine-readable contract makes
ceps *usable by machines*, which is the stated goal and the thing currently missing.

A second argument for this ordering: `c{}` is roughly 1200 LOC carrying a substantial
test burden, and those tests are read *through* the execution trace. Building it first
means validating new semantics with an instrument known to drop lines, invent lines, and
miscount coverage.

---

## Phase 1 — Make the output machine-readable

**Goal:** one versioned, structured report that any consumer can parse, covering trace,
coverage, diagnostics and status.

**Why first:** it is the interface everything later plugs into, and it is cheap. The
report is *already* a `ceps::ast::Nodeset` with a settled schema, and a Nodeset-to-JSON
serialiser already exists and is already exercised by the websocket API.

| Step | Work | Anchor |
|---|---|---|
| 1.1 | Implement `--report_format_json` as a branch beside the s-expression one, delegating to `ceps2json` | `state_machine_simulation_core.cpp:1863`; `api/websocket/ws_api.cpp:151` |
| 1.2 | Implement `--report_format_ceps`, or remove it and `--report_format_xml` | [D8](DEFECTS.md#d8) |
| 1.3 | Add a schema version field to the report root | — |
| 1.4 | Bring the execution trace into the same structured report, not just stdout | [D3](DEFECTS.md#d3), [D4](DEFECTS.md#d4) |
| 1.5 | Make diagnostics structured too — code, severity, source location, message | — |

**Acceptance:** a model can be run once and its trace, coverage, diagnostics and exit
status recovered from a single parse, with no screen-scraping and no locale-dependent
number formatting.

**Note on scope.** Step 1.1 is small. Steps 1.3–1.5 are where the design effort is, and
they are worth doing deliberately — this becomes a public interface, and changing it later
costs more than getting it right now. A short specification before implementation is
advisable, in the manner of `RECURSIVE-STATE-MACHINES.md`.

---

## Phase 2 — Make the trace complete and causal

**Goal:** a trace that can be replayed to reconstruct every intermediate configuration,
and that says which edge was taken.

| Step | Work | Defect |
|---|---|---|
| 2.1 | Decouple initial-entry logging from the coverage opt-in | [D3](DEFECTS.md#d3) |
| 2.2 | Eliminate the spurious trailing line in covering models | [D4](DEFECTS.md#d4) |
| 2.3 | Restore `log_triggered_transitions`, or delete it deliberately | [D7](DEFECTS.md#d7) |
| 2.4 | Decide whether intra-line ordering should be causal rather than index order | [D11](DEFECTS.md#d11) |
| 2.5 | Expose `log_verbosity` as a flag, or remove it and its guarded code | [D9](DEFECTS.md#d9) |

**Why 2.3 matters most.** Today a reader infers the edge from the state-set delta, and
that inference is ambiguous whenever two edges produce the same delta. Reporting the edge
directly turns the trace from a sequence of observations into a sequence of *causes* —
which is what makes a counterexample actionable, and what a test generator needs.

**Acceptance:** replaying the trace from the empty configuration reproduces the final
configuration exactly, and every step names the transition responsible.

---

## Phase 3 — Make coverage trustworthy

**Goal:** a coverage figure that can be used as a pass/fail gate or a search signal
without caveats.

| Step | Work | Defect |
|---|---|---|
| 3.1 | Redefine `*_coverage_defined` as "denominator > 0", fixing both the division and the print guard | [D5](DEFECTS.md#d5) |
| 3.2 | Decide the `SM`-endpoint exclusion, and reconcile the state and transition filters | [D6](DEFECTS.md#d6) |

**3.2 is a design decision, not a bug fix.** Excluding `INIT` and `FINAL` endpoints is
defensible. Excluding `SM` endpoints is the questionable one, and the two filters
currently disagree with each other — the test is live in the transition filter and
commented out in the state filter (`state_machine_simulation_core.cpp:2033`,
`:2072-2082`). It needs a ruling before phase 5, because under `c{}` every call edge
targets a machine and every return edge leaves a `Final` — so both kinds of edge
introduced by recursion would be precisely the edges not counted.

**Acceptance:** no `nan` reachable; a documented, consistent rule for what counts; and a
model whose recursive structure is untested cannot report full coverage.

---

## Phase 4 — Robustness for unattended use

**Goal:** ceps never crashes on malformed input, and always exits with a meaningful code.

| Step | Work | Defect |
|---|---|---|
| 4.1 | Fix the SIGSEGV on a covering machine without `Initial` | [D2](DEFECTS.md#d2) |
| 4.2 | Declare `bgt`, `bneq`, `callx` | [D1](DEFECTS.md#d1) |
| 4.3 | CI check that the assembler opcode table and `oblectamenta_decls.ceps` agree | [D1](DEFECTS.md#d1) |
| 4.4 | Audit for other unguarded descents of the same shape as 4.1 | — |
| 4.5 | State the id-resolution rule once in `SKILL.md`, resolving the 114/116 contradiction | [D10](DEFECTS.md#d10) |

4.2 is three lines and unblocks three working instructions; do it while in the area. 4.5
matters more than its severity suggests: an LLM generating ceps reads `SKILL.md` as its
specification, so a contradiction there propagates into generated models. 4.3
is what stops the two lists drifting apart again — they are two hand-maintained lists that
must agree and nothing currently checks that they do.

**Acceptance:** a fuzz pass over small malformed models produces diagnostics and exit
code 1, never a signal.

---

## Phase 5 — `c{}`: recursive state machines

Fully specified in **[RECURSIVE-STATE-MACHINES.md](RECURSIVE-STATE-MACHINES.md)**;
§15 carries a twelve-step implementation plan.

Deliberately placed last, for the reasons in *Why the ordering is what it is*. Two
prerequisites from earlier phases are hard:

- **Phase 3.2 must be settled**, or recursion will be invisible to coverage.
- **Phase 2 should be done**, because `c{}` is tested through the trace, and the
  depth-tagged trace format of §9 is an extension of it.

Open design question still outstanding: **E4** — whether a callee whose index interval is
split across cover classes should be rejected or renumbered ([D11](DEFECTS.md#d11)). The
specification currently rejects; that is a placeholder, not a decision.

---

## Phase 6 — The ecosystem play

**Goal:** an AI agent can drive ceps natively — write a model, run it, read a structured
verdict, revise.

Two routes, and they are not alternatives but a sequence:

**6a — a thin adapter over the phase-1 contract.** Once the report is stable JSON, an MCP
server that exposes *validate*, *simulate*, *coverage* and *explain* is a small program
against a fixed interface. Days of work, and it is what makes the thesis demonstrable.

**6b — `mcp/main.ceps`, the MCP server written in ceps itself.** The dogfooding endgame,
and a genuinely strong statement: the specification language hosts the service that serves
specifications. It needs JSON parsing, stdio framing and a VM solid enough to carry them —
so it graduates from 6a rather than replacing it.

Starting at 6b is the romantic choice and the slow one. Starting at 6a makes the thesis
real now and leaves 6b as the goal it was always going to be.

---

## Summary of sequencing

```
Phase 1  machine-readable report  ──┐
Phase 2  complete, causal trace   ──┼──> Phase 5  c{} recursive state machines
Phase 3  trustworthy coverage     ──┘
Phase 4  robustness (independent, do opportunistically)
Phase 6  MCP: 6a thin adapter ──> 6b ceps-native
```

Phases 1–3 are each small, and together they convert ceps from a tool that prints things
into a tool that can be *called*. Phase 5 is the expressiveness leap, and it lands on
solid ground once the instrument is trustworthy.

---

## Documents

| Document | Contents |
|---|---|
| [DEFECTS.md](DEFECTS.md) | 11 defects, each with a minimal reproducer |
| [RECURSIVE-STATE-MACHINES.md](RECURSIVE-STATE-MACHINES.md) | the `c{}` specification |
| [SKILL.md](SKILL.md) | the language and tool reference |
| [QUICK-START-UML-WITH-CEPS.md](QUICK-START-UML-WITH-CEPS.md) | UML-oriented introduction |
