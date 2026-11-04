# Prior art for the channel stack

Companion to `GOALS.md`, specifically to [G11](GOALS.md#g11--channels) and
[G12](GOALS.md#g12--a-compilation-target).

The channel idea in G11 — a stack of small grammars, each consuming the token stream the
previous one emitted, each matching only the shapes it knows and passing the rest through —
was arrived at independently in this project. It is also published, precisely, and has been
for thirty years.

**This document exists for two reasons, and the second is the important one.**

1. So that nothing here is claimed as new. The architecture has a name, a literature and a
   lineage.
2. **So that G11 is built from it rather than rediscovered.** The termination argument, the
   composition result, the streaming execution model and the longest-match-with-passthrough
   rule are all worked out. There is a design manual available; it is just filed under
   computational linguistics rather than under compilers.

## Epistemic status

The verdicts below carry the status they were established with. *Read* means the primary text
was fetched and quoted. *Bibliography* means the citation was confirmed but the content was
not independently verified. Two items are explicitly unresolved and are marked as such. ACM
DL and dblp were unreachable during the search, which is why some items stop at
*bibliography*.

---

## 1. The architecture — Abney 1996

> Steven Abney, **"Partial Parsing via Finite-State Cascades."** *Natural Language
> Engineering* 2(4):337–344, Cambridge University Press, 1996.
> DOI [10.1017/S1351324997001599](https://doi.org/10.1017/S1351324997001599).
> A near-identical version appeared in the ESSLLI'96 Robust Parsing Workshop. **Read.**

A cascade is a sequence of levels L0 (raw input), L1, L2, …, where each level's recognizer
Tᵢ *"is run with Lᵢ₋₁ as input, and it produces Lᵢ as output."* Matching is longest-match.
When *"the recognizer blocks without reaching a final state … a single input element is
punted to the output and recognition resumes at the following word."*

That is the channel stack, including the pass-through disposition that
[G11 decision 2](GOALS.md#g11--channels) proposes. Abney's own description of the
architecture is worth reading twice:

> "little more than a pipeline of Lex-style lexical analyzers"

Which is, to the character, what this engine is.

**What Abney does not do**, both checked against the full text:

- **No coverage reporting.** Punting is a silent per-token fallback. There is no aggregate
  *N of M tokens were punted* anywhere in the paper. This is the gap — see §6.
- **Streaming is not demonstrated.** The paper says "pipeline", but its performance
  discussion reads as sequential batch passes per level. Abney never states the concurrency
  model either way. The streaming model in G11 is therefore *not* contradicted by this paper,
  but it is not supported by it either.

**Related, and already linked in the literature:** Moonen's island-grammar paper cites Abney
directly. These are not two independently forgotten results; they are two members of one
small sub-lineage.

---

## 2. Fusing the stack — Kaplan & Kay 1994

> Ronald M. Kaplan and Martin Kay, **"Regular Models of Phonological Rule Systems."**
> *Computational Linguistics* 20(3):331–378, 1994.
> [aclanthology.org/J94-3001](https://aclanthology.org/J94-3001/). **Read.**

G12 floats the idea that compiling a channel stack might allow the stages to be *fused* — N
grammars collapsed into one automaton, N passes for the cost of one. **This is a settled
result, not an opportunity:**

> "The more radical approach that we explore in this paper is to eliminate the cascade
> altogether, representing the information in the grammar as a whole in a single more unified
> device, namely, a finite-state transducer. … Repeated composition reduces the machines
> corresponding to the rules of a complete phonological grammar to a single transducer."

The paper also establishes that cascades of rule-transducers were *already* standard practice
before 1994, citing speech-understanding systems back to Woods et al. 1976.

**The theoretical root predates all of it:**

> C. C. Elgot and J. E. Mezei, **"On Relations Defined by Generalized Finite Automata."**
> *IBM Journal of Research and Development* 9(1):47–68, 1965.
> DOI [10.1147/rd.91.0047](https://doi.org/10.1147/rd.91.0047). Closure of transducers under
> composition. **Bibliography**, confirmed via Mohri 1997's attribution.

**Consequence for G12:** fusion is a technique to *apply*, with a citation. It is not a
contribution, and presenting it as one would not survive a first reading.

---

## 3. Streaming execution — Mohri 1997

> Mehryar Mohri, **"Finite-State Transducers in Language and Speech Processing."**
> *Computational Linguistics* 23(2):269–311, 1997.
> [aclanthology.org/J97-2003](https://aclanthology.org/J97-2003/). **Read.**

On the pipelined execution model:

> "instead of waiting for the result of the application of T1 to be completely given, one can
> gradually apply T2 to the output strings of T1 yet to be completed."

and

> "it can then be used, for instance, at any step in an on-the-fly cascade of composition of
> transducers in speech recognition to expand only the necessary part of a lattice or
> transducer."

Deployed in production speech recognition since 1996 (Pereira & Riley 1996; Mohri, Pereira &
Riley 1996).

**One nuance that matters for G11.** Mohri's on-the-fly composition lazily expands states of
the *single fused* transducer. That is streaming execution of a compiled result — not N
independent stages running concurrently and passing tokens to one another live, which is the
model described in G11. Conceptually adjacent, not identical. Whether the difference is worth
anything is an open question, not an assumed yes.

**The general principle, much older:**

> Melvin E. Conway, **"Design of a Separable Transition-Diagram Compiler."** *Communications
> of the ACM* 6(7):396–408, 1963. Coroutines introduced to pipeline compiler passes so stages
> run concurrently rather than as separate passes. **Unverified** — the primary text could
> not be retrieved; this rests on Knuth's and Wikipedia's characterisation. Cite via Knuth,
> or not at all, until someone reads the paper.

**The supporting lineage**, confirmed through the bibliographies of the two papers read
directly:

- C. Douglas Johnson, *Formal Aspects of Phonological Description*, Mouton, 1972 — ordered
  rewrite rules have exactly finite-state power.
- Kimmo Koskenniemi, *Two-level morphology*, University of Helsinki Publication No. 11, 1983.
- Karttunen, Koskenniemi & Kaplan, *A compiler for two-level phonological rules*,
  CSLI-87-108, Stanford, 1987 — an actual compiler composing rule cascades.
- Karttunen, Kaplan & Zaenen, *Two-level morphology with composition*, COLING-92, 141–148.
- Mohri & Sproat, *An efficient compiler for weighted rewrite rules*, ACL 1996.

---

## 4. Partial matching over documents — the software-engineering branch

> Leon Moonen, **"Generating Robust Parsers using Island Grammars."** *WCRE 2001*, 13–22.
> DOI [10.1109/WCRE.2001.957806](https://doi.org/10.1109/WCRE.2001.957806). **Read.**

Grammars that describe only the constructs of interest ("islands") plus a catch-all "water"
nonterminal absorbing everything else. Structurally the same premise as the inverted default
plus a catch-all rule — but a **single level**, not a cascade.

And decisively for §6, the water is thrown away:

> "we have created a simple filter that removes all subtrees that have been parsed as Water
> from a parse tree."

No coverage figures appear anywhere in the paper.

> Roland Koppler, **"A Systematic Approach to Fuzzy Parsing."** *Software: Practice and
> Experience* 27(6):637–649, 1997. **Bibliography; content secondhand via Moonen.**
> Reported to produce a per-parse *degree of correctness* for a given input — per-document,
> not corpus-level. Read before relying on it.

> Gail C. Murphy and David Notkin, **"Lightweight Lexical Source Model Extraction."** *ACM
> TOSEM* 5(3):262–292, 1996. DOI [10.1145/234426.234441](https://doi.org/10.1145/234426.234441).
> **Abstract read; full text not retrieved.** Explicitly tolerant: *"there are few constraints
> on the condition of the artifacts. For example, we can extract from source that cannot
> necessarily be compiled."* **This is the most important unresolved item in this document** —
> see §6.

---

## 5. The inverted default

[G6](GOALS.md#g6--the-grammar-stays-readable-to-the-person-who-owns-the-requirements) treats
*bare words are literals, abstraction is opt-in* as the property to protect. It is not
unprecedented.

- **SNOBOL4** (Griswold, *A History of the SNOBOL Programming Languages*, HOPL-1, 1978,
  DOI [10.1145/800025.808393](https://doi.org/10.1145/800025.808393)) — bare concatenation is
  a literal sequential match; alternation and repetition are explicit operators. Predates
  regex's metacharacter-pervasive design. **Bibliography**; no passage found arguing this as
  a stated *principle*.
- **Cucumber Expressions** — the strongest and most citable precedent, and a live one.
  [cucumber.io/docs/cucumber/cucumber-expressions](https://cucumber.io/docs/cucumber/cucumber-expressions/).
  **Read.** *"The simplest Cucumber Expression that matches that text would be the text
  itself"* — bare text is the baseline, abstraction is opt-in via `{int}`, `{word}`,
  `{string}`. Explicitly positioned as *"an alternative to Regular Expressions with a more
  intuitive syntax"*, aimed at non-programmers writing step definitions. Not cascaded, not
  streaming, no residue reporting — but it makes the bare claim *nobody has done this*
  untenable.
- **EARS** — Mavin, Wilkinson, Harwood & Novak, *Easy Approach to Requirements Syntax*,
  RE'09, 317–322. DOI [10.1109/RE.2009.9](https://doi.org/10.1109/RE.2009.9).
  **Bibliography.** Note the distinction and do not blur it: EARS is a controlled-language
  template for *writing* requirements, not a pattern notation for *matching* text. It shares
  the small-reserved-vocabulary flavour and is not a precedent for the pattern syntax.

### A naming collision to preempt

**ANTLR already uses the word "channels"** and means something materially different:
`WS : [ \t\r\n\f]+ -> channel(HIDDEN) ;` routes token *types* to a named side-stream of the
**same** token stream from **one** grammar, so a parser can ignore whitespace while a
formatter can still retrieve it. Single-grammar output partitioning — not a cascade.

Anyone searching "lexer channels" hits ANTLR first. Either pick a different word, or
distinguish it explicitly and early. *Cascade*, *stage* and *level* are all available and all
carry the right meaning from the literature above.

---

## 6. What is actually unclaimed

One thing, and it is thin.

**Residue as a first-class, corpus-level, ground-truth-free deliverable** — *"these 11 of your
400 requirements are not in a form I understand, here they are."* On this exact question:

- Abney punts silently; no aggregate statistic exists in the paper.
- Moonen explicitly filters water out of the tree.
- Koppler appears to be per-document, not corpus-level (secondhand).
- Murphy & Notkin is **unresolved** and is the thing to read next.

There is a real distinction here that is easy to lose. The MUC/information-extraction
recall-precision tradition measures *task correctness against a gold standard* — it needs
ground truth. The residue metric in [G3](GOALS.md#g3--nothing-is-understood-partially-in-silence)
needs none: it is intrinsic coverage of a corpus's **surface forms**, *how much of this text's
shape do I recognise*, independent of whether the recognised parts were interpreted
correctly. As far as the search went, that is not what any of the work above measures.

**Treat this as promising and unverified, not as clear ground.** The search never reached IE
evaluation literature, data-wrangling tools (the OpenRefine/Trifacta *% of rows transformed*
family), or schema-inference systems — which are precisely where this would surface if it
exists. That is a gap in the search, not a clean negative.

---

## What to read first, if you are building G11

1. **Abney 1996** — eight pages. It is the design you already have, written down, with the
   punting rule stated precisely. Read it before writing any code.
2. **Mohri 1997**, §on composition — for the streaming execution model and the on-the-fly
   algorithm.
3. **Kaplan & Kay 1994**, §on composition — only if fusion is actually wanted; it is long.
4. **Murphy & Notkin 1996** — because §6 depends on it, and nobody here has read it.

The honest summary: **the architecture is thirty years old and well-understood, which is
good.** It means G11 is an implementation task with a literature behind it rather than a
research project. The part that is genuinely open — reporting what was *not* understood — is
also the part this project already cares most about, and it is the smaller piece of work.
