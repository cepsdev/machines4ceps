# Goals: reworking the `.ceps.lex` engine

Companion to `AUDIT.md`, which records what `core/include/transform/streamtransform.hpp` does
today. This document states what it should become, in terms concrete enough to tell when each
goal is met. `PRIOR-ART.md` records the literature behind G11 and G12 — the channel stack is
a published architecture, and the goal is to build it from that work rather than rediscover
it.

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
how large a grammar is, how deeply a grammar recurses, or whether a file ends in a newline.

**Why.** Five of the nineteen findings are this ([L1](AUDIT.md#l1), [L3](AUDIT.md#l3),
[L4](AUDIT.md#l4), [L5](AUDIT.md#l5), [L18](AUDIT.md#l18)), with one shared cause: a size
assumed rather than checked. One of them — L1 — is reachable from a supplier's `.dbc` file.

**Met when.**

- No fixed-size stack buffer is indexed by a length taken from input. Numeric conversion uses
  `std::from_chars` over a pointer range, so the buffer in question ceases to exist.
- Every array indexed by a count that comes from elsewhere either grows or refuses, and
  refusing produces a diagnostic rather than a corruption.
- The input buffer carries a sentinel byte, and every scanning loop tests its bound before it
  dereferences.
- Exceeding the layer-stack ceiling is reported and names the recursing lexer, rather than
  indexing `lex_envs_` out of range ([L18](AUDIT.md#l18)). Depth is input-driven in any
  grammar that recurses, so this is a bound on data, not on the grammar author.
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
to — which is why it sits this early despite being rated High rather than Critical. G11 makes
it a hard precondition: a position must survive a rewrite, or traceability dies at the first
channel.

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

**Note.** G11 promotes this from a capability to a precondition twice over. Once because a
stack of channels is undebuggable without a per-channel remainder — a fault in channel 1
presents as silence in channel 3. And once because *pass-through* unmatched input is the
mechanism channels are built from, which makes this goal and G11 two readings of the same
thing.

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
0-based engine, and shows a dialect no grammar in the tree uses — and whose algorithm sketch
specifies a fixed-point re-feed the engine does not perform. It has been wrong for as long as
it has existed, because nothing could notice.

And it is incomplete in ways the audit only found by reading a grammar rather than the
engine: `|` as a guard separator ([L14](AUDIT.md#l14)) and identifier splicing
([L15](AUDIT.md#l15)) are both in production use in `dbc.ceps.lex` and appear in no
description of the format anywhere.

**Met when.**

- The header comment's invented example **and its algorithm sketch** are deleted. Both
  describe an engine that does not exist.
- Every construct the engine accepts is described — including the ones found only by reading
  the grammars: `|`, `\|`, `$N` in name position, `call`, `rewind exit`, and the pattern and
  action reserved words.
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

**But *cut* is not a free option.** `read_id_list` in `dbc.ceps.lex` genuinely needs it — and
reading that rule tells you what for. The guard and the assignment are there to **terminate a
list**: the pattern language cannot say *repeat while there is a comma*, so the grammar hand-
builds a one-bit state machine. The sublanguage is load-bearing, but what it stands in for is
**iteration** — a much smaller feature than sixteen node kinds, a parser and a symbol table.
That reframes the decision: the honest comparison is not *keep versus cut*, it is *keep a
general evaluator versus add the one construct that was missing*.

**Met when** — one of the following is true and written down:

- it is replaced by a repetition construct in the pattern language, which is what the one
  real use needs; or
- it is cut, and guards are expressed some other way (or not at all); or
- it is reduced to guards over matched tokens with no mutable store, documented and covered by
  the corpus; or
- it is kept whole, documented, covered, and the case for the fourth mechanism is made
  explicitly; or
- it is compiled rather than interpreted (G12), which removes the second *evaluator* while
  keeping the feature.

**The goal is the decision, not any particular outcome.** What is not acceptable is a quarter
of the file whose status nobody can state.

---

## G9 — The corpus exists

**Statement.** One command runs every grammar in the tree against fixed inputs and compares
against recorded output.

**Why.** There is no *automated* test for this file, and the one thing closest to a corpus is
not run by anything. `test/cepslexer/` holds two grammars and an import script, untouched
since March 2024, with **no input document, no expected output and no runner** — the intent
existed and stalled.

But `import-scripts/dbc/` is further along than that, and is the obvious seed:

```
$ cd import-scripts/dbc && ./run.sh      # ceps common.ceps dbc.ceps.lex frames.dbc …
(STRUCT "dbc_msg" (STRUCT "canid" "601" ) … )
$ echo $?
0
```

A 114-line grammar with ten sub-lexers, a 48-line input, and a runner — it works today.
What it lacks is an *expected* output to compare against, which is the whole of the gap.

**And it already shows why the gap matters.** Of the eleven rules in that grammar's `BEGIN`
block, `frames.dbc` exercises six. The five that never fire include

```
 BA_DEF_DEF "GenMsgCycleTime" integer ; => dbc_default_cycle{$2} .
```

— which is missing the `;` that every sibling rule has inside its braces. Nobody has noticed,
because nothing runs it.

A corpus would have caught [L1](AUDIT.md#l1) (128 digits), [L7](AUDIT.md#l7) (`suffix()` was
never callable), [L14](AUDIT.md#l14) (a rule that is accepted and never fires), `D25`'s
`$99`, and would catch [L2](AUDIT.md#l2) the first time anyone wrote an umlaut.

**Met when.**

- Each case is a grammar, an input, and the expected ceps — comparison by text.
- `import-scripts/dbc/` is one of the cases, with its current output recorded as the
  baseline, and extended until every rule in the grammar fires at least once.
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

**Deliberately late.** It is the cheapest goal here and would be the least important — except
that G12 makes it a precondition: there is no point compiling an algorithm that allocates per
token. Do it after the shape has stopped moving and before anything is retargeted.

---

## G11 — Channels

**Statement.** A grammar's output is another grammar's input. Channels stack, the order of
the arguments is the stack, and **the channels coexist** — channel *k+1* does not wait for
channel *k* to finish.

> **This architecture is published.** Abney's *Partial Parsing via Finite-State Cascades*
> (1996) describes it exactly, down to the pass-through rule for unmatched tokens, and calls
> it "little more than a pipeline of Lex-style lexical analyzers". See
> [`PRIOR-ART.md`](PRIOR-ART.md) — which is good news, because it means this goal has a
> design manual. Note also the naming collision with ANTLR's unrelated `channel()`.

```
ceps normalise.ceps.lex ears.ceps.lex model.ceps.lex door_lock.req extract.ceps simulate.ceps
       └─ channel 1 ─┘  └─ channel 2 ─┘ └─ channel 3 ─┘
```

Channel 1 reads the document. Channel *k+1* reads the token stream channel *k* emits, **as
it emits it**. The last channel's output is the ceps the transforms see. The sentence keeps
the reading it already has — order is composition — with one more layer in the sandwich.

### Why: it buys expressiveness without buying a metacharacter

EARS in the wild varies where it does not matter. `shall` / `must` / `will`. `When` / `Upon`.
`initialise` / `initialize`. These are the same requirement.

Today they are not. Against the shipped grammar:

```
$ echo 'REQ-001 The DoorLock must initialize in the unlocked state.' > v.req
$ ceps ears.ceps.lex v.req --pe
(STRUCT "incose_nonconforming" …
    "[REQ-001] NON-CONFORMING: matches no accepted pattern, no model element generated"
```

A correct requirement, rejected over a modal verb and a letter.

The pattern language has no alternation, so the only repair available inside one grammar is
to write the product: five EARS patterns × three modals × two spellings is **thirty rules**,
and a fourth modal makes it forty. Adding alternation would fix it and cost
[G6](#g6--the-grammar-stays-readable-to-the-person-who-owns-the-requirements) — a
metacharacter is a character that does not mean itself.

A channel fixes it with **more literals**, which is exactly what the inverted default is good
at. (In the three grammars below, `<name>` is G4's proposed named hole and
`%passthrough_unmatched` is the proposed pragma from decision 2 — neither exists today. The
rest is current syntax.)

**Channel 1 — `normalise.ceps.lex`**

```
%ext_req
%ignore_ws
%passthrough_unmatched
%%
BEGIN{
 must       => shall .
 will       => shall .
 Upon       => When .
 initialize => initialise .
}
%%
```

Thirty rules collapse to five rules and four lines of vocabulary — and those four lines are
reviewable by the person who owns the requirements, which is the whole of G6.

### This is not hypothetical — it is already in the tree

The EARS case above is a projection. The same explosion has already happened in
`import-scripts/dbc/dbc.ceps.lex`, where two sub-lexers are each **four rules for two
optional minus signs**:

```
lexer read_signal_factor_offset{
 ( any, any )     => scale{$1;};  offset{$3;};.
 ( - any, any )   => scale{-$2;}; offset{$4;};.
 ( any, - any )   => scale{$1;};  offset{-$4;};.
 ( - any, - any ) => scale{-$2;}; offset{-$5;};.
 else => rewind exit .
}
```

`read_signal_min_max` is the same four rules with `[ … \| … ]`. Two optional signs give four
rules; a third number would give eight. Note also that the `$N` indices shift in every
variant — `$1`/`$3`, then `$2`/`$4`, then `$1`/`$4`, then `$2`/`$5` — which is `D25` and G4
seen from a second angle: the duplication is not just long, it is *error-prone in a way the
author cannot see*.

A channel removes it. A signed-literal channel below this one:

```
 - <n> => <negated n> .
```

and `read_signal_factor_offset` becomes one rule:

```
 ( <scale> , <offset> ) => scale{<scale>;}; offset{<offset>;};.
```

Four rules to one, in a grammar that shipped. Optionality is a metacharacter (`?`) just as
alternation is (`|`) — and [L14](AUDIT.md#l14) is a live demonstration of what the engine's
*existing* `|` metacharacter costs. **A channel buys the same expressiveness and adds no
character that means something other than itself.** That is the thesis of this goal, and the
DBC grammar is the evidence for it.

### Why: each channel can be replaced

**Channel 2 — `ears.ceps.lex`** keeps one job: English sentence shapes to neutral requirement
nodes. It stops knowing anything about state machines. (Shown in G4's named holes.)

```
 <prefix> - <num> The <sys> shall initialise in the <st> state \. =>
   requirement{
     id{double_quote <prefix> - <num> double_quote;};
     form{ubiquitous;};
     system{<sys>;};
     state{<st>;};
   }; .
```

**Channel 3 — `model.ceps.lex`** maps requirement nodes onto a model vocabulary. Its patterns
match channel 2's **token stream**, so braces and semicolons are literals like everything
else:

```
 requirement { id { <id> ; } form { ubiquitous ; } system { <sys> ; } state { <st> ; } } ; =>
   incose_init{ req{<id>;}; system{<sys>;}; state{<st>;}; }; .
```

That is more verbose than channel 2, and the document should not pretend otherwise. It is
also self-limiting: a channel whose patterns grow brace-heavy is a channel doing too much.
**The verbosity is the pressure that keeps channels thin**, and thin channels are the point.

What it buys: channels 1 and 2 are about *English*. Channel 3 is about *your model*. Swap
channel 3 and the same requirements produce SCXML, or a different house vocabulary, without
touching the grammar that understands the sentences.

### This is a pipeline, not a sequence of passes

The obvious reading of a stack is **batch**: channel 1 consumes the document and produces a
stream, then channel 2 consumes that, and so on. That reading is wrong, and taking it costs
both of the things the design is for.

The right reading is a CPU's execution pipeline. Channel 1 emits its first token; channel 2
starts on it while channel 1 is still reading line 1. Every channel is live at once, each
one a stage.

**Two distinct wins follow, and they should not be conflated:**

| | batch passes | coexisting stages |
|---|---|---|
| Memory | the whole intermediate stream at each of *N*−1 boundaries | O(window) — one bounded queue per boundary |
| Latency to first output | every stage over the whole input | one token through *N* stages |
| Throughput, one core | 1/Σ(stages) | 1/Σ(stages) — **unchanged** |
| Throughput, *N* cores | 1/Σ(stages) | **1/max(stage)** |

The first two rows are won by coroutines alone, single-threaded: the total work is identical,
but nothing is ever materialised and the first result is available immediately. The third row
is the one to be honest about — **pipelining buys no throughput on one core**, because the
work is still the sum of the stages. Only genuinely concurrent stages turn that sum into a
maximum, and then the slowest channel sets the rate.

This retracts an estimate made earlier in this document's history: that a five-channel stack
would run at a fifth of the present speed. That was batch arithmetic. In a pipeline the
intermediate streams never exist, and on separate cores five thin channels can beat one fat
one.

### Three consequences, one of which closes an open decision

**1. The inter-channel interface is forced, not chosen.** Open decision 7 asked whether
channels should exchange token streams or ASTs. A pipeline answers it: **you cannot stream a
tree.** An AST is complete only when its closing brace arrives, so an AST boundary is a
barrier, and a barrier is a batch pass. A token stream is the only interface that keeps the
stages live — which is the same answer *one mechanism, repeated* gave for entirely
independent reasons. When the aesthetic argument and the mechanical argument agree, the
decision is made.

**2. Backtracking is branch misprediction — and the invariant that avoids it already holds.**
If a channel emits a token and then rewinds, everything downstream has already consumed
something that is about to be re-derived, and must be flushed. That is the one genuinely
expensive event in a pipeline, and a flush-and-rollback protocol at every channel boundary
would be a serious tax on a design whose selling point is thin channels.

It is not needed, because **emit XOR rewind holds across the whole tree**: all 25 occurrences
of `rewind` in the four grammars that use it are `rewind exit`, and not one of them emits. A
rule that hands input back produces no output. *A rule firing is a commit*, and a commit
never needs un-sending.

But it holds **by habit, not by rule**. The engine accepts an action that emits *and*
rewinds, and performs both, leaving two nodes behind for one input token
([L17](AUDIT.md#l17)). So this is a **precondition with work attached**: make emit-XOR-rewind
a checked invariant, rejected when the grammar is read, *before* anything depends on it. No
grammar in the tree would change. See also [L16](AUDIT.md#l16), the neighbouring case —
`rewind` without `exit`, which does not terminate at all.

**3. Streaming input becomes a first-class case.** `examples/analyze_logs/log.ceps.lex` reads
logs; today it reads a *file* of them. With coexisting channels and O(window) memory there is
no reason the input must end. A log stream, a CAN trace, a serial port — the stack runs
continuously and the last channel emits model updates as it recognises them. That is a
different product from a batch importer, and it falls out of the mechanism rather than being
added to it.

### Where the CPU analogy stops

A CPU pipeline has fixed-length stages and a clock. Channels have neither: one may consume
ten tokens to emit one, the next may emit three for every one it reads, and the ratios vary
with the input.

So the implementable form is a **dataflow network with bounded queues**, not a clocked
pipeline — each channel a coroutine reading an upstream queue and writing a downstream one,
blocking when its output queue is full. Queue depth becomes a real design parameter: too
shallow and the stages stall against each other, too deep and the memory win evaporates. It
is the one number this design cannot derive from first principles, and will have to measure.

[`PRIOR-ART.md`](PRIOR-ART.md) holds the nearest published relative. **Mohri 1997** composes
transducer cascades on the fly, and has been in production since 1996 — but it lazily expands
a single *fused* transducer rather than running *N* concurrent stages. Whether N live stages
beat one lazy fused one is genuinely open, and is the most research-shaped question in this
document.

### Three decisions this still needs

The fourth — what a token is between channels — is closed above: a pipeline cannot carry a
tree, so the interface is the token stream.

1. **The default disposition of unmatched input must invert.** Today `any => .` and
   `%skip_unmatched_tokens` both discard. A channel must **pass unmatched tokens through
   unchanged**, or channel 1 destroys the document before channel 2 can see it.

   This is [G3](#g3--nothing-is-understood-partially-in-silence)'s remainder used as a
   *mechanism* rather than as a *report* — and it unifies two ideas that looked separate:
   **channels with pass-through subsume the remainder pipeline.** Running a second grammar to
   cover more of the same document is just a second channel that ignores what the first
   already cooked. Vertical composition (raising abstraction) and horizontal composition
   (raising coverage) are the same stream, differing only in what the next channel chooses to
   look at. One mechanism, repeated — which is the test in *The yardstick*.

2. **Termination.** A finite stack terminates by construction: **the stack is the proof.**
   This is why channels are the right form of the idea and the header comment's self-re-feed
   ([L7](AUDIT.md#l7)) is the wrong one — under a fixed point over the *same* rule set,
   `a => a a .` diverges and nothing in the engine would stop it. A channel stack is also
   inspectable: you can tap the stream between channel 2 and channel 3, which you can never
   do inside a fixed-point loop. In a pipeline that tap is a live view rather than a dump.

   Termination *within* a single channel is a separate matter, and is not guaranteed today —
   see [L16](AUDIT.md#l16).

3. **Provenance must survive a rewrite.** If channel 1 turns line 7 into tokens and channel 3
   rejects them, the engineer needs *`door_lock.req`, line 7, REQ-001*. Position has to
   travel with the token through every rewrite, or traceability — the thing the INCOSE
   example sells — dies at channel 1.

   A pipeline makes this harder rather than easier: there is no moment at which a whole
   intermediate stream exists to be re-scanned for positions, so position must ride **on the
   token** or be lost. Deciding this late would be expensive, because it is a change to the
   token representation that every channel sees.

**Met when.**

- Several `.ceps.lex` arguments form a stack, and order is the composition.
- The channels run concurrently: no intermediate stream is ever fully materialised, and the
  first output appears before the input has been fully read.
- Peak memory is independent of input length, demonstrated on an input that does not end.
- Emit-XOR-rewind is enforced when a grammar is read, so no channel boundary needs a flush.
- `ears.ceps.lex` carries no per-synonym duplication, and modal and spelling variation is
  handled by a normalisation channel of a few readable lines.
- A channel can be replaced without touching its neighbours, demonstrated by retargeting
  channel 3 to a second model vocabulary.
- Unmatched tokens pass through by default, and each channel can report its own remainder.
- A token's origin survives the whole stack and appears in `incose_traceability`.

**G2 and G3 are preconditions, not companions.** Without positions, traceability dies at
channel 1. Without per-channel remainder, a fault in channel 1 presents as silence in channel
3 with nothing in between to look at. Both move ahead of this goal in the order of work for
that reason.

---

## G12 — A compilation target

**Statement.** The matcher should be compiled rather than interpreted, and Oblectamenta is the
right target — **after** the algorithm is settled, not before.

### The measurement

Today, `ears.ceps.lex` over generated requirement corpora:

| input | wall clock |
|---|---|
| 0.58 MB | 0.98 s |
| 5.80 MB | 9.70 s |

Differencing to cancel start-up: **5.22 MB in 8.72 s ≈ 0.6 MB/s.** Linear, so nothing is
quadratic — it is simply far from the hardware: about 1.7 µs per input byte, on the order of
**6,700 cycles per byte** at 4 GHz.

Now `vm/benchmarks/counting_a.ceps`: 10 million iterations of an eight-instruction loop in
0.360 s — roughly **220M instructions/s, ~18 cycles per instruction**, a sound bytecode
interpreter. A DFA step is perhaps eight to ten instructions (`ldsi8`, widen, index, compare,
branch, advance), which lands at **20–40 MB/s**.

> **Compiled to Oblectamenta and interpreted, with no JIT at all, the lexer would be on the
> order of fifty times faster than it is today in hand-written C++.**

### What that number actually says

It is the argument for compiling, and simultaneously the argument against treating the JIT as
the lever. Those 6,700 cycles are not instruction count. They are the per-token heap
allocation ([L9](AUDIT.md#l9)), the linear scan over every rule at every position, the
bounds-grow check inside `matched_tokens()` on every call, and `Tokentable::match` walking 31
operators per punctuation character. A native DFA reaches 200–1000 MB/s. Past the algorithm,
the VM lands near 40 and a JIT perhaps near 150 — **both still behind plain C++ done right.**

### So the case for Oblectamenta is not speed

- **[L13](AUDIT.md#l13) dissolves.** The side-effect sublanguage stops being a second
  interpreter and becomes code generation into an existing target. This is a third answer to
  open decision 1, and a better one than cut-or-keep.
- **The substrate count drops.** ceps executes five ways today: the tree-walking evaluator,
  the lexer's table interpreter, the side-effect interpreter, the VM, and the C++ generator.
  [G8](#g8--one-mechanism-repeated) argues for fewer.
- **Channels become calls.** Each channel a compiled function, the stack a call sequence —
  composition supplied by the VM rather than built a second time.
- **A compiled grammar is disassemblable.** You can show an author what their rules became.
- **The JIT, when it lands, is inherited rather than ported.**

### Three frictions, named now

- **`callx` is not declarable.** The indirect call a compiled matcher needs for dispatch is
  implemented but not declared in the assembler — `DEFECTS.md` D1. The instruction this
  project most needs is the one that currently cannot be emitted. D15, where the assembler
  rejects guard expressions the language admits, sits on the same path.
- **The swap zoo.** The opcode tail reads `swpi64b72`, `swp128b8`, `swpi160i64`,
  `swpi192i32`. Variants that specific are a stack machine reporting that its data flow
  hurts. A matcher carries position, token start, active rule set and match state at once;
  expect to extend that list, and count each addition as a cost rather than a feature.
- **Zero copy.** Tokens are `nonmutable_string` — pointers into the input buffer, never
  copied. `ldptr`/`stptr` suggest this survives a move to the VM, but it should be confirmed
  before committing, because losing it would undo the win.

**Met when.**

- Rule matching is generated code, not a table walked by a loop.
- Guards are compiled and `Sideeffects_node` is gone — G8 settled by construction.
- `callx` is declarable, or dispatch is designed without it.
- Throughput is a recorded number before and after, on the same corpus, so the claim is a
  measurement rather than an expectation.

**Ordering rule: only retarget an algorithm you are happy with.** Compiling the present
matcher would freeze 0.6 MB/s into bytecode. Remove the per-token allocation and the linear
rule scan first — in plain C++, where it is cheap and reversible — then compile the result.

### A convergence worth noting

The inverted default makes the fast algorithm *easier*, not harder. Patterns dominated by
literal tokens are exactly what multi-pattern automata — Aho–Corasick and its relatives —
consume: linear in the input, independent of the number of rules. G6 and G12 point the same
way, which is usually a sign that both are right.

### And one thing that is not an opportunity

If G11 lands, it is tempting to think that compiling the stack would allow the channels to be
**fused** — N grammars collapsed into one automaton, N passes for the cost of one. That is a
settled result, not a discovery: Kaplan & Kay (1994) state it outright — *"eliminate the
cascade altogether … compose these individual rule transducers into a single machine"* — on
a foundation going back to Elgot & Mezei (1965). See [`PRIOR-ART.md`](PRIOR-ART.md) §2.

Apply it, with the citation. Do not spend anything on it as if it were new.

---

## Non-goals

Stated so that scope does not drift during the rework.

- **Not a general parser generator.** No context-free grammars, no ambiguity resolution, no
  left recursion, no precedence declarations. The layer mechanism plus longest-match is the
  whole model, and G6 is the reason to keep it that way.
- **Not a regular-expression engine.** Character classes and alternation beyond the existing
  token kinds would buy expressiveness at exactly the cost G6 forbids. G11 is the alternative:
  buy the same expressiveness with another channel instead of another metacharacter.
- **Not an evaluator.** ceps evaluates; `extract.ceps` walks the model. Anything the engine
  computes is a thing computed twice, in two semantics, that will drift. This is G8 stated
  from the other side — and G12 is the same point again: if guards must execute, they should
  execute on the machine ceps already has.
- **Not incremental or streaming.** Reading the whole file into one buffer is what makes
  `state_t` a pair of integers and backtracking a value copy. That is worth more than
  streaming.
- **Not a new file format.** The seven `.ceps.lex` grammars in the tree keep working, subject
  to the migration decision below.

---

## Open decisions

Each of these blocks a goal and should be settled before the stage that needs it.

1. **The side-effect sublanguage** (G8, [L13](AUDIT.md#l13)) — cut, reduce, keep and
   document, **replace with a repetition construct**, or compile it (G12). Everything else
   about the file's shape follows from this one. Its only real use in the tree is terminating
   a list, which makes the repetition option the one to price first.

   **The size of this decision has been measured, and it is small.** `dbc.ceps.lex` contains
   seven `|` characters. Five are *escaped literal pipes* — DBC's own `[min|max]` and
   `startbit|width@` syntax — and are not guards at all. The grammar has exactly **two**
   guarded rules, `:35` and `:36`, and together they express one thing:

   ```
    ident ,                    => $0; .
    ident | !read_id_list_end  => $0; /read_id_list_end=1;/ .
    any   | read_id_list_end   => rewind exit .
   ```

   That is `list(ident, ",")`, hand-rolled as a state machine with a one-bit flag, because
   the engine has no separator construct. `ears.ceps.lex` contains **zero** guards. So the
   whole side-effect sublanguage — a second language inside the first, undocumented, and the
   cause of [L19](AUDIT.md#l19) — is load-bearing for two rules, and a repetition construct
   retires both.

   Two findings and one capability ride on this. [L19](AUDIT.md#l19) disappears with the
   feature. [L14](AUDIT.md#l14) loses its subject, since `|` stops being overloaded. And
   [`BIDIRECTIONAL.md`](BIDIRECTIONAL.md) shows that these same two rules are the only thing
   standing between `dbc.ceps.lex` and an invertible grammar — which is the capability that
   would have answered the objection the project has met in the field for years: *we can get
   our DBC into ceps, but not back out.*
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
7. ~~**What flows between channels** (G11) — token stream or AST.~~ **Settled: the token
   stream.** Not by preference but by construction — channels coexist, and an AST boundary is
   a barrier, so a tree interface would silently turn the pipeline back into batch passes.
   See *This is a pipeline, not a sequence of passes* in [G11](#g11--channels). Kept here
   with its resolution rather than deleted, because the *reason* is the useful part.
8. **The default disposition of unmatched tokens in a channel** (G11) — today it is discard,
   via `any => .` and `%skip_unmatched_tokens`. Channels need pass-through, and pass-through
   is what makes channels subsume the remainder pipeline. Whether the default inverts
   globally or only for non-final channels is the question.
9. **The concurrency model between channels** (G11) — coroutines on one thread, or a thread
   per channel. Coroutines buy the memory and latency properties and are far simpler;
   only threads convert Σ(stages) into max(stage). The honest order is coroutines first,
   threads only if a measurement asks for them. Queue depth is the attached parameter and
   is the one number in this design that cannot be derived, only measured.
10. **What the component is called.** ~~Open.~~ **Settled: `ceps-transduce`.** It is not a
    lexer: `call` pushes a real stack, and the engine is demonstrably beyond regular — see
    *What this component actually is* in [`AUDIT.md`](AUDIT.md). The implementation is
    already named correctly (`transform/streamtransform.hpp`); only the surface says
    `.ceps.lex` and `lexer NAME{…}`.

    The cost of the current name is not aesthetic. A lexer is expected to terminate, not to
    recurse, and to be regular — which is why nothing checks termination
    ([L16](AUDIT.md#l16)), nothing bounds the recursion ([L18](AUDIT.md#l18)), and the stack
    is documented nowhere. The name shaped what nobody thought to ask.

    **Why `transduce` and not `parse` or `rewrite`.** A transducer relates two languages; it
    does not privilege a direction. `parse` names the forward direction and makes the reverse
    look like a separate tool, which is exactly the framing that kept `ceps → dbc` off the
    roadmap for years. The name should describe the *relation*, because
    [`BIDIRECTIONAL.md`](BIDIRECTIONAL.md) argues the relation is the object of interest and
    both directions are readings of one grammar.

    **What remains open is the migration, not the name.** The surface can be renamed (seven
    grammars and their run scripts change, plus `%ext_*` handling), `.ceps.lex` can be kept
    as an alias with the real name introduced alongside it, or the name can stay while the
    documentation tells the truth. The third is free, is already done in AUDIT.md, and holds
    regardless. Worth settling before G11 — *channel* only reads naturally once the thing
    being stacked has an honest name.

---

## A proposed order

The order is chosen so that each stage makes the next one cheaper, and so that the two stages
that change no visible behaviour come first.

| Stage | Goal | Why here |
|---|---|---|
| 1 | **G1** — safety | Pure containment, no behaviour change. Everything after this is edited in a file that cannot corrupt memory while you experiment. |
| 2 | **G9** — the corpus | Must precede every behavioural change, or there is no way to know a change was behavioural. Built under sanitizers, so it also enforces stage 1. |
| 3 | **G2** — positions and messages | Gives G3 and G5 somewhere to report *to*. Nothing downstream is pleasant without it. It also unlocks the two cheapest fixes in the audit: [L16](AUDIT.md#l16) and [L17](AUDIT.md#l17) are rules the engine can prove wrong when it reads them, and both need a position to report. |
| 4 | **G5** — text | Highest-severity finding. Needs G2 to report malformed input; needs G9 to prove the ASCII path did not move. |
| 5 | **G8** — the decision | Settle it before G4, because a cut would remove `$N`'s only competitor for the `$` sigil and may change what naming should look like. |
| 6 | **G4** — named holes | The user-visible improvement. Cheap once G2 exists, since the diagnostics it needs are already there. |
| 7 | **G3** — partiality | Needs G2 to report *where*. Promoted from "nice capability" to precondition by G11: a channel stack without per-channel remainder fails in channel 1 and presents as silence in channel 3. |
| 8 | **G10**, [L12](AUDIT.md#l12) | Allocation and dead weight, once the shape has stopped moving — and before G12, because compiling a per-token allocator freezes it. |
| 9 | **G11** — channels | The capability that makes the component worth more than it costs. Last of the language stages because it consumes G2, G3 and G4 and is not safe to attempt without them — and because it rests on emit-XOR-rewind ([L17](AUDIT.md#l17)), which must be an enforced invariant before a pipeline leans on it, not after. |
| 10 | **G12** — compilation target | Only retarget an algorithm you are happy with. Everything above is what makes it one. |

**G6 and G7 are not stages.** G6 runs at every stage as a veto. G7 is the obligation attached
to each stage that changes the format.
