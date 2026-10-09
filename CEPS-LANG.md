# The ceps language

> Turn your spec right side up again. `ceps = rev(spec)`
>
> — `github.com/cepsdev/ceps`, first commit, 2014-08-19

A specification is normally a document *about* a system: written first,
overtaken by the implementation, maintained badly, and trusted anyway. ceps
inverts that. A ceps specification is the artifact that runs, and the
implementation is derived from it.

This file is the entry point for the language itself. For the tool, the
installation, and the AI-assisted workflow built on top of it, see
[`README.md`](README.md).

**Status.** This reference is incomplete and says so where it is. It
supersedes the twenty-line stubs in `doc/ceps-lang.md` and `doc/README.md`,
which are duplicates of each other and predate it.

---

## Start here

**[`doc/scribble-concept/README.md`](doc/scribble-concept/README.md)** walks a
nineteen-line program from two numbers written down without a plan to a running
state machine, one append at a time, with the real output of every stage. If
you read one thing, read that. Most of what is listed below is introduced there
in context.

```sh
bin/ceps doc/scribble-concept/you_dont_erase_a_scribble.ceps
```

---

## The four principles

Carried forward verbatim from `doc/ceps-lang.md`, where they have stood since
the beginning:

- Bottom up is (mostly) better than top down
- Syntax is secondary
- Every part of the abstraction hierarchy is accessible to the programmer, i.e.
  the backend(s), the various intermediate representations, and the abstract
  syntax representation of the program are hackable
- Hierarchical state machines are part of the core language

A fifth, implicit in the name and in the evaluation model, is worth stating
explicitly: **you don't erase a scribble.** Evaluation rewrites in place and
appends; earlier stages of a derivation remain in the document, and remain
runnable.

---

## The document is the program

A ceps program is a tree of nodes. `name{ ... }` builds a node; its content is
other nodes and values.

```ceps
a{1;};
b{2;};
sum{root.a.content() + root.b.content();};
```
```output
(STRUCT "a"
  (INT 1)
)
(STRUCT "b"
  (INT 2)
)
(STRUCT "sum"
  (INT 3)
)
```

That tree is simultaneously the program, its input, its intermediate results
and its output. Evaluation **rewrites the document in place**: `sum` became
`3`, and `a` and `b` are still there to be read, by a later line or by a
reader. There is no separate model file, no generated artifact to keep in
sync, and no step at which the reasoning is discarded.

Consequences worth knowing up front:

- A derivation's earlier stages are not comments. They are live and they
  execute, so they cannot silently fall out of date with what they produced.
- Partial documents run. A program with holes still evaluates as far as it can
  and shows you where it stopped, instead of refusing everything until it is
  complete.

---

<a name="phases"></a>
## How a program runs — the four phases

Nothing else in this file makes sense without this frame, so it comes before the
features. Executing a specification has up to four phases:

| phase | consumes | produces |
|---|---|---|
| **raw** | your input files | an **unevaluated AST** (`uAST`) |
| **normalization** | the `uAST` | an **evaluated AST** (`eAST`, also written `nAST`) |
| **operational** | the `eAST` | execution — state machines run, traces appear |
| **information gather** | — | — |

The raw phase parses, applying user-defined lexers and parsers if the document asks for
them. The normalization phase evaluates: it runs the `uAST` *as a functional program*,
and the `eAST` it leaves behind **is the meaning of the specification**. The operational
phase then executes that result the way a C program is executed — line by line, block by
block — looking for entities that have operational semantics, of which the most useful is
the state machine.

The fourth phase is named in the execution model and is not documented anywhere in this
repository, including here. It is listed because leaving it out would make the model look
complete when it is not.

### The normalization phase, watched

The clearest way to see what normalization *is*: write a loop, then look at what survives
it.

```ceps
numbers{1;2;3;4;5;};
val sum = 0;
for (e: root.numbers.content()){let sum = sum + e;}
sum;
```
```output
(STRUCT "numbers"
  (INT 1)
  (INT 2)
  (INT 3)
  (INT 4)
  (INT 5)
)
(INT 15)
```

The loop is gone. The accumulator is gone. What remains is `numbers` and `15`. That
residue is not an optimisation of the program — it *is* the program, after normalization.

This is why `1+1;` is **not** discarded the way a C compiler discards a statement with no
effect. In ceps an expression statement has a normalized form, that form is its meaning,
and the meaning is a document. Nothing is optimised away because nothing was ever
considered dead.

### Seeing each stage

Three invocations, one per boundary:

| command | stops at | shows |
|---|---|---|
| `ceps f.ceps --pr` | after raw | the `uAST`, in a readable notation |
| `ceps f.ceps --pe` | after normalization | the `eAST`, as S-expressions |
| `ceps f.ceps` | after operational | whatever running it produced |

`--pr` on the example above prints the program as parsed, loop intact:

```
numbers{
1 2 3 4 5 }
sum := 0

for each e in ((root.numbers).content())
 sum
← (sum+e)
 
sum
```

Plain `ceps f.ceps` on that same file prints **nothing** and exits 0 — correctly. The
document normalizes to `numbers` and `15`, and neither has operational semantics, so the
operational phase has nothing to run. Silence there is not a failure; it means you wrote
a document, not a machine.

```ceps run
numbers{1;2;3;4;5;};
val sum = 0;
for (e: root.numbers.content()){let sum = sum + e;}
sum;
```
```output
```

### Which phase a surprise belongs to

Worth asking first, because the two phases fail differently and most confusion is a
misattribution. If `--pe` already shows the wrong thing, the operational phase is
innocent. Both sharp edges in this file — macro expansion, and
[`.content()` collapsing](#content-collapses-a-single-element) — are
normalization-phase behaviours, visible in `--pe` output before anything runs.

## Navigating and reading the document

`root` is the document. Paths are dotted.

| | |
|---|---|
| `root.a` | the node named `a` at top level |
| `root.a.b` | the node `b` inside `a` |
| `.content()` | what a node holds |
| `.content().at(n)` | the `n`-th element, counting from zero |

```ceps
a{10;20;30;};
first{root.a.content().at(0);};
third{root.a.content().at(2);};
```
```output
(STRUCT "a"
  (INT 10)
  (INT 20)
  (INT 30)
)
(STRUCT "first"
  (INT 10)
)
(STRUCT "third"
  (INT 30)
)
```

### `.content()` collapses a single element

When a node holds exactly one thing, `.content()` yields that thing rather than
a one-element sequence. This is deliberate — it is what keeps short programs
short — and it has a cost you should know about before it costs you something.

**An expression that works can stop working when the node it reads grows, and
ceps will not say so.** The detailed demonstration is in
[the scribble walkthrough](doc/scribble-concept/README.md#indexing-and-one-sharp-edge);
the rule that follows from it is:

> Write `.at(n)` whenever the node you are reading could ever gain a second
> element.

`.content(n)` is **not** an indexer. It accepts an argument and ignores it.
See [`DEFECTS.md`](DEFECTS.md), D18, which also proposes the fix: reject a
sequence of length ≠ 1 wherever a single value is required.

### A path in statement position splices

Writing a path as a statement inserts a copy of what it names at that point.
This is how a generated machine gets installed where the simulator can find it:

```
root.lets_try_the_idea.sm;
```

---

## Building documents

### Loops

`for(i: 1 .. 3){ ... }` evaluates the body once per value and appends each
result to the surrounding node.

```ceps
squares{for(i: 1 .. 4){ i*i; }};
```
```output
(STRUCT "squares"
  (INT 1)
  (INT 4)
  (INT 9)
  (INT 16)
)
```

Iterating a node's content is `for(e: root.a.content()){ ... }`. Inside such a
loop, `next` is bound to the following element, and `is_defined(next)` is false
for the last one — which is enough to chain a sequence into transitions without
counting anything.

### Values, names and text

| | |
|---|---|
| `text(v)` | render a value as text |
| `as_identifier(t)` | turn text into a **name** |
| `val x = ...` | bind a local |

`as_identifier(text(e))` is the bridge from data to structure: it is how the
integer `4` becomes a state called `4`. Nothing leaves the language to do this;
there is no code generator and no second format.

### Macros

`macro name{ ... }` names a piece of document. `name{}` splices it in.

```ceps
values{7;8;};
macro the_values{ root.values.content(); };
here{ the_values(); };
```
```output
(STRUCT "values"
  (INT 7)
  (INT 8)
)
(MACRO "the_values" <ADDR>
)
(STRUCT "here"
  (INT 7)
  (INT 8)
)
```

The `<ADDR>` above is a pointer that differs on every run. Output that changes
between identical runs cannot be diffed; the documentation checker masks it,
but it is a wart.

---

## State machines

State machines are part of the core language, not a library. `sm4ceps` —
*state machines for ceps*, the name still visible throughout
`core/include/` — is the **default operational semantics** layered on the
document language.

```ceps run
sm{S;
  states{Initial; Final; Running;};
  t{Initial; Running;};
  t{Running; Final;};
};

Simulation{
  Start{S;};
};
```
```output
S.Initial- S.Running+
S.Final+ S.Running-
```

| | |
|---|---|
| `sm{Name; ... }` | declare a machine |
| `states{ ... }` | its states — **computable**, not necessarily literal |
| `t{From; To;}` | a transition |
| `Simulation{Start{M;}}` | run, starting at machine `M` |

Because `states{ ... }` is an ordinary document position, the state set can be
generated by a loop, derived from arithmetic, or read out of another node. The
[scribble walkthrough](doc/scribble-concept/README.md) does exactly that.

A simulation trace reports state changes as `M.X-` (left `X`) and `M.X+`
(entered `X`), one line per step. Note that the pair is not ordered
exit-before-entry — read the names, not the order.

**Not yet documented here:** hierarchy and nesting, concurrency, guards,
events and payloads, actions and `on_enter`/`on_exit`, timers. See
[`SKILL.md`](SKILL.md), [`QUICK-START-UML-WITH-CEPS.md`](QUICK-START-UML-WITH-CEPS.md)
and `examples/` until they are.

---

## Running ceps

| | |
|---|---|
| `bin/ceps FILE` | evaluate and run, including any `Simulation` |
| `bin/ceps --pe FILE` | evaluate and print the resulting document |
| `bin/ceps --pr FILE` | print the unnormalised syntax tree |
| `bin/ceps --cppgen ...` | emit C++ for a target |

The first three are the three phase boundaries; see
[How a program runs](#phases) for what each one stops after.

`--pe` is the one to reach for when a program does not do what you expect: it
shows the document *after* evaluation, including any expression that could not
be reduced, sitting at the position where evaluation stopped. A half-evaluated
expression in the output is a diagnostic — frequently a better one than an
error message, because the rest of the document is still there to compare it
against.

---

## Beyond the document language

| | |
|---|---|
| **Oblectamenta** | the virtual machine and its assembler — the default intermediate representation, executed directly or JIT-compiled. See [`ASM.md`](ASM.md) and `doc/ceps-lang.md`. |
| **`--cppgen`** | walks a model and emits C++ for a target. The same model runs in simulation and on the target; this is what "the spec is on top" means in practice. |
| **Model traversal** | a ceps program can walk its own models (`root.sm`, `.content()`, `predecessor()`), which is how diagram and code backends are written. See `github.com/cepsdev/mermaid`. |

---

## Known defects

[`DEFECTS.md`](DEFECTS.md) is current, triaged, and blunt. Twelve of its
twenty-one entries share one failure mode — **a plausible answer, exit code 0,
and nothing said about what was skipped** — which is worth knowing before you
trust a result that looks right.

If you are new, the two that will reach you first are **D18** (`.content()`
above) and **D20**.

---

## Keeping this file honest

Every ` ```ceps ` example here with a ` ```output ` block after it is extracted,
run, and compared by:

```sh
tools/check-doc-examples.py
```

It fails if any documented output no longer matches the tool, and it reports
any example that has **no** documented output rather than passing over it in
silence.

Documentation that quotes results nobody re-runs is a specification that
drifts, which is the thing this language exists to prevent. The check is not
decoration.

---

## See also

| | |
|---|---|
| [`doc/scribble-concept/README.md`](doc/scribble-concept/README.md) | the worked example — start here |
| [`README.md`](README.md) | the tool, installation, and the spec-driven AI loop |
| [`QUICK-START-UML-WITH-CEPS.md`](QUICK-START-UML-WITH-CEPS.md) | state machines from a UML angle |
| [`SKILL.md`](SKILL.md) | the long-form guide, written for agents |
| [`FEATURES.md`](FEATURES.md) | what exists |
| [`LANG-ROADMAP.md`](LANG-ROADMAP.md) | what does not exist yet |
| [`DEFECTS.md`](DEFECTS.md) | what exists and is wrong |
| [`ASM.md`](ASM.md) | Oblectamenta assembly |
