# Goals: reworking the `.ceps.lex` engine

Companion to `AUDIT.md`, which records what `core/include/transform/streamtransform.hpp` does
today. This document states what it should become, in terms concrete enough to tell when each
goal is met.

Nothing here is a schedule. The last section proposes an order, and the order is the only
claim about sequencing.

## What the component is for

The engine turns text that is not ceps into ceps. It does so by **partial recognition**: a
grammar names the shapes it understands, emits ceps source for each, and leaves everything
else alone. It is not asked to parse a language — it is asked to find the sentences it knows
inside a document written for human beings, and hand the rest back.

That framing decides most of what follows. A tool that must understand everything needs a
full grammar formalism. A tool that understands *some* of a document needs, above all, an
honest account of which parts those were.

## The yardstick

The owner of this codebase states the criterion as: **minimal, its order bears meaning, the
mechanism is sandwiching layers, a child can understand it.**

Taken seriously as an engineering rule it says three things, and each one decides a question
below:

1. **One mechanism, applied repeatedly**, beats several mechanisms each applied once.
2. **Order is allowed to carry information** — it is the cheapest encoding of dependency, and
   it uses something that already exists instead of inventing apparatus.
3. **A layer may be handed to, and may hand back. It may not reach up.** The `print` wart in
   `examples/incose/hello_world/SPEC.md` §7.1 is condemned by this rule; so, arguably, is the
   construct in [L13](AUDIT.md#l13).

Where a goal below is a judgement call rather than a defect, this is what it is judged
against.

---

## G1 — Memory safety is not a property of the input

**Statement.** No document, and no grammar, can cause a read or write outside an allocation.
The engine's safety must not depend on how long a number is, how many operators a table has,
how large a grammar is, or whether a file ends in a newline.

**Why.** Four of the thirteen findings are this ([L1](AUDIT.md#l1), [L3](AUDIT.md#l3),
[L4](AUDIT.md#l4), [L5](AUDIT.md#l5)), with one shared cause: a size assumed rather than
checked. One of them — L1 — is reachable from a supplier's `.dbc` file.

**Met when.**

- No fixed-size stack buffer is indexed by a length taken from input. Numeric conversion uses
  `std::from_chars` over a pointer range, so the buffer in question ceases to exist.
- Every array indexed by a count that comes from elsewhere either grows or refuses, and
  refusing produces a diagnostic rather than a corruption.
- The input buffer carries a sentinel byte, and every scanning loop tests its bound before it
  dereferences.
- A fuzz corpus — random bytes, truncated files, very long tokens, deeply nested layers —
  runs to completion with no sanitizer report. `-fsanitize=address,undefined` on the test
  corpus is the practical form of this.

**Note.** This goal is pure containment: no behaviour visible to a correct grammar changes.
That is what makes it the right first stage.

---

## G2 — Every rejection names a place

**Statement.** Every error the engine raises carries a file, a line and a column, and
describes the problem in the vocabulary of the grammar language — not in token-kind integers,
and not in question marks.

**Why.** [L6](AUDIT.md#l6). Eight throw sites, of which five say some number of `?`. No error
anywhere carries a position. A six-line grammar is survivable; `dbc.ceps.lex` is 113 lines.

**Met when.**

- `match_ex` carries the byte offset at which the error was detected; the catch site converts
  it to line and column by scanning the buffer it still holds. This work happens **only on the
  error path** — the matching loop is not asked to maintain a line counter.
- `eval_ex` either carries the same or is removed.
- Every message names the construct: *unknown pattern keyword* `foo`, *expected* `=>` *after
  the pattern*, *rule has no terminating* `.`, *`$7` out of range, the pattern has 5 tokens*.
- A deliberately broken grammar produces one message a first-time user can act on, verified
  for each error class in the test corpus.

**Note.** This is the prerequisite for G3 and G5 — both of which need somewhere to report
to — which is why it sits this early despite being rated High rather than Critical.

---

## G3 — Nothing is understood partially, in silence

**Statement.** When no rule in the active layer recognises the input, that fact is
*available*: where it happened, what text was involved, and which layer was active. A grammar
may choose to ignore it. It may not be denied the information.

**Why.** [L10](AUDIT.md#l10). Today that branch has three exits and no diagnostic in any of
them. The consequence is already visible: the INCOSE grammar had to carry its own catch-all
rule and emit its own non-conformance node, because the engine could not be asked. Every
grammar re-invents this, per grammar, by hand — and `%skip_unmatched_tokens` removes the
ability entirely.

**Why it matters beyond convenience.** For a requirements tool the unmatched set *is* the
product. "These eleven of your four hundred requirements are not in a form I understand, here
they are" is the answer an engineer needs. It is a weaker and more useful claim than
conformance, and the engine is the only thing that knows it.

**Met when.**

- Unmatched input is recorded with position and text, and is retrievable after a run.
- There is a way to surface it as ceps — so a transform can walk it like anything else —
  without each grammar writing a catch-all rule.
- `%skip_unmatched_tokens` suppresses the *emission*, not the *record*.
- Running a grammar against a document it only partly covers yields a count and a list, and
  the INCOSE example can drop its hand-rolled catch-all without losing the conformance
  report.

**Open.** Whether this is a new ceps node, an exit code, a side channel, or all three. See
*Open decisions*.

---

## G4 — References are names, not positions

**Statement.** A rewrite action refers to what a pattern matched by name. Positional `$N`
either goes away or becomes a legacy spelling.

**Why.** `D25` in `DEFECTS.md`, and [L7](AUDIT.md#l7) is the proof that it is not a
theoretical hazard: the engine's *own documentation* gets the indexing wrong. `$N` is
0-based, counts punctuation, and is unchecked — a miscount binds a neighbouring token (often
a valid identifier, so the error is invisible by shape) and an out-of-range index yields an
empty node. Both at exit 0.

The cost is paid by the reader. The left-hand side of an EARS rule reads like the sentence it
matches; the right-hand side reads `$7`, `$10`, `$20`. One half of the rule is prose and the
other is arithmetic, and `STEP-BY-STEP.md` §3.2 contains a 23-cell table that exists only to
let a human do that arithmetic.

**Met when.**

- A pattern element can be named, and the action refers to the name. `$ident` is already
  taken by side-effect variables, so the syntax is open — `<system>` reads naturally on both
  sides and keeps the left-hand side looking like text.
- Out-of-range and unbound references are rejected **when the rule is read**, not silently at
  match time. `pattern_len_` is already on the rule; the check is one comparison.
- The slot table in `STEP-BY-STEP.md` §3.2 can be deleted, and the EARS rules read as
  sentences on both sides.

---

## G5 — Text is text

**Statement.** A document in UTF-8 is tokenised correctly, or rejected with a message. It is
not silently mistokenised.

**Why.** [L2](AUDIT.md#l2), and it is the most serious finding in the audit. An identifier
containing a non-ASCII byte is split at that byte, and the fragments fall out as unknown
tokens. The visible effect, reproduced against the shipped INCOSE grammar, is that a
well-formed requirement containing `Türschloss` is reported to the engineer as
`NON-CONFORMING: matches no accepted pattern, no model element generated` — at exit 0, with
every clause of that sentence false. The conformance machinery becomes a source of confident
false accusations, and the state machine goes missing from the model.

A German, French or Swedish requirements corpus is not an edge case for an INCOSE tool. It is
the normal case.

**Met when.**

- `TOK_UTF8` is either produced or removed. It has been declared and unused since the file
  was written.
- Every `isalpha`/`isalnum`/`isdigit`/`isspace` call takes an argument that cannot be
  negative — today they take a signed `char`, which is undefined behaviour for every byte
  ≥ 0x80 regardless of the tokenisation question.
- A requirements document in UTF-8 with accented identifiers produces the same model as its
  transliterated equivalent.
- Malformed UTF-8 is reported with a position (G2), not absorbed.

**Open.** How deep this goes — byte-transparent, codepoint-aware, or Unicode identifier
classes. See *Open decisions*.

---

## G6 — The grammar stays readable to the person who owns the requirements

**Statement.** A requirements engineer, a test engineer or a systems engineer can read a
`.ceps.lex` grammar and say whether it means what the document means — without learning
regular expressions, BNF, or C++.

**Why.** This is the property that made the INCOSE prototype work, and it is the one a
rework can lose by accident. It rests on the **inverted default** (`AUDIT.md`, *What is
right*): a bare word is a literal, so only `ident`, `integer`, `string`, `any`, `else` and
the escapes are syntax. Regex, BNF and ANTLR all default the other way, and the price is that
the grammar becomes an artefact the domain expert must take on trust — normally the *only*
such artefact in a requirements toolchain.

**Met when.**

- The inverted default survives every other change in this document. Specifically: G4's named
  holes must not require quoting the literals around them.
- No new construct requires a reader to know precedence or escaping rules in order to read a
  rule aloud.
- The EARS grammar in `examples/incose/hello_world/ears.ceps.lex` gets *shorter* or stays the
  same length. If a goal below makes it longer, the goal is wrong or its design is.

**This goal is a veto, not a task.** It has no work of its own; it rejects work.

---

## G7 — The documentation is executed

**Statement.** The authoritative description of the `.ceps.lex` format is a document whose
examples are files in the tree that are run by the test corpus.

**Why.** [L7](AUDIT.md#l7). The only documentation of the format is a header comment whose
worked example uses two functions that do not exist, numbers its holes 1-based against a
0-based engine, and shows a dialect no grammar in the tree uses. It has been wrong for as
long as it has existed, because nothing could notice.

**Met when.**

- The header comment's invented example is deleted. The algorithm sketch above it is worth
  keeping; the CANOps grammar is not.
- There is one reference document for the format, and every grammar fragment in it is
  extracted from, or checked against, a file that runs.
- A change to the engine that breaks a documented example fails the corpus.

---

## G8 — One mechanism, repeated

**Statement.** The engine recognises and rewrites. Computation belongs to ceps. Any construct
that does not fit *a layer is handed to, runs until it stops recognising, and hands back* has
to justify itself against the whole of §*The yardstick*.

**Why.** [L13](AUDIT.md#l13). `Sideeffects_node` is sixteen node kinds, a recursive-descent
expression parser, a global symbol table and a tree-walking evaluator — roughly a quarter of
the file. It is a second language with its own evaluation semantics living inside the first.
Its documentation does not parse. `CLEAR_SYMBOLS`, `GT` and `LT` are declared and unreachable.
Its symbol store is global mutable state that cuts across the layers rather than nesting with
them: a layer reaching sideways.

The INCOSE grammar reached a complete working state without touching any of it.

**Met when** — one of the following is true and written down:

- it is cut, and guards are expressed some other way (or not at all); or
- it is reduced to guards over matched tokens with no mutable store, documented and covered by
  the corpus; or
- it is kept whole, documented, covered, and the case for the fourth mechanism is made
  explicitly.

**The goal is the decision, not any particular outcome.** What is not acceptable is a quarter
of the file whose status nobody can state.

---

## G9 — The corpus exists

**Statement.** One command runs every grammar in the tree against fixed inputs and compares
against recorded output.

**Why.** There is no executable test for this file. `test/cepslexer/` holds two grammars and
an import script, untouched since March 2024, with **no input document, no expected output
and no runner** — the intent existed and stalled. Meanwhile seven grammars depend on the
engine, one of them a working DBC importer. A corpus would have caught
[L1](AUDIT.md#l1) (128 digits), [L7](AUDIT.md#l7) (`suffix()` was never callable), `D25`'s
`$99`, and would catch [L2](AUDIT.md#l2) the first time anyone wrote an umlaut.

**Met when.**

- Each case is a grammar, an input, and the expected ceps — comparison by text.
- Error cases are cases: a broken grammar's *message* is part of the expected output, which
  is what keeps G2 from eroding.
- It runs under `-fsanitize=address,undefined`, which is where G1 is actually enforced.
- It runs in well under a minute, so it is run.

---

## G10 — Nothing is allocated per token

**Statement.** Scanning a token performs no heap allocation.

**Why.** [L9](AUDIT.md#l9). `std::vector<bool> guard_is_true(n)` is constructed and destroyed
for **every token of every input file**, in the matcher. `matching_rules_` is the bit-packed
`vector<bool>` specialisation, accessed in the innermost loop. This is the inner loop of every
import the product performs, and the fix is to make two locals into members.

**Met when.**

- The per-token allocation count is zero, demonstrated by measurement rather than inspection.
- A before/after figure on the largest `.dbc` in the tree is recorded, so the claim is a
  number.

**Deliberately last.** It is the cheapest goal here and the least important; doing it first
would be optimising a component that is about to change shape.

---

## Non-goals

Stated so that scope does not drift during the rework.

- **Not a general parser generator.** No context-free grammars, no ambiguity resolution, no
  left recursion, no precedence declarations. The layer mechanism plus longest-match is the
  whole model, and G6 is the reason to keep it that way.
- **Not a regular-expression engine.** Character classes beyond the existing token kinds would
  buy expressiveness at exactly the cost G6 forbids.
- **Not an evaluator.** ceps evaluates; `extract.ceps` walks the model. Anything the engine
  computes is a thing computed twice, in two semantics, that will drift. This is G8 stated
  from the other side.
- **Not incremental or streaming.** Reading the whole file into one buffer is what makes
  `state_t` a pair of integers and backtracking a value copy. That is worth more than
  streaming.
- **Not a new file format.** The seven `.ceps.lex` grammars in the tree keep working, subject
  to the migration decision below.

---

## Open decisions

Each of these blocks a goal and should be settled before the stage that needs it.

1. **The side-effect sublanguage** (G8, [L13](AUDIT.md#l13)) — cut, reduce, or keep and
   document. Everything else about the file's shape follows from this one.
2. **Named-hole syntax** (G4) — `<system>` is the obvious candidate and keeps the left-hand
   side looking like prose; `$ident` is unavailable. Whether names are declared in the
   pattern or inferred from the literal around them is the real question.
3. **`$N` migration** (G4) — hard break, both forms with a deprecation warning, or both forms
   indefinitely. Seven grammars in the tree contain **235 positional references** between
   them (`ears` 62, `import-scripts/dbc` 46, `doc/cloud/ctrlpanel/dbc` 43, `test/cepslexer/dbc`
   40, `log` 23, `gherkin` 17, `data` 4). All seven are ours, which makes a hard break
   possible — and makes a mechanical rewrite worth considering, since the engine knows each
   rule's pattern and could print the named form.
4. **How deep UTF-8 goes** (G5) — byte-transparent identifiers (cheapest, correct for the
   cases that matter), codepoint-aware, or Unicode identifier classes. The first is probably
   right and should be chosen on purpose rather than by default.
5. **The form of the unmatched report** (G3) — a ceps node, a count and exit code, a
   side-channel listing, or some combination. It should be whatever lets a transform walk it,
   because that is how everything else in ceps is consumed.
6. **Blank lines** ([L11](AUDIT.md#l11)) — `get()` collapses runs of newlines, so a paragraph
   break is invisible. Keep as a named pragma, or make it visible. Requirements documents use
   blank lines structurally.

---

## A proposed order

The order is chosen so that each stage makes the next one cheaper, and so that the two stages
that change no visible behaviour come first.

| Stage | Goal | Why here |
|---|---|---|
| 1 | **G1** — safety | Pure containment, no behaviour change. Everything after this is edited in a file that cannot corrupt memory while you experiment. |
| 2 | **G9** — the corpus | Must precede every behavioural change, or there is no way to know a change was behavioural. Built under sanitizers, so it also enforces stage 1. |
| 3 | **G2** — positions and messages | Gives G3 and G5 somewhere to report *to*. Nothing downstream is pleasant without it. |
| 4 | **G5** — text | Highest-severity finding. Needs G2 to report malformed input; needs G9 to prove the ASCII path did not move. |
| 5 | **G8** — the decision | Settle it before G4, because a cut would remove `$N`'s only competitor for the `$` sigil and may change what naming should look like. |
| 6 | **G4** — named holes | The user-visible improvement. Cheap once G2 exists, since the diagnostics it needs are already there. |
| 7 | **G3** — partiality | The new capability, and the one that makes the component worth more than it costs. Last of the substantive stages because it benefits from all of them. |
| 8 | **G10**, [L12](AUDIT.md#l12) | Allocation and dead weight, once the shape has stopped moving. |

**G6 and G7 are not stages.** G6 runs at every stage as a veto. G7 is the obligation attached
to each stage that changes the format.
