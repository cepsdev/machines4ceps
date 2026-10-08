# Language Roadmap

**Goal: ceps taken seriously as a language, by the people who design and implement
languages.**

This document is about *entry* — what has to be true before the project can be argued
about on its merits rather than dismissed on its packaging. It is deliberately separate
from [ROADMAP.md](ROADMAP.md), which is defect-driven and asks "what is broken and in what
order should it be fixed". This one asks a different question: **what does the serious
league require, which of those things does ceps have, and what is the shortest honest path
to the rest.**

The two overlap at exactly one point — Phase 0 of `ROADMAP.md` and L1 of this document are
the same work seen from two directions. That is not a coincidence and it is the single
most important scheduling fact in either file.

---

## 1. The thesis

The claim is that ceps fits the LLM era better than the alternatives. That claim is
stronger than it usually gets stated, and it rests on two arguments. The second one is the
real one.

### 1.1 The review-surface argument

The dominant way to point a language model at a pile of unstructured documents is to have
it emit **the answer**: one JSON object per document, a million of them, audited by
sampling and hope.

The ceps shape has it emit **the rule** instead. `rollAut/lex/rollout.ceps.lex` is
**27 lines**, ends in `any => .`, and lifts a rollout plan into a tree. A human reviews
that page once. A machine then applies it deterministically, identically, a million times,
and the result is reproducible and diffable.

Review surface collapses from a million outputs to one page. That is not a marginal
improvement in an existing workflow; it is a different division of labour between the
model and the machine — **the model proposes the rule, the machine applies it** — and it
is the division that survives an audit.

### 1.2 The verification argument

Ask a language model for Python and then ask it for tests, and the tests are wrong in the
same direction as the code, because they came from the same misunderstanding. There is no
independent oracle. Every current answer to this problem is a process answer: more review,
more sampling, more evaluation harness.

ceps has a structural answer, and this repository already contains the demonstration of
it. The specification, the test procedure, the simulation, the coverage evidence, the
generated C++ and the running service are **projections of one artifact** (see
[POSITIONING.md](POSITIONING.md), "The invariant", and
[INVENTORY.md](INVENTORY.md) §4.3). They cannot disagree, because there is nothing for
them to disagree *with*. Review is of one text, and agreement between the projections is
mechanical rather than hoped for.

**Generated code that is checkable by construction** is a claim almost nobody in this space
can make. It should be the headline.

### 1.3 The dependency you will not like

Language models are fluent in Python because there is an ocean of Python. The sweep in
[INVENTORY.md](INVENTORY.md) found **370 `.ceps` models in this repository**, and most of
the world's remaining supply is in three private directories. A model writing ceps is
working from approximately nothing.

The only substitute for training data is a **precise grammar and semantics supplied in
context**. A small, regular language with a two-page reference can be generated correctly
by a model that has never seen it; a small, regular language with no reference cannot.

So the conclusion is uncomfortable and unavoidable:

> **The language reference is not the boring chore that comes after the interesting work.
> It is the enabling artifact for the entire LLM thesis.**

The same applies to failure behaviour, for the same reason. See L2.

---

## 2. What the serious league requires

Not a feature list — a short list of artifacts that the audience expects to exist before
it will spend attention. Current state verified 8 Oct 2026.

| # | Requirement | State | Gate |
|---|---|---|---|
| R1 | A language reference: grammar + semantics, citable | grammar exists as bison (`ceps.y`, 790 lines); prose reference is `doc/ceps-lang.md`, **20 lines**, titled *"NOT EVEN REMOTELY COMPLETE"* | **L1** |
| R2 | A conformance suite — "does my reading match the implementation?" | 41 run scripts, **39 dead paths** ([ROADMAP.md](ROADMAP.md) 0.1) | L1 / L5 |
| R3 | Errors that diagnose rather than announce | undeclared kind and unterminated struct produce the **identical** `syntax error` | **L2** |
| R4 | No silent wrong answers | [D17](DEFECTS.md#d17) | **L2** |
| R5 | An on-ramp: install → something real, unaided | `kind` declarations and the prelude convention defeat a newcomer in the first five minutes | L1 / L3 |
| R6 | One flagship demonstration a sceptic can run | exists, undocumented, scattered across four repositories | **L3** |
| R7 | One sentence that does not require prior agreement | [POSITIONING.md](POSITIONING.md) has the argument, not the sentence | L3 |
| R8 | Stability: versions, a changelog, deprecation | none | L5 |
| R9 | Performance that survives contact | AST is pointer-chasing; see L4 | L4 |

Four of the nine already have most of their work done somewhere in the tree and need
collecting rather than building. That is the good news and it is why this is a months
problem rather than a years problem.

---

## 3. Where ceps actually stands

Stated plainly, because the plan depends on being honest about the starting line.

**What is genuinely strong, and would be respected:**

- A coherent design position, written down by the author before any of this review:
  *bottom up over top down; syntax is secondary; every layer of the abstraction hierarchy
  — backends, IRs, AST — is hackable; hierarchical state machines are in the core*
  (`doc/ceps-lang.md`). Those four lines are a better statement of intent than most
  languages manage, and L1 should be built outward from them rather than replacing them.
- A real VM with its own instruction set and assembler (Oblectamenta), a native code
  path, and a C++ backend.
- Compile-time metaprogramming over the program's own tree (`static_for`, `val`,
  `as_identifier`, path expressions) that has been used in production to generate state
  machines per market per rollout.
- A document renderer producing five formats, 3,295 lines (§4.3 of `INVENTORY.md`).
- Two shipped applications built on it by someone who needed them, not by someone who
  wanted a language.

**What is weak, and would be noticed within an hour:**

- No grammar. No semantics. R1 is unmet, and nothing else in the list can be assessed
  without it.
- `static_for` — the construct every transformation in `rollAut` rests on — appears in
  **zero** documentation files in this repository.
- 54 command-line flags are parsed; 19 are listed in `--help`.
- Diagnostics do not diagnose.
- One critical silent-wrong-answer defect ([D17](DEFECTS.md#d17)) and sixteen others.
- The on-ramp has an undeclared prerequisite (`kind`) and a prelude convention nothing
  auto-loads.

The gap is not conceptual. Every item in the second list is finite, mechanical, and
boring — which is exactly why it is still there (see the closing section).

---

## 4. The entry strategy

### 4.1 One idea, not a feature list

Every language that got in did it on **one idea done thoroughly**: ownership for Rust,
comptime for Zig, supervision trees for Erlang, proof for Lean, arrays for APL. Each had
far more to it than the one idea; none of them led with the rest.

ceps has an unusually wide surface — state machines, a VM, an assembler, a C++ backend,
docgen, serialization, model-based test generation, a plugin system, CAN and websocket
transports. **Leading with the surface is the failure mode**, because it reads as an
unfocused personal project and every item invites a comparison with a specialist tool that
does only that.

The one idea is the projection invariant. Everything else is discovered later, by people
who have already decided to pay attention.

### 4.2 The sentence

R7 is a real deliverable, not a formality. It has to be sayable to someone who has not yet
agreed to anything. The candidate:

> **A language whose programs evaluate to documents — so a specification, its tests, its
> evidence and its implementation are the same text, and cannot drift.**

The LLM-era corollary, for the audience that cares about that:

> **Generated code you can check by construction, because the spec and the code are not two
> artifacts.**

### 4.3 The anchor

Do not explain `static_for` from first principles. Zig has taught the audience what
comptime is, and the shortest route into the idea is:

> *Comptime — except what you are computing over is the program's own tree, and what comes
> out can be a state machine, a document, or a running service.*

That sentence buys a hearing from exactly the people in R-list range.

### 4.4 The flagship

The demonstration already exists and is currently filed as a transformation directory in
a customer project. `rollAut/transformations/rollout2simulation.ceps`, in full:

```ceps
Simulation{
Start{
 static_for(e:root.rollout){
  static_for(market:e.markets.content()){
   market.content();
  }
 }
};
};
```

**Eight lines to dry-run a retail rollout across every market**, against 206 lines for the
tracking state machines, 249 for the worker and 156 for the watchdogs — all projected from
the same plan.

This is the right flagship, and it beats the two obvious alternatives:

- It beats the docgen demonstration (`--ppe --format markdown`) because that one only
  lands for an audience already sold on specification-driven development.
- It beats the MDF demonstration because that one requires explaining measurement data
  first.

It wins because **the audience already knows what dry-run normally costs them**: a parallel
code path, separately built, separately tested, drifting from the real one. Here it cannot
drift and it was nearly free. The argument is made before the explanation starts.

The MDF pipeline stays as the second demonstration — *lift a 1.2 GB measurement, run the
analysis, swap in hand-written test data by commenting out four lines* — because it proves
the invariant on data nobody authored.

---

## 5. The plan

### L1 — Grammar and semantics *(the gate)*

Nothing else ships before this. R1 blocks R2, R5 and the whole of §1.3.

1. **Grammar.** `ceps/core/src/grammar/ceps.y` is **790 lines of bison, 26 rules**, and the
   parser is generated from it — so it is authoritative by construction. This half is
   transcription and annotation, not reverse-engineering. Cover: structs, `kind` and
   kind-instance declarations, `val`, units and literals, path expressions, `static_for`
   and `for`, macros, `sm{}` / `states{}` / `t{}` / `Actions{}`, `Simulation{}`,
   `oblectamenta{}` / `asm{}`, `msg{}`. Record where it disagrees with `SKILL.md` rather
   than silently choosing one.
2. **Evaluation semantics.** The part that does not exist anywhere and that no reader can
   reconstruct: what runs when. The `--pr` → `--pe` → `--ppe` progression is already the
   user-visible shape of this and should be the document's spine — parse, expand, execute,
   with a statement of what each pass may observe and what it may produce.
3. **Name resolution.** Forced by [D17](DEFECTS.md#d17). Scopes, shadowing, the unit
   namespace, and what a binding does to a name already in the unit table.
4. **The standard prelude.** Decide whether `kind Event;` is predeclared, and if it is
   not, make `.ceps/prelude.ceps` auto-load. The current state — a convention observed in
   two projects that nothing in the implementation honours — is indefensible in a
   reference.
5. **Build `doc/ceps-lang.md` outward from its own four principles.** They are good. Keep
   them as the preamble.

**Done when:** a reader who has never seen ceps can write a correct model with three state
machines, a `static_for` transformation and a message definition, using only the reference.
Test it the obvious way — put the reference in a language model's context, ask for the
program, and see whether it compiles. That test is also, directly, the §1.3 deliverable.

### L2 — Loud failure

R3 and R4. Cheap, and it gates adoption harder than any feature.

1. **Fix [D17](DEFECTS.md#d17)**, at minimum by diagnosing the collision. A generated
   program *will* use `m` for a market and `s` for a step, because that is what reads
   naturally and no corpus teaches otherwise. A silent wrong answer in machine-generated
   code is disqualifying, not inconvenient.
2. **Diagnostics that name the problem.** Today an undeclared kind and an unterminated
   struct produce the identical message. The minimum bar: what was expected, what was
   found, and — for the single most common newcomer failure — "`Event` is not a known
   kind; declare it with `kind Event;`".
3. **Audit for other silent paths.** [D8](DEFECTS.md#d8) (flags parsed and never read) and
   [D7](DEFECTS.md#d7) (a feature disabled by an undocumented `return;`) are the same
   species: the system accepts an instruction and does nothing. Enumerate them.
4. **Give `.ceps.lex` the loudness knob** already scoped in `ROADMAP.md` 0.5. A partial
   parser whose catch-all is `any => .` is *designed* to discard; the user needs to be
   able to ask what was discarded. This matters more under §1.1 than it did before, since
   the catch-all is now load-bearing for the extraction story.

**Done when:** no documented construct can produce wrong output with exit code 0, and the
first five errors a newcomer hits each name their cause.

### L3 — The flagship demonstration

R6 and R7.

1. Extract the `rollAut` rollout example into this repository as a self-contained,
   runnable demonstration — lex file, plan, and the five transformations — with the
   customer specifics removed.
2. Write it up as the one-idea argument from §4.1–§4.3: one plan, five projections, eight
   lines for the one everybody has paid for separately.
3. Add the MDF pipeline as the second demonstration.
4. Fold the sentence into `README.md` and `POSITIONING.md`.
5. `--help` tells the truth (`ROADMAP.md` 0.4).

**Done when:** a sceptic can clone, build, run one command, and see five artifacts come
out of one file — without reading anything first.

### L4 — The flat arena

R9, and considerably more than a performance item.

The current AST is one heap node per element — `mk_struct`, `mk_int_node`,
`children()` returning a vector of pointers. Lifting a 1.2 GB measurement spends exactly
the compactness that `free-mdf` was handcrafted to achieve.

A flat, arena-allocated AST with index handles buys three things:

- **Speed and footprint**, the obvious one.
- **An image.** A flat arena is `write(2)` and `mmap(2)`. Lift once, map thereafter.
  Today every run re-lifts.
- **Subtree hashing.** A contiguous subtree is a byte range, so content-addressing it is
  one hash over a span. That is what makes "the projections cannot drift" *cheap* rather
  than merely true: you can tell which document, which generated C++ and which test need
  regenerating.

Two notes on sequencing. The `--pr` / `--pe` / `--ppe` progression is **already a
generational arena design** described in user-facing terms — parse into A, expand into B,
execute into C, each pass append-only. The layout has to be made to agree with the
pipeline that is already documented; the pass structure does not need inventing.

And the hazard: `plugin_entrypoint` returns a `ceps::ast::node_t` across the `.so`
boundary, and plugins allocate nodes on their own side. Arena growth invalidates pointers;
indices survive. **The plugin ABI has to change, which temporarily breaks the only
evidence that the lift works at scale.** Plan for that rather than discovering it.

**Done when:** a lifted measurement round-trips through an image file, and the plugin ABI
passes an arena.

### L5 — Stability and release discipline

R2 and R8. Unglamorous and load-bearing.

1. Repair the harness (`ROADMAP.md` 0.1) and promote it to a conformance suite: every
   construct in the L1 reference has a case, and the suite is the authority when the
   reference and the implementation disagree.
2. Versioning, a changelog, and a statement of what may change.
3. A release that is not a `git clone`.

### L6 — The ecosystem

Only after L1–L3. Editor support, a formatter, syntax highlighting, a package story —
`ROADMAP.md` Phase 6. None of it is worth anything before there is a reference to be
consistent with, and all of it is cheap once there is.

---

## 6. Sequencing

```
L1  Grammar + semantics ........... the gate; blocks everything
L2  Loud failure .................. concurrent with L1 from the D17 fix onward
L3  Flagship + the sentence ....... needs L1 done, L2 mostly done
L4  Flat arena .................... independent of L1–L3; schedule against yadb
L5  Conformance + releases ........ needs L1 for the authority question
L6  Ecosystem ..................... last, and cheap by then
```

L1 and L2 are the whole of the entry problem. L3 is the announcement. L4 is the thing that
makes the applications viable at scale and is better scheduled against yadb's needs than
against this list.

---

## 7. What this roadmap deliberately leaves out

- **New language features.** Nothing in the list adds one. `c{}` (recursive state
  machines, `ROADMAP.md` Phase 5) and shadow states are both interesting and both strictly
  after entry.
- **Repositioning as a general-purpose language.** That is the crowded shelf and the wrong
  argument; see §4.1.
- **The applications.** `rollAut` and yadb appear here only as evidence and as the
  flagship. A language designer is taken seriously when a real system exists in the
  language, built by someone who needed it — Erlang had the switch, Rust had Servo. Two
  already exist. They do not need to be rebuilt for this; they need to be *cited*.

---

## 8. The honest obstacle

Every item in L1, L2, L3 and L5 is in the last ten percent: the part with no unsolved
structure in it, the part that is transcription rather than design. That is precisely the
work that has not happened here in fifteen years, while the hard ninety percent — a VM, an
assembler, a backend, a renderer, two shipped applications — did.

It is worth noting that this has never been a uniform failure. `rollAut` has configs, a
health check, an environment template, a registry and a web front-end, because it had
users. The last ten percent gets done here when there is a second person. There has never
been a second person for ceps itself, and partly because there is no reference, there has
never been a way for one to arrive.

That loop is the actual obstacle, and it is not a discipline problem. **It is the first
thing L1 breaks.**

---

## Related documents

| Document | Relationship |
|---|---|
| [ROADMAP.md](ROADMAP.md) | Defect-driven. Phase 0 and L1 are the same work from two directions. |
| [DEFECTS.md](DEFECTS.md) | 17 defects. [D17](DEFECTS.md#d17) is the one that is disqualifying under §1.3. |
| [INVENTORY.md](INVENTORY.md) | What exists. §4.3 is the renderer behind the §1.2 argument. |
| [POSITIONING.md](POSITIONING.md) | The argument. This document supplies the sentence and the flagship it lacks. |
| `doc/ceps-lang.md` | 20 lines; the four principles L1 builds outward from. |
| [SCHEDULE.md](SCHEDULE.md) | the execution plan: Phase 0 (2–13 Nov) plus L1–L3 over 31 working days to 31 Dec 2026. |
