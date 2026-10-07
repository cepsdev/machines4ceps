# Positioning

> **ceps is the tool for making described behaviour executable before anyone has agreed
> on anything.**

That sentence is a decision rule, not a slogan. Its job is to settle what ceps is obliged
to do well, and — more usefully — what it is free to refuse.

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

## What the positioning is derived from

The category was not chosen and then justified. It is the one place where every one of
ceps's real strengths is load-bearing at the same time:

| Property | Where it actually pays off |
|---|---|
| Partial programs are legal | only when the description is incomplete — which, per the claim above, is always |
| Interfaces are *derived* from models, not declared ([Phase 7](ROADMAP.md)) | only when no interface has been agreed |
| No setup, no boilerplate, a single binary | only when the cost of *trying* must stay below the cost of *arguing* |
| Model, transformation, execution and report in one algebra | only when each party has to build its own view of the system |
| The configuration is a first-class value, so traces and coverage exist | only when you must produce evidence rather than assertions |
| LLMs write ceps fluently | only when there are more behaviours to describe than people to describe them |

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

This is worth stating explicitly because *"a specification language that happens to
execute"* sounds like a claim to this category. It is not. The word "specification" is
earned by legal under-specification, not by proof.

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
| Phase 1A/1B — reporting | **yes, first** | they have to see what their half actually did, and convince someone else of it |
| Phase 7 — composition checking | **yes** | it *is* the "we have not agreed" check, made computable |
| Phase 2 — complete, causal trace | **yes** | evidence that survives being shown to a sceptic |
| Phase 3 — trustworthy coverage | **yes** | a green result must mean something was checked |
| Phase 6a — MCP adapter | **yes** | more authors working in parallel, same unsettled world |
| [D12](DEFECTS.md#d12) — document the traversal layer | **yes** | it is how each party builds its own view |
| Phase 4 — robustness | **yes, opportunistically** | a tool that stays must not crash |
| Phase 5 — `c{}` | **defer** | expressiveness is not what blocks these users |
| Graphical editor | **no** | |
| Model checking | **no** | contradicts partiality |
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
(roadmap 1.3) and to the roughly sixty of sixty-six `test/` areas that nothing currently
executes — an experiment becomes an asset only when something breaks when it breaks.

The positioning writes a cheque that phases 1–3 have to cash.

---

## Costs and risks

Stated plainly, because a positioning document that only lists upside is marketing.

- **The category has no name and no one is shopping for it.** It must be explained every
  single time, and the explanation is a demonstration, not a sentence.
- **Readers will mis-slot ceps as a statechart tool** and evaluate it on a matrix it was
  never built to win. The refusal above has to be visible, not implied.
- **The obligations are harder, not easier.** A tool people keep must be dependable, and
  ceps today has twelve known defects, six of them in the channel that reports results.
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
compiler; the need to see inside a model produced the traversal layer. The good ideas came
from constraints, not from planning sessions — which is a reason to trust this one, since
it came from the same place.

---

## Documents

| Document | Contents |
|---|---|
| [ROADMAP.md](ROADMAP.md) | what to build, in what order |
| [DEFECTS.md](DEFECTS.md) | what is broken, with reproducers |
| [RECURSIVE-STATE-MACHINES.md](RECURSIVE-STATE-MACHINES.md) | the `c{}` specification (deferred, see above) |
| [SKILL.md](SKILL.md) | the language and tool reference |
