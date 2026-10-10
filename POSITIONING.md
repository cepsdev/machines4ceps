# Positioning

> **ceps is the tool for making described behaviour executable before anyone has agreed
> on anything.**

That sentence is a decision rule, not a slogan. Its job is to settle what ceps is obliged
to do well, and — more usefully — what it is free to refuse.

It has an operational companion, which says what you actually do about it:

> **SDDI — specify data in a high level notation and derive your interfaces.**

The decision rule tells you what ceps is for. SDDI tells you what to type. The order is
the whole point: the data description is written first and is the thing that is true; the
interface is a consequence of it, not a precondition for it. Conventional tooling runs
the other way — agree the interface, generate the types, fill in the logic — which is why
it stalls when nobody has agreed. SDDI has nothing to stall on, because the artifact you
start from is one you can write alone and run the same afternoon.

---

## The claim it rests on

Pre-consensus is not an early phase of a project. It is the **steady state of any
non-trivial software system.**

That is the load-bearing claim, so it is worth stating why it holds:

- **No single person holds the whole model.** Past a certain size, every participant works
  from a partial and slightly wrong picture of everyone else's part. This does not resolve
  as the project matures; it is what maturity looks like.
- **Agreement is a snapshot, not a state.** An interface is agreed on a Tuesday and has
  drifted by Friday, because the thing it describes kept moving.
- **The parties are rarely all present.** Vendors, suppliers, downstream consumers and
  future maintainers are part of the system and absent from the meeting.
- **Requirements arrive continuously.** A system that has stopped receiving them has
  stopped being developed.

So the condition ceps was invented for — teams who must proceed without having settled the
interface — is not a window that closes. It is the normal operating condition of software,
and the "settled" phase that tooling usually assumes is the exception.

### Why that distinction decides everything

If pre-consensus were a *phase*, ceps would be a prototyping tool: used for a fortnight,
then handed over to the real toolchain once the API settled. Prototyping tools are
permitted to be rough, because their output is thrown away.

If pre-consensus is the *steady state*, ceps is not a phase tool. Its output is not thrown
away. It runs in CI next year, against models nobody has read in months.

**The sharpened positioning therefore raises ceps's obligations rather than lowering
them.** Roughness in reporting would be forgivable in a prototyping tool. In a tool that
stays, it is disqualifying. See [Consequences](#consequences-for-the-roadmap).

---

## The invariant

Underneath the positioning there is a single design rule, applied consistently for ten
years and never named:

> **Specify what you need. Leave out what you don't. Proceed anyway.**

It is implemented six times, at six different scales, in two repositories:

| Layer | Construct | What may be omitted |
|---|---|---|
| Specification | partial programs | entities that are declared but never bound |
| Behaviour | `shadow_state(Concept.X, Impl.X)` | states and transitions the concept does not constrain |
| Message | `msg{read; …}` with `handler{onerror; …}` | fields the sender has not added yet, or sent in another order |
| Bits | `breakup_byte_sequence(s, bool(i1, any, any, …))` | bit positions the reader does not care about |
| Test design | `partition{}` | regions of the value range you do not classify |
| Foreign syntax | `.ceps.lex` with `any => .` | every sentence of the input language you chose not to match |

The test-design row was found last and is the clearest case, because the gap is visible in
a working example: `examples/doing_specs/lueftersteuerung/` partitions a temperature into
`niedrig` / `mittel` / `hoch` and leaves `(1.0, 2.0]` unnamed. Nine of thirty samples fall
in the hole; the generated machine rides through it and the run completes. Nobody decided
that unclassified regions should be tolerated — it falls out of writing the classes as
guards rather than as a total function.

A seventh instance sits outside this repository: `ceps/NOISE-MARKS.ceps` in the
demand-bundling project opens with *"guards and actions are DECLARED, not defined"*.

These are not six features that happen to resemble each other. They are one rule with
six backends, and the ordering matters: the first five apply to artifacts **you** wrote,
and the last applies to artifacts **the other party** wrote. Only that one actually
discharges the promise in the sentence at the top of this document, because the other
party does not write ceps — they have a `.dbc`, a `.feature` file, a diagram, a log.

### What follows from naming it

Three things that were previously unconnected become consequences rather than
coincidences.

**The strengths are not a list, they are a derivation.** Derived interfaces, traceability,
the single binary, the one-algebra property: each is what you get when totality is not
required. See the table below, which is now a dependency graph rather than an inventory.

**The declined categories are declined for one reason, not three.** Model checking,
certification kits and code-generation matrices all presuppose a closed world. The
refusals in this document are a single refusal applied three times.

**The gaps become visible, and they are gaps in the same place.** Partiality is only safe
when the system can say *"this was omitted"* out loud. `msg{read}` can: that is what
`onerror` is for. `.ceps.lex` cannot — a sentence that nearly matched is indistinguishable
from one deliberately ignored, so a typo silently deletes a test. A `partition{}` with an
unclassified gap cannot either — the generated machine rides through it and the coverage
figure says nothing. Unbound entities in a specification are reported inconsistently.
**Every partial layer needs a loudness knob, and only one layer of six has one.** That is
a concrete work item that only exists once the invariant is named.

### Why it was never named

Each instance arrived separately, under pressure, and felt obvious at the time. "Obvious"
here measures ceps's internal coherence — a new mechanism feels natural because it agrees
with the six already present — not the ordinariness of the idea. The result is that the
most distinctive property of the system was re-derived six times and written down zero
times, which is also why it is absent from `SKILL.md`.

### The artifact that proves the claim already exists

The sentence at the top of this document — *making described behaviour executable before
anyone has agreed on anything* — needs something you can put in front of a person who does
not use the tool. That something is already built and already runs:

```
$ ceps spec.ceps --ppe --format markdown
```

produces one document containing the specification, the test procedure rendered as prose
(*"Start state machine Motor. Trigger Event ev1."*), and a `Visited` column showing which
states the run actually reached. 3,295 lines, five output writers including HTML and two
markdown dialects, last touched January 2025, and named in none of the three
documentation files. See [INVENTORY.md](INVENTORY.md) §4.3.

The significance is not that it renders nicely. It is that the specification, the test and
the evidence are **projections of one model**, so they cannot disagree — which is the only
honest way to hand someone a document and claim the behaviour in it has been exercised.
Everything else in this document argues that ceps belongs in a category; this is the
artifact that would demonstrate it, and nobody outside the repository knows it exists.

---

## What the positioning is derived from

The category was not chosen and then justified. Every one of ceps's real strengths is
downstream of the invariant above, and load-bearing only under pre-consensus:

| Property | Follows from | Where it actually pays off |
|---|---|---|
| **Partiality** | — | the invariant itself |
| Interfaces are *derived* from models, not declared ([Phase 7](ROADMAP.md)) | partiality | only when no interface has been agreed |
| Foreign notations can be read without a grammar (`.ceps.lex`) | partiality | only when the other party will not adopt your notation |
| Conformance is checked as a simulation relation, not by exhaustion (shadow states) | partiality | only when the specification is deliberately incomplete |
| Test obligations are *generated* from a declared partition of the input domain (`partition{}`, `signal{}`) | partiality + AST as a first-class value | only when the acceptance criteria are themselves still moving |
| Model, transformation, execution and report in one algebra | AST as a first-class value | only when each party has to build its own view of the system |
| The configuration is a first-class value, so traces and coverage exist | same | only when you must produce evidence rather than assertions |
| No setup, no boilerplate, a single binary | — | only when the cost of *trying* must stay below the cost of *arguing* |
| LLMs write ceps fluently | — | only when there are more behaviours to describe than people to describe them |

Each of those is close to worthless in the categories declined below. Together, under
pre-consensus, they are the whole product.

---

## Categories declined

A category brings obligations. These three bring obligations ceps cannot meet, should not
want, or actively contradicts.

### Statechart modelling tool

*Competitors:* Yakindu/itemis, Stateflow, Rhapsody, SCXML tooling.

*Obligations:* a graphical editor, an IDE integration, code generation across a matrix of
targets, requirements traceability, certification kits for functional-safety standards,
and a sales motion to match.

*Verdict:* **decline, and say so out loud.** This is the category every reader will assume
by default, which is precisely why silence is not enough. It is also a feature-matrix
fight against twenty-year-old products, fought by one maintainer, on their ground.

### Formal specification language

*Competitors:* TLA+, Alloy, Z, B/Event-B, Maude.

*Obligations:* exhaustive verification — a model checker, a refinement relation, a
semantics worth citing.

*Verdict:* **decline, because it is incoherent with the design, not merely out of reach.**
Exhaustive checking needs a closed model. Partial programs are deliberately open. "Model
check this incomplete specification" is a research problem, not a missing feature. ceps
chose openness over closure ten years ago, before it had state machines, and that choice
is the reason it is useful at all. Reversing it would cost more than the category is
worth.

*But the refusal is narrower than "ceps does not verify."* It does verify, in the one way
that survives partiality. **Shadow states** check an implementation against a concept
machine by requiring the shadow map to be a simulation relation: every implementation
transition between two shadowed states must have a compatible concept transition, or the
run aborts. That is conformance checking, it is sound, it has been implemented since 2017
(`core/src/sm_sim_core_shadow_states.cpp`), and it needs no closed world — it quantifies
over the transitions that exist, not over all reachable states. `test/alloy/` and
`test/agda/` go further still, with a `rule{}` construct and a `symbolic_equality`
primitive that returns a structured difference.

The line is therefore: **ceps checks conformance, it does not search for counterexamples.**
It will tell you that this implementation refines that concept. It will not tell you that
no execution violates an invariant, because that question needs the closure ceps gave up.
Liskov and Wing's behavioural subtyping (*ACM TOPLAS* 16(6), 1994,
doi:10.1145/197320.197383) is the relation shadow states implement; it was arrived at
independently.

This matters because *"a specification language that happens to execute"* sounds like a
claim to the declined category. It is not. The word "specification" is earned by legal
under-specification, not by proof.

### Model-driven engineering platform

*Competitors:* EMF/Ecore with OCL and ATL/QVT, Xtext.

*Obligations:* metamodel tooling, editor generation, an ecosystem, enterprise support.

*Verdict:* **decline, despite having the better technical argument.** ceps uses one
notation where EMF uses four — model, query, transformation and generated executable are
the same language. That is a genuine advantage. It is also an advantage in a market that
is shrinking and enterprise-captured, so winning it wins little.

---

## The decision rule

For any proposed feature, ask:

> **Does this help someone who has not agreed with anyone yet?**

Applied to the current [roadmap](ROADMAP.md):

| Item | Verdict | Why |
|---|---|---|
| Fix the 39 dead run-script paths | **yes, immediately** | nothing else can be *demonstrated* until this is done ([INVENTORY.md](INVENTORY.md) §2) |
| [D14](DEFECTS.md#d14) — state assertions see nothing | **yes, immediately** | a specification that cannot check itself is not executable in the sense this document means |
| Phase 1A/1B — reporting | **yes, first** | they have to see what their half actually did, and convince someone else of it |
| Document `.ceps.lex` | **yes** | it is the only layer of the invariant that applies to the other party's artifacts |
| Phase 7 — composition checking | **yes** | it *is* the "we have not agreed" check, made computable |
| Phase 2 — complete, causal trace | **yes** | evidence that survives being shown to a sceptic |
| Phase 3 — trustworthy coverage | **yes** | a green result must mean something was checked |
| A diagnostic for unmatched input in `.ceps.lex` | **yes** | partiality without a loudness knob produces silent false greens |
| Phase 6a — MCP adapter | **yes** | more authors working in parallel, same unsettled world |
| [D12](DEFECTS.md#d12) — document the traversal layer | **yes** | it is how each party builds its own view |
| Phase 4 — robustness | **yes, opportunistically** | a tool that stays must not crash |
| Phase 5 — `c{}` | **defer** | expressiveness is not what blocks these users |
| Graphical editor | **no** | |
| Model checking | **no** | contradicts partiality; conformance checking already exists and does not |
| Code generation breadth | **no** | that is the settled-world problem |

The rule's value is that it rejects things cleanly. `c{}` is good work and the
specification is sound; it is simply not what someone stuck before an agreement needs.

---

## Consequences for the roadmap

The steady-state reading changes two things.

**1. Phase 7 becomes continuous, not a gate.** If the parties never finish disagreeing,
the composition check is not run once at integration — it runs on every change, forever.
That makes it a CI artifact, which makes it depend on the structured report of Phase 1A,
which confirms the existing ordering.

**2. Quality stops being negotiable.** A tool used for a fortnight can afford
`Error:A.A.B is not a state.` with no file and no line. A tool that is still running in
CI in two years cannot. The same applies to the stability of the report schema
(roadmap 1.3), and to the harness: **39 of 41 run scripts invoke a binary path that no
longer exists** (`INVENTORY.md` §2). An experiment becomes an asset only when something
breaks when it breaks.

**3. Partiality needs a loudness knob at every layer.** This follows from the invariant
rather than from the steady-state reading, but it lands in the same place: a tool that
stays must distinguish *"omitted deliberately"* from *"omitted by mistake"*, or its green
results decay into noise. `msg{read}` has `onerror`; `.ceps.lex` has nothing; and
[D14](DEFECTS.md#d14) is the degenerate case — an assertion that passes vacuously because
the thing it checks is empty.

The positioning writes a cheque that phases 1–3 have to cash.

---

## Costs and risks

Stated plainly, because a positioning document that only lists upside is marketing.

- **The category has no name and no one is shopping for it.** It must be explained every
  single time, and the explanation is a demonstration, not a sentence.
- **Readers will mis-slot ceps as a statechart tool** and evaluate it on a matrix it was
  never built to win. The refusal above has to be visible, not implied.
- **The obligations are harder, not easier.** A tool people keep must be dependable, and
  ceps today has seventeen known defects, six of them in the channel that reports results
  and one ([D14](DEFECTS.md#d14)) in the mechanism by which a model checks itself.
- **The invariant is invisible.** Six implementations, no name, no documentation, no
  mention in `SKILL.md`. An unnamed idea cannot be defended in an argument, taught to a
  contributor, or recognised by its own author — which is how it came to be re-derived
  six times.
- **One maintainer.** Every declined category is also declined capacity that will not be
  spent defending ground that does not matter.

The compensation is that the category is empty.

---

## Provenance

None of this is a new direction. It is a recognition of what ceps has been doing since
before it was called ceps.

The precursor language, *yamdl*, was written for the BMW HAF research project, where
several teams had been deadlocked for weeks on a shared API. The resolution was to stop
requiring agreement first: each team specified what it needed, omitted what it did not,
coded against its own model, and integration proceeded by replacing the modelled part of a
team's world with another team's implementation. Partial programs are the language feature
that fell out of that decision.

The same pattern produced everything else worth having here. No access to ECU hardware
produced the simulator; a simulator too slow for the target produced the ceps-to-C++
compiler; the need to see inside a model produced the traversal layer; a corpus of foreign
notations produced `.ceps.lex`; the need to show that a specification had actually been
exercised produced `partition{}` and `signal{}`. The good ideas came from constraints, not
from planning sessions — which is a reason to trust this one, since it came from the same
place.

It is also the reason the invariant was never written down. Each instance was a response
to a specific pressure, and each felt like the obvious move rather than an application of
a principle. The principle was there the whole time; nobody had occasion to say it.

---

## Documents

| Document | Contents |
|---|---|
| [ROADMAP.md](ROADMAP.md) | what to build, in what order |
| [DEFECTS.md](DEFECTS.md) | what is broken, with reproducers |
| [INVENTORY.md](INVENTORY.md) | what exists, what still runs, and what is implemented but undocumented |
| [RECURSIVE-STATE-MACHINES.md](RECURSIVE-STATE-MACHINES.md) | the `c{}` specification (deferred, see above) |
| [SKILL.md](SKILL.md) | the language and tool reference |
