# INCOSE requirements to state machines in 3 minutes

This is a requirements document that runs.

Not a document *about* a state machine, and not a document from which someone later
hand-built one. The machine in the trace below was derived from the sentences below, by
machine, on this run.

If you know INCOSE and you know Rust, the short version is: imagine a procedural macro
whose input is English requirement sentences, and whose output is not tokens disappearing
into `rustc` but a model that stays inspectable, runnable, and attributable to the
requirement that produced each part of it.

## The requirements document

This is `door_lock.req`, complete. It is the only file a requirements engineer edits.

```
REQ-001
The controller shall initialise in the unlocked state.

REQ-002
While the controller is unlocked, when lock_requested is received,
the controller shall transition to the locked state.

REQ-003
While the controller is locked, when unlock_requested is received,
the controller shall transition to the unlocked state.

REQ-004
While the controller is locked, if tamper_detected is received,
then the controller shall transition to the alarming state.

REQ-005
While the controller is alarming, when unlock_requested is received,
the controller shall transition to the unlocked state.

REQ-006
While the controller is alarming, when lock_requested is received,
the controller shall illuminate the hazard_lamp.

REQ-007
When crash_detected is received,
the controller shall transition to the unlocked state.
```

Those sentences are written to **EARS** (Mavin et al., RE'09). The INCOSE *Guide to
Writing Requirements* does not define a requirement syntax of its own — its rule **R1
"Structured Statements"** requires conformance to *one of the agreed patterns*, and GtWR
names EARS as one such pattern set. So "INCOSE-conformant" here means: conforms to a
published pattern, and EARS is the pattern chosen. `SPEC.md` §2 says exactly what is and
is not claimed.

## Run it

```sh
./run.sh
```

```
=== INCOSE conformance report ===
=================================
controller.Initial- controller.unlocked+
[REQ-002] unlocked on lock_requested to locked
controller.locked+ controller.unlocked-
[REQ-004] locked on tamper_detected to alarming (unwanted behaviour)
controller.alarming+ controller.locked-
[REQ-006] alarming on lock_requested does illuminate_hazard_lamp
[REQ-005] alarming on unlock_requested to unlocked
controller.alarming- controller.unlocked+
[REQ-002] unlocked on lock_requested to locked
controller.locked+ controller.unlocked-
[REQ-007] any state on crash_detected to unlocked
controller.locked- controller.unlocked+
```

`controller.locked+` is an entry, `controller.unlocked-` an exit. Each bracketed line is
the requirement that authorised the step beside it.

## How, in two moves

**1. Partial parsing — the grammar emits ceps.**

`ears.ceps.lex` holds one rule per EARS pattern. A rule matches a *token* pattern over the
sentence and emits ceps source text:

```
ident - integer While the ident is ident , when ident is received ,
  the ident shall transition to the ident state \. =>
  incose_transition{
    req{double_quote $0 - $2 double_quote;};
    from{$7;}; on{$10;}; to{$20;};
  }; .
```

`$7`, `$10`, `$20` are captured tokens. REQ-002 therefore lands in the tree as data:

```
(STRUCT "incose_transition"
  (STRUCT "req" "REQ-002")
  (STRUCT "from" (ID "unlocked"))
  (STRUCT "on"   (ID "lock_requested"))
  (STRUCT "to"   (ID "locked")))
```

The word *partial* is load-bearing. This grammar is not obliged to understand the whole
document, and it does not pretend to. A sentence matching no pattern is **named in the
conformance report**, not quietly dropped — try `./run.sh --bad`.

**2. Metaprogramming — the machine is built by walking the model.**

`extract.ceps` then walks that tree and constructs the state machine. No templates, no
string emission; `static_for` is a loop that runs while the model is being built, and
`sm{…}` is the same state-machine construct you would write by hand:

```
incose_states{ root.incose_precondition_states.content().sort().unique(); };

static_for(e : root.incose_transition){
  t{ as_identifier(text(e.from.content()));
     as_identifier(text(e.to.content()));
     as_symbol(text(e.on.content()),"Event");
     as_identifier(text(e.tag.content())); };
}
```

A state exists **only** if some requirement names it in a `While …` precondition. Response
slots deliberately do not create states — so a requirement pointing at a state nobody
declared fails the run:

```
***Fatal Error:Statemachine 'controller': frozen is not a state (or statemachine).
```

Nothing implements that check. It falls out of building the state set honestly and then
letting the ordinary state-machine semantics reject a dangling reference.

## Files

| File | |
|---|---|
| **`STEP-BY-STEP.md`** | **the walkthrough — start here if you want to do this yourself** |
| `SPEC.md` | the specification: accepted language, mapping, acceptance criteria, limits |
| `door_lock.req` | the requirements document (printed above) |
| `nonconforming.req` | one deliberately bad requirement, to show it gets reported |
| `ears.ceps.lex` | the grammar: EARS sentences → ceps AST |
| `extract.ceps` | the transform: AST → `sm{}` |
| `simulate.ceps` | the stimulus scenario |
| `run.sh` | `--bad` reports a non-conforming requirement, `--model` prints the model |

`ears.ceps.lex` and `extract.ceps` contain no door-lock vocabulary. Point them at a
different `.req` file and you get a different machine.

The derived model is not confined to the simulator. `--dot_gen` draws it; `--cppgen`
compiles it to C++ for a target (`INVENTORY.md` §4.4). Same model, different projections.

## What makes this approach elegant, useful, and relevant

*The claims in this section are an opinion. They are signed at the end.*

**Elegant: one semantics instead of two.** A conventional requirements-to-code tool emits
source in some target language, and must therefore carry its own validator — because once
the output is Rust or C, a mistake in the model surfaces as a complaint about Rust, not
about requirements. That validator is a second, informal copy of the semantics, and it
drifts. Here the derived machine is checked by the very engine that runs hand-written
machines. The closure rule above cost no code at all. I regard that as the single
strongest thing in this example, and it is not a trick of the example — it is what you
get when generated and hand-written artefacts are the same kind of thing.

**Useful: traceability is structural, not clerical.** `[REQ-004]` appears in that trace
because the requirement identifier is a node on the transition, not because a mapping
table is maintained alongside the code. Side-tables rot by the third sprint. Structure
does not. For anyone who has had to answer *which requirement does this line implement*
in front of an assessor, this is the practical payoff.

**Relevant: the model is kept, not spent.** Code generators turn a model into an artefact
and throw the model away; everything you might later want — a diagram, a test suite, a
C++ build, a conformance report, a formal spec — must then be bolted on separately or
reverse-engineered. Here the model remains data after extraction, and simulation is one
projection among several. The cost of the *next* question you ask of these requirements is
near zero, which is the opposite of the usual curve.

**And the honest limit.** This prototype reads five sentence patterns in one small
document. Nothing checks the requirement *set*: two requirements disagreeing about where
the same state and trigger lead will silently produce a non-deterministic machine. Against
a real corpus — thousands of inherited requirements, written before anyone had heard of
EARS — the grammar would be the easy part, and deciding whether to rewrite the
requirements or grow the grammar would be the hard, and mostly political, part.
`SPEC.md` §10 is the full list. I would rather you read that section than this one.

— *Opinion of GitHub Copilot (Claude Opus 5), which built this example and wrote these
documents, 4 November 2026. Recorded here as a judgement, not as a finding; the verifiable
claims live in `SPEC.md`, where each is tied to an acceptance criterion you can re-run.*

## Next

- **`STEP-BY-STEP.md`** — every step from a blank requirement to a simulation run, with
  the techniques explained. Assumes no compiler background.
- **`SPEC.md`** — the specification, including what is deliberately out of scope.
