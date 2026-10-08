# Roadmap

What to build, in what order. **Why** any of it is worth building — and what ceps should
refuse to build — is in **[POSITIONING.md](POSITIONING.md)**; this document assumes it.

The organising thesis of this roadmap is the one ceps has been heading towards for a
while:

> Let the AI write the state machine, let the tooling verify it.

ceps is unusually well placed for that. It is a text format with a formal execution
semantics, partial programs are legal, and LLMs generate valid, rich ceps without
difficulty. The generation half of the loop already works.

One caveat on that, recorded as [D12](DEFECTS.md#d12): an agent generates *state machines*
well, because state machines are what `SKILL.md` describes. It does not generate model
transformations or queries over models, because `SKILL.md` never says the model is
traversable — so the half of ceps that phase 7 depends on is, today, invisible to the
generator.

**The verification half is the bottleneck, and the gap is not expressiveness — it is the
quality of what the tool reports back.** That observation sets the ordering below.

That thesis is developed further, and turned into an entry strategy, in
**[LANG-ROADMAP.md](LANG-ROADMAP.md)**. The division between the two documents: this one
is defect-driven and asks what is broken; that one asks what the serious league of
language designers requires before it will look. They meet at exactly one point — Phase 0
here and L1 there are the same work.

A second caveat, added after the sweep recorded in **[INVENTORY.md](INVENTORY.md)**: a
large part of what this roadmap proposes to build **already exists and does not run**.
Thirty-nine of forty-one run scripts invoke a binary path that no longer exists, and four
regressions ([D13](DEFECTS.md#d13)–[D16](DEFECTS.md#d16)) account for most genuine
failures. **Phase 0 below comes before everything else**, because it is cheap, mechanical,
and it changes what the rest of this document is working with.

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
| [D7](DEFECTS.md#d7) | which transition fired is not reported, only the state delta — *by design*, see 2.3 |
| [D5](DEFECTS.md#d5) | `Transition Coverage: -nan ( -nan% )` |
| [D6](DEFECTS.md#d6) | coverage silently overstated — `SM`-endpoint edges are not counted |
| [D8](DEFECTS.md#d8) | `--report_format_json` is accepted and does nothing |

Six of the twelve known defects sit in the feedback channel — five of them genuine bugs,
with D7 a deliberate design decision that nonetheless shapes what a consumer can learn. A
human eyeballing one trace notices none of them; a machine iterating against it cannot
function, because a wrong number and a broken tool are indistinguishable.

Hence: **fix the instrument before running the experiment.** `c{}` makes ceps more
expressive, and ceps is already expressive. A trustworthy machine-readable contract makes
ceps *usable by machines*, which is the stated goal and the thing currently missing.

A second argument for this ordering: `c{}` is roughly 1200 LOC carrying a substantial
test burden, and those tests are read *through* the execution trace. Building it first
means validating new semantics with an instrument known to drop lines, invent lines, and
miscount coverage.

---

## Phase 0 — Harvest what is already built

*Prerequisite for everything else. Mostly mechanical. Source: [INVENTORY.md](INVENTORY.md).*

The sweep of 370 models found that the code is in substantially better condition than the
ability to demonstrate it. Nothing in this phase is new development.

### 0.1 Repair the harness

39 of 41 run scripts invoke `../../x86/ceps`, `../eclipse/Debug/statemachines` or
`../../x86/sm`. None exists; `ceps` is on `PATH` and `bin/ceps` is byte-identical to it.
One substitution across 41 files.

**Done when** every run script executes, and the two conventions already present in the
newer scripts — bare `ceps`, or `../../bin/ceps` — are the only ones left.

### 0.2 Fix the four regressions

| | Effect of fixing |
|---|---|
| [D14](DEFECTS.md#d14) | restores the assertion mechanism; removes a silent false green |
| [D13](DEFECTS.md#d13) | restores `test/jenkins/` and `test/mms_devops/` — 12 models |
| [D15](DEFECTS.md#d15) | restores `in_state(M.S)` guards, which phase 7 depends on — **and re-enables `cover_path{}`, which generates them** |
| [D16](DEFECTS.md#d16) | removes a segfault reachable by omission |

[D14](DEFECTS.md#d14) is first. While state assertions report an empty configuration, no
acceptance criterion in this document phrased as an expected configuration can be checked —
including those of phase 5.

### 0.3 Adopt the test layout that already exists

`vm/features/` is the right pattern: one file per behaviour, named after the behaviour,
with a `run` script, and all ten serialization cases pass. Extend it rather than inventing
a convention. The golden-file comparison in `test/native_main_loop/run.sh` supplies the
missing half.

**Done when** `test/run.sh` executes every area and reports a verdict per area, and a
regression anywhere is visible from one command.

### 0.4 Document the undocumented

Ranked by how much is lost by leaving it hidden:

1. **The modelling layer** — `partition{}`, `cover_path{}`, `signal{}`, `start_signal()`.
   Equivalence-class partitioning, generated coverage automata and generated stimuli: a
   complete model-based testing loop, registered and live since 2024, and the words do not
   occur in `SKILL.md`, `QUICK-START-UML-WITH-CEPS.md` or `README.md`. See
   [INVENTORY.md](INVENTORY.md) §4.2. `examples/doing_specs/lueftersteuerung/` is already
   a usable worked example; it needs a paragraph of prose, not new code.
2. **The document renderer** — `--ppe --format markdown|html5|…`, 3,295 lines, five
   writers ([INVENTORY.md](INVENTORY.md) §4.3). `--ppe` emits the specification, the test
   procedure in prose and a `Visited` column from the run, in one document. **This is the
   artifact that demonstrates the claim in [POSITIONING.md](POSITIONING.md)** — behaviour
   described, executed, and shown to have been executed, in something you can hand to
   someone who does not use the tool. One worked example with its output would do more for
   the positioning than any amount of prose.
3. **`.ceps.lex`** — partial parsers for foreign notations, six of them in the tree, zero
   documentation. See [INVENTORY.md](INVENTORY.md) §4.1 and
   [POSITIONING.md](POSITIONING.md) on the invariant: this is the only layer that applies
   to artifacts the other party wrote.
4. **The traversal layer** — [D12](DEFECTS.md#d12).
5. **Message definitions** — `doc/tutorial/serialization/README.md` exists and is good;
   link it from `SKILL.md` and `README.md`.
6. **Shadow states** — conformance checking, implemented 2017, mentioned nowhere.
7. **`--cppgen`** — verified working during the sweep, and absent from `--help` along with
   39 other accepted flags ([INVENTORY.md](INVENTORY.md) §4.10). Regenerating `--help`
   from the parser, or at least listing the generator flags, is a half-hour fix with a
   large discoverability payoff.
8. **`macro` and `.ceps/prelude.ceps`** — both live, both undocumented. The prelude is the
   answer to "why does `Event E;` no longer parse", which is the first thing a returning
   user hits.
9. `rule{}` and `symbolic_equality`; the plugin interface; automatic differentiation.

### 0.5 Give `.ceps.lex` a loudness knob

`any => .` is what makes a lexer partial, and it is also what makes a typo in the input
indistinguishable from a sentence deliberately ignored. `msg{read}` solved this with
`onerror`. The same affordance is needed here — a rule that fires on input that was
consumed by no named pattern, reporting position and context.

Small, but it is the difference between a partial parser and an unreliable one.

---

## Phase 1 — Fix the reporting

Reporting is the weakest part of ceps today, and it has two independent axes. **Format**
is whether a consumer can parse the output. **Quality** is whether the output was worth
parsing. They are orthogonal: a JSON-wrapped message that says `Error:A.A.B is not a
state.` with no file and no line is still useless, it is merely machine-readably useless.
Both halves are in this phase.

### 1A — Format

**Goal:** one versioned, structured report that any consumer can parse, covering trace,
coverage, diagnostics and status.

**Why first:** it is the interface everything later plugs into, and it is cheap. The
report is *already* a `ceps::ast::Nodeset` with a settled schema, and a Nodeset-to-JSON
serialiser already exists and is already exercised by the websocket API.

**Correction on scope.** The report is already machine-readable in one sense that was
overlooked when this phase was first written: being a `Nodeset`, it can be queried by a
ceps program directly. `sm2mermaidjs_mark_visited_states.ceps` in
[cepsdev/mermaid](https://github.com/cepsdev/mermaid) reads
`root.summary.coverage.state_coverage.covered_states` and joins it against the structural
model to emit a diagram with the visited states marked — a report-to-model join in the
same notation as the model. The gap this phase closes is therefore narrower than it looks:
it is about *external* consumers, which cannot reasonably be asked to embed ceps.

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

### 1B — Quality

**Goal:** when ceps says something is wrong, the message is enough to fix it; when ceps
says nothing, nothing is wrong.

The second half matters as much as the first. ceps executes a great deal and reports very
little, and the gap between those two is where a user — human or machine — loses time
deciding whether the model is wrong or the tool is.

| Step | Work | Evidence |
|---|---|---|
| 1.6 | Source location in every diagnostic: file, line, column | `Error:A.A.B is not a state.` names neither the file nor the line |
| 1.7 | Stable error codes, so diagnostics can be matched and documented | — |
| 1.8 | Reject unimplemented options instead of accepting them silently | [D8](DEFECTS.md#d8) — three inert `--report_format_*` flags |
| 1.9 | Warn on legal-but-suspicious models | two unguarded transitions out of one state silently activate **both** targets — an implicit fork, see below |
| 1.10 | Report when output has been suppressed rather than omitting it silently | [D3](DEFECTS.md#d3) — the entry line simply is not there |
| 1.11 | Make the tool self-describing: list opcodes, list options, both machine-readably | the opcode gap of [D1](DEFECTS.md#d1) was found by diffing two source files, because nothing can be asked |

**Acceptance:** a wrong model produces a diagnostic that locates the problem in the source;
a right model produces no diagnostic; and no accepted input produces neither output nor
complaint.

### Field notes

The items in 1B are not hypothetical. They are what actually cost time while auditing this
repository in a single session:

- an accepted flag that did nothing, with no signal that the request was ignored
- a trace missing a line, with no indication that anything had been omitted
- a coverage figure of `-nan` passing a validity check that existed and was meant to stop it
- an error naming an identifier but not its location
- a plausible model exiting on signal 11 ([D2](DEFECTS.md#d2))
- two documentation statements contradicting each other, so there was no third reference
  point to triangulate against

One of these deserves separate mention, because it is a semantic question rather than a
reporting one. Given

```
sm{A; states{Initial;X;Y;}; t{Initial;X;}; t{Initial;Y;}; };
```

ceps produces `A.Initial- A.X+ A.Y+` and exits 0. Both targets become active: two
unguarded transitions sharing a source are an **implicit fork**. That may well be
intended — ceps has orthogonal regions, and the `THREAD` / `REGION` machinery exists —
but it is reachable without writing anything that looks like a fork, and nothing in the
output says a fork happened. At minimum it should be visible in the report. Whether it
should also require explicit syntax is a design decision worth taking deliberately, and
it bears directly on the `c{}` rule forbidding calls inside thread regions
(`RECURSIVE-STATE-MACHINES.md` §5).

Individually these are small. Together they produce a tool whose failure modes are
indistinguishable from a user's mistakes — which is survivable for a human who can
experiment, and disqualifying for an automated consumer that cannot.

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

**The entry convention is settled:** on entry both the machine and its initial state are
logged, i.e. `A+ A.Initial+`. `SKILL.md` deliberately continues to document current
behaviour until 2.1 lands, so its trace examples must be revisited as part of this phase.

**2.3 is a design question, not a bug.** `SKILL.md` states the omission is deliberate:
execution traces are *compact projections* onto sequences of sets of state changes, they
are explicitly "not diagnostic traces", and **observer state machines are the canonical
way to enrich them**. That is a coherent position, and an elegant one — the enrichment
mechanism stays inside the language and is compositional.

The counter-argument is narrow but real: an observer must be *added to the model* to
diagnose it, so the instrument changes the thing it measures, and a counterexample
produced from an uninstrumented run cannot say which edge was taken when two edges yield
the same delta. For an automated consumer that matters, because it cannot go back and
re-run with instrumentation it did not know it needed.

Both can be true at once: keep the default trace a pure state-change projection, and
expose taken transitions as an *opt-in* channel in the phase-1 structured report rather
than in the human-readable trace. That honours the design position and still gives
machines the causal information. Either way, the dead `return;` in
`log_triggered_transitions` should become an explicit decision rather than a silent one.

**Acceptance:** replaying the trace from the empty configuration reproduces the final
configuration exactly; and if 2.3 is taken, the transition responsible for each step is
recoverable from the structured report without modifying the model.

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

### 3.3 — The half that was missing from this phase

As first written, this phase treated coverage as a **measurement** problem. It is also a
**generation** problem, and that half is already built: `partition{}` derives a coverage
automaton from a declarative partition of a system state's value range, and `signal{}`
derives the stimulus ([INVENTORY.md](INVENTORY.md) §4.2). The loop
*partition → generate → stimulate → measure* closes today, for one example.

What is missing is not the mechanism but its reach:

| Step | Work | Defect |
|---|---|---|
| 3.3 | Re-enable `cover_path{}` so sequence obligations can be stated, not only class obligations | [D15](DEFECTS.md#d15) |
| 3.4 | Report *which* class transitions were missed, not only the percentage — the data is already in the s-expression report as `not_covered_transitions_by_id` | — |
| 3.5 | Decide whether an unclassified region of a partition should be reportable | — |

**3.5 is a design decision and it is the interesting one.** A `partition{}` need not be
total — the fan-control example leaves `(1.0, 2.0]` unclassified and the machine simply
rides through it. That is correct by the same rule that governs partial programs, and it
is also exactly where a typo hides. This is the loudness question from phase 0.5
reappearing at the test-design layer, and it should get the same answer: partial by
default, with an opt-in that names what fell through.

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

**Deferred under the positioning.** `c{}` is the one item here that fails the test in
[POSITIONING.md](POSITIONING.md): it does not help someone who has not agreed with anyone
yet. The specification is sound and the work is good; expressiveness is simply not what
blocks the users this tool is for. Keep it specified, build it when the instrument is
trustworthy and the composition story is told.

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

## Phase 7 — Composition: make "largely compatible" checkable

**Goal:** given several independently authored models, compute whether they fit, and name
the obstruction when they do not.

**Status:** half of this exists and runs today.

### Composition has three layers, not one

An earlier draft of this phase treated compatibility as agreement on **event names**. That
is one third of the question, and `yamdl` is a reminder of which third was missing — it
stood for *yet another **message definition** language*, and the thing teams disagreed
about was payloads.

| Layer | Question | Status |
|---|---|---|
| **Wiring** | do the event names line up? | prototyped — `extract_events_transitively_and_group.ceps`, 81 lines |
| **Behaviour** | does this implementation refine that concept? | **built** — shadow states, since 2017 |
| **Payload** | do the messages agree, and are the disagreements survivable? | **built** — `msg{}` definitions with `onerror`, `vm/features/serialization`, 10/10 passing |

Two of the three are already implemented and neither is documented. Phase 7 is therefore
much less construction than it appeared: it is mostly **connecting three existing checks
and reporting their results together**.

The payload layer is the interesting one, because it already distinguishes *incompatible*
from *tolerable*. `case_read_non_existing_field_with_onerror` is precisely the situation
where the other party has not added a field yet: the reader declares a handler, the run
continues, and the omission is recorded rather than fatal. That is schema evolution
without a schema registry, and it is the composition check's notion of "largely".

### Where it comes from

This is the oldest idea in ceps, older than state machines and older than the VM. In the
precursor language *yamdl*, written for the BMW HAF research project, several teams were
deadlocked for weeks on a shared API. The resolution was to drop the requirement that the
API be agreed first: each team specified what it needed, omitted what it did not, coded
against its own model, and integration proceeded by **replacing the part of a team's world
that existed only in the model with another team's implementation**. Partial programs are
the language feature that falls out of that decision — not an ergonomic convenience but
the mechanism that made parallel work possible.

The axiom was that everyone arrives at different views of almost the same thing, which
should be *largely* compatible. That axiom was asserted, never checked; it was confirmed
or refuted only when substitution succeeded or failed. Phase 7 is about checking it.

### What already exists

`extract_events_transitively_and_group.ceps` in
[cepsdev/mermaid](https://github.com/cepsdev/mermaid) — 81 lines of ceps — walks the
models and derives each machine's event signature, transitively through nested machines:

```
$ ceps examples_ceps_sm/sm_with_actions.ceps extract_events_transitively_and_group.ceps
{
  "components":
 [
    { "name":"basic_example1", "in_events":["ANY_KEY","CAPS_LOCK"], "out_events":["OUT1","OUT2","OUT3"]},
    { "name":"basic_example2", "in_events":["ANY_KEY","OUT1","OUT2"], "out_events":["CAPS_LOCK","OUT2"]}
 ]
}
```

The structural point matters more than the output: the interface is **derived, not
declared**. A declared interface is a second artifact that can drift from the model it
describes. A projection of the model cannot. That is also why the yamdl trick worked
socially — an interface that is a query over work already done needs no consensus before
work can start.

### What is missing

Set algebra over that output, and a name for each kind of mismatch:

| Relation | Meaning |
|---|---|
| produced ∖ consumed | a dead output, **or** the world is missing a participant |
| consumed ∖ produced | an unmet dependency, **or** a genuine environment input |
| produced ∩ consumed | the wiring, i.e. the induced component graph |

In the output above, `OUT3` is produced by `basic_example1` and consumed by nobody, and
`ANY_KEY` is consumed by both machines and produced by neither. Both are facts a reviewer
wants on sight. Neither is reported today.

| Step | Work |
|---|---|
| 7.1 | Bring the event-signature extraction into this repository, documented and tested |
| 7.2 | Add the obstruction computation: unconsumed outputs, unproduced inputs, induced component graph |
| 7.3 | Allow the environment to be declared, so a genuinely external input is distinguishable from a missing producer |
| 7.4 | Emit the result in the phase-1A report format |
| 7.5 | Expose it on the MCP surface as *compose*, beside *validate* and *simulate* |

**Acceptance:** several independently authored models can be checked for fit in one
command, and every mismatch is named with the event and the machines involved.

**Why it matters for the thesis.** When an agent generates machines independently — or
several agents do — this is the artifact that says whether the pieces fit. It is the
phase-6 integration problem arriving early. Unlike phases 1–3 it needs no engine work:
7.1–7.3 are ceps programs, which makes it the cheapest substantial item in this document.

**Prior art.** Proving components correct against explicit assumptions about their
environment is *assume-guarantee* (equivalently rely-guarantee) reasoning: Misra & Chandy,
*Proofs of Networks of Processes*, IEEE TSE, Jul 1981, 417–426,
doi:10.1109/tse.1981.230844; Jones, *Tentative Steps Toward a Development Method for
Interfering Programs*, ACM TOPLAS, Oct 1983, 596–619, doi:10.1145/69575.69577. The
"different views of almost the same thing, which should glue" formulation is sheaf-shaped,
and has been developed as such for sensor fusion: Robinson, *Sheaves are the canonical
data structure for sensor integration*, Information Fusion, Jul 2017, 208–224,
doi:10.1016/j.inffus.2016.12.002.

---

## Summary of sequencing

```
Phase 0  harvest: harness, 4 regressions, test layout, docs  ──> everything
Phase 1  reporting: 1A format, 1B quality  ──┐
Phase 2  complete, causal trace            ──┼──> Phase 5  c{} recursive state machines
Phase 3  trustworthy coverage              ──┘
Phase 4  robustness (independent, do opportunistically)
Phase 6  MCP: 6a thin adapter ──> 6b ceps-native
Phase 7  composition checking (no engine work; 7.4 wants 1A, 7.5 wants 6a)
```

**Phase 0 precedes everything and is mostly not development.** [D14](DEFECTS.md#d14)
alone blocks every acceptance criterion in this document that is phrased as an expected
configuration, and the harness repair is a single substitution across 41 files.

Phases 1–3 are each small, and together they convert ceps from a tool that prints things
into a tool that can be *called*. Phase 5 is the expressiveness leap, and it lands on
solid ground once the instrument is trustworthy.

If only one thing is done after phase 0, do 1B. Format without quality gives a consumer a
clean parse of an unhelpful message; quality without format still lets a human work. Both
together are what the thesis needs.

Phase 7 is the exception to that ordering, because it is nearly free: steps 7.1–7.3 need
no change to the engine at all, two of its three layers are already implemented, and
together they deliver the one capability no competing statechart tool offers — a derived,
checkable account of how separately written models fit together.

---

## Documents

| Document | Contents |
|---|---|
| [POSITIONING.md](POSITIONING.md) | what kind of tool ceps is, what it refuses to be, and the invariant underneath both |
| [DEFECTS.md](DEFECTS.md) | 17 defects, each with a minimal reproducer |
| [LANG-ROADMAP.md](LANG-ROADMAP.md) | the other roadmap: what entry into the serious league of languages requires, and why the reference is the gate |
| [SCHEDULE.md](SCHEDULE.md) | day-by-day execution plan for L1–L3, 16 Nov – 31 Dec 2026 |
| [INVENTORY.md](INVENTORY.md) | the 370-model sweep: what exists, what runs, what is undocumented |
| [RECURSIVE-STATE-MACHINES.md](RECURSIVE-STATE-MACHINES.md) | the `c{}` specification |
| [SKILL.md](SKILL.md) | the language and tool reference |
| [QUICK-START-UML-WITH-CEPS.md](QUICK-START-UML-WITH-CEPS.md) | UML-oriented introduction |
| [cepsdev/mermaid](https://github.com/cepsdev/mermaid) | model transformation and traversal — in practice the reference for the metaprogramming layer, see [D12](DEFECTS.md#d12) |
