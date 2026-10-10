# Abstract

Working notes for the paper. Not the paper. The job of this file is to fix the thesis,
the related work, and the cost — in that order, because the cost is the part that decides
whether the rest is believed.

> **Every stage of compilation is written in the notation the programmer wrote, in the
> document the programmer wrote it in, and no stage is discarded.**

---

## Draft abstract

> Compilers discard. A front end parses a surface syntax into an intermediate
> representation and the surface syntax is gone; each subsequent stage replaces its
> predecessor, and by the time code is emitted nothing of what was written survives in a
> form its author could read. The intermediate representations are the compiler's, not the
> program's.
>
> ceps is built the other way. Lowering is *additive*: each stage introduces vocabulary
> into the same document rather than translating the document into a new one, so a
> specification, the state machine that realises it, the assembler that the state machine's
> actions are written in, and the encoding of those instructions are simultaneously present
> and mutually visible. A data section declared after a state machine is visible to actions
> inside it; the simulator and the assembler read the same declarations. There is no
> intermediate representation because the surface notation is the intermediate
> representation, and consequently every stage can be printed and read — `--pe` shows the
> program after evaluation in the notation it was written in, which is not something
> `-fdump-tree-gimple` can do.
>
> We describe the mechanism, which is small — a new stage requires a kind and a rewrite,
> no grammar, no parser, no printer, no IR — and we price it. Openness of vocabulary means
> an undeclared name does not fail to parse; it parses as something else. We give the
> defect class this produces, with a reproducer that prints a wrong answer and exits 0.
>
> The design has been in industrial use since 2014. We report a ten-year readability
> comparison from one such project, in which the state machines and the C++ they shipped
> alongside had the same author and were read a decade later by someone who wrote neither.

Roughly 300 words; needs cutting to ~200. The two paragraphs that must survive are the
price and the field observation — the mechanism can be compressed to a sentence, because
the mechanism is the part a reader will believe without being convinced.

---

## Title

`IR is the program` is the candidate. It is positive, it is accurate, and it survives the
first objection a reviewer will raise.

Two names are already owned and must not be used:

| Name | Owner | Why it costs a page |
|---|---|---|
| *Scribble* | Racket — Flatt's documentation language | A paper positioned against Racket, using Racket's word for something else |
| *programmable IR* | MLIR — extensible dialects, Lattner | Reads as an MLIR comparison; invites being measured on their axis, by a lone implementer |

`A compiler without IR` was considered and rejected. It is a negative claim, it invites
*"the eAST is an IR and you know it"*, and it files the work under compiler construction,
where the first question is about optimisation — a question ceps has no answer to and no
obligation to answer.

---

## The thesis chain

Each link is load-bearing and the order matters. State it alone and it is philosophy;
state it with the mechanism and it is a design.

1. **Nothing is discarded.** The scribbling principle, inherited from *yamdl*.
2. Therefore lowering must be **additive** — a stage adds vocabulary, it does not replace
   the document.
3. Therefore there is **no IR**: every arrow but the last is AST→AST. The encoder is the
   only representation change in the stack.
4. Therefore **every intermediate stage is readable**, because it is in the notation the
   programmer already knows.
5. Therefore the pipeline is **open**: a new stage costs a kind and a rewrite.

Point 4 is the one to lead with for an audience, because it is the one they have felt.
Everyone who has tried to work out what a C++ construct actually means has been defeated
by a pipeline that hides its middle. C++ has dumps — `-E`, `-fdump-tree-*`, cppinsights —
and they print a different notation, so what you read is the compiler's internals and not
your program. GIMPLE is not C++. `--pe` is legible *because* there is no IR. Those are the
same sentence.

---

## Related work

The ingredients are old. Nobody gets credit for ingredients; the composition is the claim.

| Prior art | What it shares | Where it diverges |
|---|---|---|
| **Lisp, 1958** | homoiconicity — code and data in one notation | homoiconic for *source*; ceps is homoiconic across *lowering* |
| **Self '87 / JavaScript '95** | open, retroactively extensible vocabulary | extends *objects*; ceps extends the *pipeline* |
| **Literate programming, 1984** | one document in several roles | prose and code; ceps spans requirement to encoding |
| **Racket `#lang`** | cheap new languages | a *tower* — macros expand into a core and the layers disappear; in ceps nothing disappears |
| **Stratego/XT, Rascal** | program transformation by rewriting | transformation between artifacts; ceps accumulates within one |
| **MLIR** | extensible, multi-level lowering | dialects are compiler-facing; the programmer never reads them |
| **C++ overload sets / ADL** | open set, extended later, resolved at use | the nearest thing in a mainstream language — *and it misbinds in silence* |

The one-sentence placement: **Lisp made source and data the same notation; ceps makes
source and every intermediate stage the same notation.**

### On LLVM

Worth a paragraph, carefully, because the strong form is defensible and the slogan is not.

LLVM made everything below the AST free, so the cheap move became *new surface syntax,
same semantics*, and the proliferation has been in syntax. The deeper point: LLVM IR is
not neutral. SSA, flat memory, C calling conventions — targeting it means accepting a
model of computation, so languages wanting a *different* model get less benefit and the
gradient pushes toward C. LLVM standardised semantics in the layer nobody writes anymore
and therefore nobody examines.

"It killed language design" is too strong, and the counterexamples are instructive: Swift
and Julia both inserted their *own* IR above LLVM — SIL, Julia's typed IR — precisely
because LLVM IR could not carry their semantics. People do route around it. They route
around it with Apple and MIT behind them; a lone implementer takes it as given. That is
the version to defend.

---

## The cost

This section is not a concession. It is the reason the paper is credible, and it belongs
in the abstract rather than in a *Limitations* paragraph on the last page. A reviewer's
job is to find the cost that was concealed; hand it over and the attack is gone.

**The mechanism and the defect are one decision.** If a layer's vocabulary is only a set
of kinds, then a name that was never declared does not fail to parse — it binds somewhere
else. D1, D17 and D24 are not three defects, they are one defect class seen three times.

The reproducer is [D17](DEFECTS.md#d17) (`DEFECTS.md:1012`): fourteen names —
`m metre meter s second kg kilogram celsius kelvin ampere cd candela mol mole` — resolve
to SI units rather than to whatever the author meant. A loop variable called `m` binds to
metre. Wrong answer, no diagnostic, exit code 0. `A`, `K` and `g` look like unit symbols
and are safe, so guessing which names are dangerous does not work.

A Racket reader will ask **"what is your hygiene story?"** within thirty seconds, and the
honest answer is that there isn't one — D17 *is* a hygiene failure, at the level of kinds
rather than bindings. That community spent twenty years on hygiene precisely because
unhygienic systems misbind silently. Write that section before a reviewer writes it.

### Where the loud fail goes

Not in the parser. A parser strict enough to reject an undeclared name also rejects
`A + B;`, and transient state is a feature, not an oversight. The test is **consumption,
not declaration**: a symbol that reaches a consumer expecting a kind and finds a bare ID
is an error *at that boundary*. Each layer brings its own checker — same bootstrap as
everything else. The assembler already does this correctly for unknown opcodes
(`core/src/vm/oblectamenta-assembler.cpp:1435-1436`).

---

## The precedent nobody wants to be compared to

**Perl is the nearest thing to a shipped scribble language**: open vocabulary, additive,
context-sensitive, `AUTOLOAD`, symbol-table surgery, TIMTOWTDI. It allowed extraordinarily
compact and idiosyncratic code — you could recognise the author in the script — and its
reputational death was *write-only*.

The usual explanation is fashion, and there is something to it: regex, the densest
notation ever devised, was adopted by every one of the languages that called Perl line
noise — as a library, inside quotes, where it did not disturb the house style. Density was
not the crime.

But the charge was partly earned, and the mechanism is the one that matters here:

> **Ingenuity is locally driven. At each point you write what expresses *that* point well,
> and the why gets buried.**

That is the scribble's real gap, stated from the inside. A sheet preserves the marks and
not the reason the mark was made. Nothing is discarded except the only thing that was ever
in the author's head. `use strict` existed from 1994 and was **opt-in** — the loud fail,
available and off by default, exactly like JavaScript's thirteen years later.

**The answer, and it is testable.** Perl kept only the program. The claim here is that a
ceps document holds the requirement *next to* the machine that satisfies it, so the why is
not buried, it is adjacent. The experiment: take a `.ceps` file untouched for a year and
try to recover the intent from the document alone. If it works, this is Perl's compression
without Perl's amnesia, and it is a paper section. If it does not, better to find out
before a reviewer does.

---

## The evidence

The experiment above has already been run, at ten years rather than one, on an industrial
project — **Dingo TRGS** — and with a control.

**The design, which is stronger than the usual readability anecdote.** One engineer wrote
both artifacts: the ceps state machines *and* the accompanying C++. The reader, ten years
later, wrote neither — he wrote the tooling. So this is not someone finding their own code
readable, which tests nothing. It is a third party reading one author's work in two
notations, which is the configuration maintenance actually happens in.

**The outcome.** The ceps was recoverable without difficulty. The C++ was markedly less
enlightening.

**The objection to pre-empt**, because it will arrive immediately: *the ceps was the spec
and the C++ was the implementation, and specs are always more readable.* TRGS defeats it —
the ceps was compiled to C++ and ran on the ARMv7 target. Both artifacts were production.
State this in the same breath as the claim; unstated it looks like the confound rather
than the refutation.

**The remaining confound, and why it is smaller than it looks.** The reader is the
notation's designer. Not a C++ fluency gap — he writes C++ daily — but knowledge of ceps's
semantics that no reader of the C++ had. The rebuttal is measurable rather than rhetorical:

| | 2014-09-19 (`93c7a24`, the TRGS era) | today |
|---|---|---|
| nonterminals in `ceps.y` | **16** | 20 |
| total grammar lines | 564 | 790 |

The sixteen were `cepsscript id_list decl struct_decl struct_initialization expr
func_stmts if_then_else id_or_struct_id for_loop parameter_list parameter argument_list
raw_map raw_lines raw_line`. Six are plumbing. The whole surface has grown by four
productions in eleven years.

And the decisive detail is *which* of them the models used. `sm`, `states`, `t`, the `+`/`-`
enter/exit notation — **none of it is in the grammar**. It is all structs. The state
machines were written almost entirely in `struct_decl` and `expr`. So designer fluency
amounts to knowing about ten productions, two of which the artifact exercised.

That is the mechanism claim arriving as evidence rather than as argument: **the layers are
vocabulary, not grammar.** It is also why a second author could extend the notation — the
`+`/`-` enter/exit convention was the freelancer's invention, not the designer's. The
notation survived an author it was not designed by. Perl's did not.

**The blocker is clearance, not argument.** The artifacts are KMW's. Nothing here can be
quoted until that conversation has happened.

**Cheapest way to strengthen it:** one engineer who knows neither artifact, both
codebases, timed comprehension questions. Even n=3 moves this from experience report to
measurement. Same blocker.

---

## What is missing before this can be written

| Gap | Where |
|---|---|
| **The x64 backend is a stub** — empty loop body, `return {}`. Any claim of "down to machine code" is currently false; the design reaches the encoder and stops. | `core/src/vm/oblectamenta-assembler.cpp:1380-1387` |
| **No evaluation.** The natural measure is the cost of adding a layer: grammar rules added (zero), parser changes (zero), printer changes (zero). The msgdef schema and the x64 layer are the two worked cases. The grammar-growth figures in [The evidence](#the-evidence) are the other half — four productions in eleven years, while the vocabulary grew by an assembler, a VM and a serialiser. | — |
| **The loud fail is designed and not built.** The cost section is much stronger if it ends with a fix rather than a plan. | [ROADMAP.md](ROADMAP.md) |
| **A worked end-to-end example** small enough to print: requirement → state machine → action → assembler → encoding, in one file. | [ASM.md](ASM.md) §4 is the nearest existing candidate |

---

## Venue

A design paper, not a theory paper: **GPCE, SLE, Onward!** Not PLDI — there is no novel
mechanism in the sense PLDI means, and the contribution is a composition with a measured
price, which is what those three venues exist for.

---

## Provenance

The thesis is not retrospective, and this is unusually easy to demonstrate. The *yamdl*
documentation from 2013 already contains the Scribbling chapters, TopUp, and the sentence

> *"without being forced to throw away any scribbling sheets"*

— `~/dev/yamdl-orig/doc/mst_part_1.html`, repository initialised 2013-09-25. A design
paper whose thesis is checkable against a thirteen-year-old file is in a different
category from one with a tidy story assembled afterwards. Say so, and cite the file.

---

## Related documents

| Document | Contents |
|---|---|
| [POSITIONING.md](POSITIONING.md) | what ceps is for, and what it is free to refuse |
| [ASM.md](ASM.md) | the Oblectamenta assembler and data section — the layer this paper describes |
| [DEFECTS.md](DEFECTS.md) | the cost, with reproducers |
| [doc/scribble-concept/README.md](doc/scribble-concept/README.md) | the scribbling principle, worked |
