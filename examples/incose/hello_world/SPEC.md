# Extracting an executable state machine from an INCOSE-conformant requirements document

**Status:** specification for the prototype in this directory. Implemented and running.
**Scope:** `examples/incose/hello_world/` only. One core bug fix (§9.1); no other change to `core/`.

---

## 1. Purpose

Demonstrate, end to end and in one command, that a requirements document written to a
published requirements-writing standard can be mechanically turned into a state machine
that *runs*, with every transition traceable to the requirement that justified it.

The chain is:

```
door_lock.req  ──ears.ceps.lex──▶  ceps AST  ──extract.ceps──▶  sm{}  ──▶  simulation trace
   (English)       (grammar)        (data)      (transform)     (model)      (behaviour)
```

No step is new machinery. `.ceps.lex` already transduces foreign notation into ceps AST;
`static_for`/`make_struct` already generate model elements from AST. This example is the
first one in the tree that points them at *requirements* rather than at CAN databases or
log files.

### 1.1 What this is not

It is not natural-language processing. Nothing here infers meaning. The document is
parsed against a fixed, published sentence pattern, and anything outside that pattern is
reported, not guessed at. That restriction is the point: it is what makes the result a
*specification* rather than a plausible reading of one.

---

## 2. What INCOSE actually mandates

A clarification that shapes the whole design, because it is easy to get wrong.

The relevant document is the INCOSE **Guide to Writing Requirements** (GtWR), v4,
INCOSE-TP-2010-006-04, 2023 — note the title says "to" since v4; v3 (2019) said "for".
It defines **15 characteristics** (C1–C15) and **42 rules** (R1–R42).

**GtWR does not define a requirement syntax.** Its rule R1 *Structured Statements* says
only that statements "must conform to one of the agreed patterns", and its pattern
appendix supplies a generic *pattern-slot* framework that an organisation instantiates
itself. It then points at external pattern catalogues. One of those, named explicitly in
v3 §B.3, is **EARS**.

So "INCOSE-conformant" cannot mean "written in INCOSE syntax" — there is no such thing.
It means: *conforming to some agreed pattern*, per R1. This prototype picks EARS as that
pattern and conforms to it.

> **Honesty note.** GtWR v3 lists EARS as one external pattern source among several
> (alongside Dick & Llorens, Hull et al., the PABRE catalogue, ARTEMIS CRYSTAL). That is
> a citation, **not** an adoption — GtWR does not make EARS's keyword set its own. Whether
> v4's Appendix C retains the same reference list could not be verified against the
> primary text. Do not read this example as "INCOSE prescribes EARS".

### 2.1 Rules this prototype mechanically enforces

Conformance to a pattern is checked by construction: a sentence either matches a pattern
or is reported. That gives the following rules for free, because no non-conforming
sentence can reach the model.

| Rule | Name | How the grammar enforces it |
|---|---|---|
| **R1** | Structured Statements | only the five patterns of §3 are accepted |
| **R2** | Active Voice | every pattern names the responsible entity as grammatical subject |
| **R3** | Appropriate Subject-Verb | subject slot is fixed; §4 D4 requires it be constant |
| **R5** | Definite Articles | `the` is a literal in every pattern; `a`/`an` cannot parse |
| **R18** | Single Thought Sentence | one sentence, one pattern, one model element |
| **R19** | Combinators | `and`/`or`/`unless`/`but` appear in no pattern, so they cannot parse |

Characteristics served: **C3** Unambiguous (one parse or none), **C5** Singular,
**C7** Verifiable (the requirement becomes an executable transition), **C9** Conforming.

The rules this prototype does **not** check are listed in §10.

---

## 3. The accepted language

### 3.1 Document shape

A requirements document is a sequence of requirements. Each is an identifier line
followed by one statement sentence:

```
REQ-002
While the controller is unlocked, when lock_requested is received,
the controller shall transition to the locked state.
```

Whitespace, including line breaks, is not significant (`%ignore_ws`); the identifier and
its sentence are one token stream. Blank lines and comment prose between requirements are
ignored. An identifier is `<word>-<number>`; the number's leading zeros are preserved.

### 3.2 The five patterns

Written as EARS templates, with the prototype's slot restrictions. `<state>`, `<trigger>`,
`<verb>` and `<object>` are single identifiers; `<system>` is a single identifier.

| # | EARS pattern | Accepted sentence |
|---|---|---|
| **P1** | Ubiquitous | `The <system> shall initialise in the <state> state.` |
| **P2** | State-driven + Event-driven (complex) | `While the <system> is <state>, when <trigger> is received, the <system> shall transition to the <state> state.` |
| **P3** | State-driven + Event-driven, action response | `While the <system> is <state>, when <trigger> is received, the <system> shall <verb> the <object>.` |
| **P4** | Unwanted behaviour | `While the <system> is <state>, if <trigger> is received, then the <system> shall transition to the <state> state.` |
| **P5** | Event-driven | `When <trigger> is received, the <system> shall transition to the <state> state.` |

P5 carries no precondition and therefore applies in **every** declared state — the shape
a crash-unlock requirement actually has.

### 3.3 Reserved words

The lexer's pattern language gives `enter`, `exit`, `any`, `else`, `endl`, `ident`,
`integer`, `int`, `double` and `string` their own meaning, so they cannot appear as
literals in a requirement sentence. This is why the mode verb is **`transition to`** and
not `enter`. Requirement text must avoid those ten words.

That `any` and `string` are among them is a happy accident: GtWR **R7** *Vague Terms*
tells you not to write "any" in a requirement regardless.

---

## 4. Extraction semantics

Five rules, all closed and checkable. No lexicon, no inference.

> **D1 — State declaration.** A state exists **iff** it appears in the precondition slot
> (`While the <system> is <state>,`) of at least one requirement. The state set of the
> generated machine is exactly that set, plus `Initial`.

> **D2 — Closure.** Every state named in a response slot (P2, P4, P5) or in the
> initialisation requirement (P1) must be declared per D1. A violation is an error.
>
> This is enforced *by ceps itself*: `states{}` is generated from the D1 set alone, so an
> undeclared target is an unresolved state in `t{}` and the build fails. The rule costs no
> checking code — it is a consequence of generating the state set honestly.

> **D3 — Response classification.** A response is a **transition** iff it has the form
> `shall transition to the <state> state`. Every other response is an **action**, named
> `<verb>_<object>`. There is no third case and no verb lexicon.

> **D4 — Single subject.** The `<system>` slot is expected to be the same identifier
> throughout; it names the generated machine. (Not enforced in the prototype — §10.)

> **D5 — Partiality is reported, never silent.** Every identifier line whose sentence
> matches no pattern yields a `nonconforming` record naming the requirement. See §6.

### 4.1 The mapping

| Requirement element | Model element |
|---|---|
| precondition state | source state |
| trigger | `Event` symbol |
| `shall transition to the X state` | target state |
| `shall <verb> the <object>` | action `<verb>_<object>`, self-transition |
| P1 initialisation | `t{Initial; <state>;}` |
| P5 (no precondition) | one `t{}` per declared state |
| requirement identifier | action `<ID>` fired by the transition (§7) |

---

## 5. Worked example

### 5.1 Input — `door_lock.req`

```
REQ-001
The controller shall initialise in the unlocked state.

REQ-002
While the controller is unlocked, when lock_requested is received,
the controller shall transition to the locked state.

REQ-004
While the controller is locked, if tamper_detected is received,
then the controller shall transition to the alarming state.

REQ-006
While the controller is alarming, when lock_requested is received,
the controller shall illuminate the hazard_lamp.

REQ-007
When crash_detected is received,
the controller shall transition to the unlocked state.
```

(The full document has seven requirements; the four omitted here are the symmetric
unlock cases.)

### 5.2 Derived model

Applying D1: the precondition slots mention `unlocked`, `locked`, `alarming`. That — and
nothing else — is the state set.

Applying D2: targets are `locked`, `alarming`, `unlocked`, and the initial state is
`unlocked`. All three are declared. The model closes.

```
sm{
 controller;
 states{ Initial; alarming; locked; unlocked; };
 t{Initial; unlocked;};                                       // REQ-001
 t{unlocked; locked;   lock_requested;   REQ_002;};
 t{locked;   alarming; tamper_detected;  REQ_004;};
 t{alarming; alarming; lock_requested;   REQ_006;};           // action response
 t{alarming; unlocked; crash_detected;   REQ_007;};           // P5, one per state
 t{locked;   unlocked; crash_detected;   REQ_007;};
 t{unlocked; unlocked; crash_detected;   REQ_007;};
};
```

### 5.3 Expected trace

Stimulus `lock_requested, tamper_detected, lock_requested, unlock_requested,
lock_requested, crash_detected` produces:

```
controller.Initial- controller.unlocked+
[REQ-002] unlocked on lock_requested to locked
controller.locked+ controller.unlocked-
[REQ-004] locked on tamper_detected to alarming (unwanted behaviour)
controller.alarming+ controller.locked-
[REQ-006] alarming on lock_requested does illuminate_hazard_lamp
...
```

Each line of behaviour names the requirement that caused it. That is the deliverable.

---

## 6. Partiality, and why it is reported

`.ceps.lex` is a *partial* parser: `any => .` drops whatever was not matched. For a CAN
database that is a feature — it is what lets ceps read a `.dbc` without a grammar for all
of DBC. For a requirements document it is a hazard, and the repository already knows it:

> *"`msg{read}` has `onerror`; `.ceps.lex` has `else => rewind exit`, but that is control
> flow, not a diagnostic. A sentence that nearly matches is indistinguishable from one
> deliberately ignored... Partiality needs a way to be loud; this layer does not have one."*
> — `INVENTORY.md` §4.1

A dropped sentence is a **missing transition in a machine that still runs and still passes**.
That is the worst failure mode a specification tool can have.

This prototype makes partiality loud without touching the core. The grammar carries a
final rule that matches an identifier line whose sentence matched no pattern, and emits a
`nonconforming` record. `extract.ceps` splices those records into the machine's `on_enter`,
so the report is printed every single run, before any behaviour:

```
=== INCOSE conformance report ===
[REQ-101] NON-CONFORMING: matches no accepted pattern, no model element generated
```

`nonconforming.req` in this directory exists to demonstrate it. Its content is taken
verbatim from `examples/req.ceps`, which is already in the tree:

> *"The system shall not unlock the doors while vehicle speed is greater than 5 km/h
> unless crash mode is active."*

That sentence breaks **R19** (`while`, `unless` as combinators), **R18** (two thoughts:
a speed interlock and a crash override), and expresses a prohibition rather than a
behaviour. It cannot become a transition, and the run says so by name.

---

## 7. Traceability

Each generated transition fires an action named after the requirement
(`REQ-002`, mangled to the identifier `REQ_002`), whose body prints the requirement
identifier and the transition it justifies. Consequences:

- the **model** carries the requirement identifier — visible in `--pe` and in `--dot_gen`;
- the **trace** names the requirement behind every step, so a simulation run reads as a
  list of requirements exercised;
- an unexercised requirement is one whose identifier never appears in the trace.

### 7.1 Why the trace text is built in the grammar, not the transform

The action body is emitted by `ears.ceps.lex`, not by `extract.ceps`. This is forced, not
chosen. `print` accepts only literal arguments at generation time: inside
`make_struct(..., print(...))`, an argument such as `e.req.content()` is a syntax error,
and `val` bindings are not available at that position. The grammar, by contrast, emits
ceps *source text*, so it can interpolate captured slots into a string literal directly.
The resulting `print(...)` sits in the tree as inert `FUNC_CALL` data until spliced into
an action body, where it executes.

Mixing presentation into the grammar is a wart. It is recorded here rather than hidden,
and §10 notes what would remove it.

---

## 8. Architecture

| File | Role |
|---|---|
| `door_lock.req` | the requirements document — the only file a requirements engineer touches |
| `nonconforming.req` | one deliberately non-conforming requirement, to demonstrate §6 |
| `ears.ceps.lex` | the grammar: EARS sentences → ceps AST. §3 is its specification |
| `extract.ceps` | the transform: AST → `sm{}`, per the rules of §4 |
| `simulate.ceps` | the stimulus scenario of §5.3 |
| `run.sh` | runs the conforming document; `run.sh --bad` adds `nonconforming.req` |

The division matters: **`extract.ceps` contains no door-lock knowledge and `ears.ceps.lex`
contains no door-lock vocabulary.** Both are generic over EARS documents. Only
`door_lock.req` and `simulate.ceps` are domain-specific.

### 8.1 Invocation

```sh
ceps ears.ceps.lex door_lock.req extract.ceps simulate.ceps
```

Argument order is load order and is significant: the grammar must be registered before
the document it governs, and the document must be read before the transform that consumes
its AST.

---

## 9. Acceptance criteria

| # | Criterion | How to check |
|---|---|---|
| **A1** | The conforming document produces a machine that runs to completion | `./run.sh` exits 0 and prints a trace |
| **A2** | The state set is exactly the D1 set plus `Initial` | `./run.sh --model` shows `states{Initial;alarming;locked;unlocked;}` |
| **A3** | All five patterns P1–P5 produce their mapped model element | §5.2 appears in `--model` output |
| **A4** | Every transition in the trace names its requirement | each `+`/`-` step is preceded by a `[REQ-nnn]` line |
| **A5** | A non-conforming requirement is reported by identifier, never dropped | `./run.sh --bad` prints `[REQ-101] NON-CONFORMING` |
| **A6** | A non-conforming requirement does not break the conforming ones | `./run.sh --bad` still produces the full A1 trace |
| **A7** | D2 is enforced: an undeclared target state fails the build | documented in `README.md`; add `shall transition to the frozen state` and the run fails |
| **A8** | The transform is domain-neutral | `sed 's,//.*,,' extract.ceps ears.ceps.lex \| grep -i 'door\|lock\|alarm\|crash'` is empty (comments stripped; they name the domain by way of example) |

### 9.1 Core change

One fix in `core/src/state_machine_simulation_core.cpp`, required by this example:

```cpp
// before — underflows for any file name shorter than 6 characters
else if (file_name.length() > 3 && ( file_name.substr(file_name.length()-4,4) == ".xml" ||
                                     file_name.substr(file_name.length()-6,6) == ".scxml") ){
```

`length()-6` is computed on a `size_t`, so a 5-character name such as `d.req` wraps to
`SIZE_MAX` and `substr` throws `std::out_of_range` — ceps aborts with an uncaught C++
exception before it reaches any of its own diagnostics. The guard is split so each
`substr` is governed by its own length test. Behaviour is otherwise unchanged.

This is on the path of this example: it introduces a new input extension, and the
obvious short file name crashes the tool.

---

## 10. Limitations

Deliberate, for a prototype of this size:

1. **One machine, flat.** No hierarchy, no concurrency, no sub-machines.
2. **No guards, no time.** EARS preconditions that are *conditions* rather than states
   ("while speed exceeds 5 km/h") have no representation. They are the natural next step:
   they map to ceps guards, not to states.
3. **D4 is not enforced.** A document naming two different systems silently yields one
   machine. Checking it needs set comparison in the transform.
4. **Slots are single identifiers.** A state is `unlocked`, not "the doors are unlocked".
   Multi-word slots need either a quoting convention or phrase-to-identifier mangling.
5. **No set-level characteristics.** C10–C15 are properties of the requirement *set* —
   completeness, consistency. Nothing here checks for contradictory requirements, and two
   requirements giving the same state/trigger pair different targets will produce a
   non-deterministic machine without complaint. This is the most valuable missing check.
6. **Non-conformance is reported, not counted.** There is no exit code for "document did
   not fully conform", so this cannot yet gate CI.
7. **The §7.1 wart.** Removing it means letting `print` take generated arguments inside
   `make_struct`, which is a core language change, not an example change.

---

## 11. References

**Standards**

- INCOSE Requirements Working Group. *Guide to Writing Requirements*, v4,
  INCOSE-TP-2010-006-04, 2023. (15 characteristics C1–C15; 42 rules R1–R42. v3,
  INCOSE-TP-2010-006-03, 2019, was titled *Guide for Writing Requirements* and had 41
  rules; its Appendix B.3 is the one that names EARS.)
- ISO/IEC/IEEE 29148:2018, *Systems and software engineering — Life cycle processes —
  Requirements engineering*. Cited by GtWR; its requirement template is a **different**
  one and should not be conflated with GtWR's own pattern-slot framework.

**EARS**

- A. Mavin, P. Wilkinson, A. Harwood, M. Novak. "Easy Approach to Requirements Syntax
  (EARS)." *17th IEEE International Requirements Engineering Conference (RE'09)*, 2009,
  pp. 317–322. doi:10.1109/RE.2009.9
- A. Mavin, P. Wilkinson. "Big Ears (The Return of 'Easy Approach to Requirements
  Engineering')." *RE'10*, 2010, pp. 277–282. doi:10.1109/RE.2010.39 — this, not the 2009
  paper, is the article GtWR v3's EARS hyperlink actually resolves to.
- A. Mavin, P. Wilkinson. "Ten Years of EARS." *IEEE Software* 36(5), 2019, pp. 10–14.
  doi:10.1109/MS.2019.2921164

**Prior art — structured requirements to behavioural models**

- B. Walter, J. Martin, J. Schmidt, H. Dettki, S. Rudolph. "Executable State Machines
  Derived from Structured Textual Requirements — Connecting Requirements and Formal System
  Design." *MODELSWARD 2019*, pp. 195–202. doi:10.5220/0007236601950202
- L. Lucio, T. Iqbal. "Formalizing EARS — First Impressions." *1st International Workshop
  on Easy Approach to Requirements Syntax (EARS @ RE'18)*, 2018, pp. 11–13.
  doi:10.1109/EARS.2018.00009 — formalises EARS as LTL rather than as automata.

**In this repository**

- `INVENTORY.md` §4.1 — the `.ceps.lex` audit, and the partiality gap §6 responds to
- `POSITIONING.md` — why meeting the other party in their own notation is the point
- `import-scripts/dbc/param_import_dbc.ceps` — the `make_struct` / `root.select` idiom
  reused by `extract.ceps`
- `vm/features/common/gherkin.ceps.lex` — the closest existing grammar, and the one this
  one is modelled on
