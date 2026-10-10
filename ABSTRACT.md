# Abstract

Working notes for the paper. Not the paper. The job of this file is to fix the thesis,
the related work, and the cost — in that order, because the cost is the part that decides
whether the rest is believed.

> **Runnable pseudocode and readable machine code, in one document.**

That is the opening line — the whole claim, on a slide, with no mechanism in it. The
clause *in one document* is not decoration: executable pseudocode and legible assemblers
both already exist separately, and the claim is that the two levels sit in the same file
and lower into each other.

Stated for a reader who wants the mechanism rather than the consequence:

> **Every stage of compilation is written in the notation the programmer wrote, in the
> document the programmer wrote it in, and no stage is discarded.**

Two cautions on the short form, learned by getting them wrong first. Say **MIX**, not
TAOCP — Knuth's prose is famously readable and it is the listings people skip, so "makes
TAOCP readable" insults the reviewer most likely to care. And the machine-code half is
**Oblectamenta**, which runs, until the x64 stub is filled. Keep Knuth and CLRS in
[the figure](#the-framing-figure), where they do analysis rather than carry the claim.

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
> comparison from one such project, its negative counterpart from a second, and a public
> MIT-licensed executable specification of ISO 15118-2 and 15118-3 — roughly 7000 lines of
> ceps — in which each requirement of the standard sits adjacent to the constant, event or
> state machine that realises it.

Roughly 320 words; needs cutting to ~200. The two paragraphs that must survive are the
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

## The framing figure

The introduction should open on an axis every reader already has opinions about, and there
is a canonical one: **how concrete should the code in a book about algorithms be?**

Knuth took the low position — MIX, and later MMIX, with exact operation counts. "3.5N + 12"
rather than O(N). That concreteness bought something CLRS genuinely cannot offer. Cormen
et al. took the middle: pseudocode, chosen deliberately, with error handling and software
engineering concerns explicitly out of scope.

Both positions have a bill, and the interesting one is Knuth's, because **it arrived as
maintenance**. Having bound the books to a concrete machine, he had to design MMIX, write
the simulator, publish Fascicle 1 in 2005, and rework decades of code — and the proportion
of machine-level presentation fell steadily from Volume 1 to 4A and 4B as the step-form
notation took over. The most careful author in the field spent years paying for a
commitment to a layer below the one his ideas lived at. That is this paper's thesis in its
most prestigious test case, and it was not chosen to flatter it.

CLRS pay the opposite bill: no quantitative precision, and the algorithms do not run.

Which makes the axis a 2×2, and the right-hand column is the claim:

| | doesn't run | runs |
|---|---|---|
| **middle** — behaviour as stated | CLRS pseudocode | **ceps state machines** |
| **low** — behaviour as executed | — | MIX / MMIX, **Oblectamenta** |

ceps does not occupy a cell. It occupies **the column** — `timing.ceps` is middle,
the data sections and opcodes of [ASM.md](ASM.md) are low, and an `Actions{}` block may
hold both. That is only possible because lowering is additive: the level is chosen
**per site, not once for the whole work**. Knuth's migration was expensive precisely
because his was a single choice binding everything.

*Honest caveat for the figure:* the low cell is the Oblectamenta VM, which runs. Native
x64 is designed and not built — see [What is missing](#what-is-missing-before-this-can-be-written).

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
| **Literate programming, 1984** | one document in several roles | the near miss: `tangle` and `weave` produce **two** artifacts, so the code that runs is *generated from* the document you read. Reconstituted by a tool, not simultaneous. ceps has no tangle step |
| **Racket `#lang`** | cheap new languages | a *tower* — macros expand into a core and the layers disappear; in ceps nothing disappears |
| **Stratego/XT, Rascal** | program transformation by rewriting | transformation between artifacts; ceps accumulates within one |
| **MLIR** | extensible, multi-level lowering | dialects are compiler-facing; the programmer never reads them |
| **C++ overload sets / ADL** | open set, extended later, resolved at use | the nearest thing in a mainstream language — *and it misbinds in silence* |
| **Harel statecharts, 1987** | the formalism the models are written in | Harel owns the formalism; the claim here is that hosting it cost **zero grammar productions** and left it co-resident with its data and actions |
| **Stateflow, Rhapsody, Yakindu** | statecharts, industrially | separate artifact, proprietary format, generated code; the behaviour is not in the document that runs. Category explicitly declined — [POSITIONING.md](POSITIONING.md#categories-declined) |
| **Qt `moc`** | a language extension C++ would not host | the expensive way: bespoke pre-processor, separate pass, generated code nobody reads |

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

Two cases, one positive and one negative, observed by the same engineer at two companies
a decade apart. And — decisively for publication — a **public artifact** for the negative
one's domain.

The spine of the section is that these are **one failure mode at three magnifications**:

| Scale | Symptom | What is unreadable |
|---|---|---|
| Compiler | you cannot tell what a construct means | the intermediate stages |
| Runtime | printf-debugging for days | the state the program is in |
| Team | dailies that are never conclusive | the relation between spec and code |

The tell at every scale is the same: when the thing you need to read does not exist as an
artifact, you substitute an experiment — a bisection, a print statement, a meeting.

### Case 1 (positive): Dingo TRGS, read at ten years

The experiment proposed at the end of the previous section has already been run, at ten
years rather than one, on an industrial project, with a control.

**The design, which is stronger than the usual readability anecdote.** One engineer wrote
both artifacts: the ceps state machines *and* the accompanying C++. The reader, ten years
later, wrote neither — he wrote the tooling. So this is not someone finding their own code
readable, which tests nothing. It is a third party reading one author's work in two
notations, which is the configuration maintenance actually happens in.

**The outcome.** The ceps was recoverable without difficulty. The C++ was markedly less
enlightening.

**What the C++ was.** Qt GUI code — the HMI the soldier operated the mast through. So this
is not statechart-versus-hand-rolled-statechart. Qt is event-driven too, it is a mature
idiom, and the author was competent; the comparison is not against bad code. The axis is
**behaviour that has a notation versus behaviour that does not.** The mast's modes and
enablement rules were just as real on the Qt side, distributed across slots, member flags
and `setEnabled()` calls, stated nowhere.

Two objections to concede before they are raised:

- *Spec versus implementation.* TRGS defeats it — the ceps was compiled to C++ and ran on
  the ARMv7 target. Both artifacts were production.
- *Essential versus accidental complexity.* Fair: GUI code carries layout, widget
  lifetimes, painting and threading. Concede it, then observe that signals and slots is a
  language extension C++ would not host, so Qt built `moc` — a bespoke pre-processor, a
  separate pass, and generated code nobody reads. Qt needed this mechanism and paid full
  price for it.

**The confound, and why it is smaller than it looks.** The reader is the notation's
designer. Not a C++ fluency gap — he writes C++ daily — but knowledge of ceps's semantics
no reader of the C++ had. The rebuttal is measurable rather than rhetorical:

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
`+`/`-` enter/exit convention was the freelancer's invention. The notation survived an
author it was not designed by. Perl's did not.

**Blocker: clearance.** The artifacts are KMW's and nothing can be quoted until that
conversation has happened. Case 2 does not have this problem.

### Case 2 (negative): an ISO 15118 stack built on signals and slots

A DC charging manufacturer's legacy stack, the same engineer as tech lead. **ISO 15118 is
nothing but a long description of state machines** — so in this case the statecharts
existed, were fully specified, and are publicly checkable. What was missing was any
representation of them in the artifact that ran. The stack was fragile, error-prone, and
adding a feature meant days of printf-debugging; nobody understood what the software did.

This is the control the positive case needs, and it is what removes the Harel confound.
The variable is not *statecharts versus none* — that is Harel's result, published in 1987
and shipped by Stateflow for thirty years. It is **the state machine is written down in
the thing that executes, or it is not.**

Signals and slots is the mechanism of the loss, not a bystander: the mode exists in no
single place, it is distributed across `connect()` calls, so there is nothing to read.

And "printf-debugging for days" is the same disease as an unreadable IR, one scale up. You
cannot read the middle, so you bisect by experiment. `--pe` is that question asked of a
compiler.

**The third rung.** Every daily was spent arguing what the standard said against what the
code did, and never conclusively. The tell is the *never*: an argument that can be settled
gets settled once, and one that recurs daily is one where the artifact needed to settle it
does not exist. "What does the standard say" and "what does the code do" were two lookups
in two documents and nobody could hold both, so the standup became the place where two
artifacts were reconciled by debate.

`timing.ceps:20-22` does not make that argument easier to win. It makes it the same lookup.

Be honest about what survives: interpretation disputes do not vanish. What vanishes is
arguing about what the code *assumes*, because the assumption sits next to the sentence.
That is a smaller argument and it terminates.

### The artifact

The response to Case 2 is public, MIT-licensed, and the author's own — so it can carry the
paper's examples with no clearance conversation at all.

| Repository | Models | Size | Dates |
|---|---|---|---|
| `v2g-guru` | ISO 15118-2 | 113 `.ceps` files, 3657 lines | 2021-03-30 … 2021-08-06 |
| `v2g-guru-slac` | ISO 15118-3 (SLAC) | 32 `.ceps` files, 3596 lines | 2021-11-06 … 2022-02-22 |

Both are executable specifications that run on machines4ceps. Three things in them are
worth putting in the paper directly:

**1. The requirement is adjacent to what implements it** — `v2g-guru-slac/model/timing.ceps:20-22`:

```ceps
label V2G3_M08_01_table_3_5 title = "[V2G3-M08-01-table-3-5] Minimum B state duration after a state F,D, or C.";
val T_conn_init_HLC = time_scale*0.2*s;
Event evT_conn_init_HLC;
```

The standard's requirement ID, its text, the constant and the event it raises, in three
consecutive lines. This is the answer to *"ingenuity is locally driven, so the why gets
buried"* shown rather than argued — and `label` is a lexer keyword, so traceability is
vocabulary, not a tool feature. The same shape governs a state machine at
`model/controlpilot/controlpilot.ceps:9` and `:14`.

**2. Traceability extends to the filesystem.** `v2g-guru/iso-15118-2/sections/8/5/2/3/`
mirrors the standard's section numbering, and `v2g2/00483.ceps` is a requirement number.
43 `label` statements across the two repositories carry requirement IDs.

Note this does *not* reopen the declined statechart-tool category
([POSITIONING.md](POSITIONING.md#categories-declined)): what is declined is the category
with its graphical editor, target matrix and certification kits, not the capability.

**3. The cost and the benefit are visible in the same file.** `0.2*s` is the SI unit
*second*, used deliberately and correctly — in a timing specification derived from a
standard's constant table, units are load-bearing. [D17](DEFECTS.md#d17) is the shadow of
a feature in genuine use, not a gratuitous one. Say that in the cost section; it is more
honest and it is also a better argument.

**Cheapest way to strengthen all of this:** one engineer who knows neither artifact, both
codebases, timed comprehension questions. Even n=3 moves this from experience report to
measurement.

### Why the middle is a real place to stand

Interpreting a standard is hard, and the paper should not pretend otherwise. The claim is
about *how* the interpretation is arrived at.

**The usual way interprets by implementing**, and that does not merely obscure the reading
— it **conflates** it. Implementing ISO 15118 in C++ forces commitments to threading,
dispatch, object lifetime and error handling that have nothing to do with the standard.
Afterwards nobody can separate *this is what the standard says* from *this is how I got it
to work*, which is why the outcome is a clear understanding of neither.

**Small semantic distance buys a second way of reading.** ISO 15118 is a description of
state machines and a Harel chart is a state machine, so the translation is close to
identity and introduces almost no commitments of its own. And because the model runs, the
interpretation can be checked **by inspecting behaviour** rather than by arguing about
text — you discover what your reading commits you to, including the consequences you did
not intend, before integration rather than during it.

**And the holes become visible.** An under-specified standard shows up as a transition
that is not there. The choice made to fill it is then recorded *at the site of the gap*.
In a C++ implementation you simply pick something, and the fact that you picked is written
down nowhere.

This is also why the two declined categories in
[POSITIONING.md](POSITIONING.md#categories-declined) are the right neighbours to decline,
and it is worth saying so in the paper rather than leaving the middle undefended:

| Neighbour | Semantic distance to the standard | Does the model ship? |
|---|---|---|
| Statechart tools | small | no — generated code ships, the model is an artifact beside it |
| Formal specification (TLA+, Alloy) | large, in the other direction | no — the model is abandoned after checking |
| ceps | small | **yes — compiled to C++, ran on the ARMv7 target** |

---

## What is missing before this can be written

| Gap | Where |
|---|---|
| **The x64 backend is a stub** — empty loop body, `return {}`. Any claim of "down to machine code" is currently false; the design reaches the encoder and stops. | `core/src/vm/oblectamenta-assembler.cpp:1380-1387` |
| **No evaluation.** The natural measure is the cost of adding a layer: grammar rules added (zero), parser changes (zero), printer changes (zero). The msgdef schema and the x64 layer are the two worked cases. The grammar-growth figures in [The evidence](#the-evidence) are the other half — four productions in eleven years, while the vocabulary grew by an assembler, a VM and a serialiser. | — |
| **The loud fail is designed and not built.** The cost section is much stronger if it ends with a fix rather than a plan. | [ROADMAP.md](ROADMAP.md) |
| **A worked end-to-end example** small enough to print: requirement → state machine → action → assembler → encoding, in one file. | `v2g-guru-slac/model/timing.ceps:20-22` covers requirement → constant → event; [ASM.md](ASM.md) §4 covers data → action → assembler. Nothing yet spans both. |

---

## Venue

A design paper, not a theory paper: **GPCE, SLE, Onward!** Not PLDI — there is no novel
mechanism in the sense PLDI means, and the contribution is a composition with a measured
price, which is what those three venues exist for.

---

## Provenance

The thesis is not retrospective, and this is unusually easy to demonstrate.

### The line, dated

| Year | What | What arrived |
|---|---|---|
| **2007** | **ANDIDEP**, Capgemini — a dependency analyser for ANDI, an interface in Telekom's PROKOM. Written first as three pages of Perl, then **rewritten using Knuth's original `weave`/`tangle`.** | literate programming, practised rather than admired |
| **2011** | **Audi EXAM** — a software project led by the author, and the first use of the approach that became ceps: a notation describing the problem domain, a diagram generator over it, and **the tests derived from it**. | *literate testing* — the domain description as the thing that is true, everything else a consequence |
| **2013** | **yamdl**, for the BMW HAF project. Repository initialised 2013-09-25. | the principle written down: Scribbling, TopUp |
| **2014** | the TRGS era. `ceps.y` at `93c7a24` has 16 nonterminals. | the notation, in production |

Three things follow that are worth a paragraph each in the paper.

**The practice precedes the principle by six years.** The Scribbling chapters are 2013;
deriving tests from a domain description was 2011; literate programming with the real
tooling was 2007. Nothing here was designed from a principle and then applied — the
principle was named after the fact, which is also what [POSITIONING.md](POSITIONING.md)
concludes from a different direction.

**SDDI is older than the document that states it.** `POSITIONING.md` gives the operational
rule as *specify data in a high level notation and derive your interfaces*. At Audi in
2011 it was *describe the domain and derive the tests*. Same shape, same ordering, a
different noun, two years before yamdl and fifteen before the positioning document. That
continuity is worth claiming, because it shows the decision rule was observed rather than
invented.

**"Nobody appreciated that move."** The literate rewrite in 2007 was received with
indifference, which is the reception literate programming has had generally — and the
reason the idea had to be rebuilt rather than adopted. That belongs in the paper as
history, not as grievance: a mechanism that requires a tool, a discipline and two
artifacts does not spread, and the version that does spread will be the one with no tangle
step.

### The 2013 artifact

> *"without being forced to throw away any scribbling sheets"*

— `~/dev/yamdl-orig/doc/mst_part_1.html`, repository initialised 2013-09-25. A design
paper whose thesis is checkable against a thirteen-year-old file is in a different
category from one with a tidy story assembled afterwards. Say so, and cite the file.

**Citation asymmetry to plan around:** EXAM is publicly documented and can be named.
ANDIDEP and PROKOM are Telekom-internal and have to be described generically — "a
dependency analyser for a telecoms interface" costs nothing and avoids a second clearance
conversation.

### Unrecovered, and perishable

The 2011 EXAM-era notation is **the earliest surviving artifact of the ceps approach, two
years before yamdl** — and it is on old laptops, unexamined. Whether it was already
struct-like text and what the diagram generator emitted are both unknown.

This is the highest-value retrieval task attached to the paper and the one with a deadline
imposed by hardware. Recovering a 2011 file that already shows the shape would push the
provenance claim back two years with an artifact rather than a recollection.

### An earlier name, recorded here because it is recorded nowhere else

One of the first labels for what ceps does was **"literate testing"** — the lineage was
understood as Knuth's from the beginning, not attached to it afterwards in a related-work
section. That is corroborated by the 2007 entry above: the author had used the real
`weave`/`tangle` tooling four years before the approach appeared.

It appears in no file and no commit message in `machines4ceps`, `ceps`, `yamdl-orig`,
`v2g-guru` or `v2g-guru-slac`; it survives only as the author's recollection. That is
worth a sentence in the paper's history and it is worth writing down here, because an
unrecorded name is one retirement away from being gone.

The name is also an accurate reading of the difference. Literate *programming* weaves
prose around code and tangles code out of prose. Literate *testing* is the same impulse
applied to behaviour — the requirement, the behaviour that satisfies it, and the run that
exercises it, in one document, with no step that separates them.

---

## Related documents

| Document | Contents |
|---|---|
| [POSITIONING.md](POSITIONING.md) | what ceps is for, and what it is free to refuse |
| [ASM.md](ASM.md) | the Oblectamenta assembler and data section — the layer this paper describes |
| [DEFECTS.md](DEFECTS.md) | the cost, with reproducers |
| [doc/scribble-concept/README.md](doc/scribble-concept/README.md) | the scribbling principle, worked |
