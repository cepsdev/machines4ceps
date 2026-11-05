# Bidirectional `ceps-transduce`

A design document for running the transform in both directions: `Text → Model` as today, and
`Model → Text` from the same grammar.

This is not a new capability bolted onto the side. It is the missing half of a goal the ceps
project has had from the beginning — lift ad hoc formats into one executable formalism — and
the half whose absence has been the standing objection in the field:

> *We can get our DBC into ceps. We cannot get it back out.*

A pivot format you can only enter is not a pivot, it is a sink. The round-trip law is what
discharges the claim that ceps is a faithful representation of the source rather than a lossy
summary of it. That is the proposition this document is about.

---

## 1. The name

The component is called **`ceps-transduce`** ([GOALS.md, decision 10](GOALS.md#open-decisions)).

Not `parse`, and not `rewrite`. A transducer relates two languages and does not privilege a
direction. `parse` names the forward direction and thereby makes the reverse look like a
separate tool that somebody should write one day — which is precisely the framing that kept
`ceps → dbc` off the roadmap for a decade. The grammar is one artifact; the two directions
are two readings of it. The name should say so.

It is also not a lexer. See *What this component actually is* in [`AUDIT.md`](AUDIT.md).

---

## 2. Two properties, and they are not the same one

Write `R` for the relation the grammar defines between texts and models. Two laws get
confused with each other, and they govern opposite directions.

| | law | makes well-defined |
|---|---|---|
| **Functionality** | `T R M₁ ∧ T R M₂ ⟹ M₁ = M₂` | `Text → Model` |
| **Injectivity** | `T₁ R M ∧ T₂ R M ⟹ T₁ = T₂` | `Model → Text` |

Today's engine satisfies **functionality** — but by fiat rather than by design. Rules are
tried in file order and the first match wins, so a text cannot produce two models no matter
how ambiguous the grammar is. The law holds and tells you nothing. Its real content is
*unambiguity*, and first-match-wins is exactly the mechanism that conceals it.

**Injectivity is false**, and this is the important half.

### Measured, not assumed

`ears.ceps.lex` contains **zero** guards. It is the clean, conditional-free fragment. Three
distinct inputs, one byte-identical model:

| input | differs from baseline by | model |
|---|---|---|
| `a.req` | — | baseline |
| `b.req` | extra spaces and a tab — `%ignore_ws` destroys them | **identical** |
| `c.req` | trailing `noise noise` — consumed by `any => .` at `ears.ceps.lex:105` | **identical** |

Three further losses are structural rather than incidental: `%ignore_ws` destroys layout
before any rule sees it, runs of newlines collapse in `get()` ([L11](AUDIT.md#l11)), and any
rule whose action discards a matched position erases that position permanently.

### And injectivity is not merely absent, it is not wanted

The first channel of the [G11](GOALS.md#g11--channels) design is normalisation — `must =>
shall`, `will => shall`. That is many-to-one **on purpose**. A transform that collapses
variants cannot be injective, and a requirements tool that refuses to collapse variants is
not useful.

So the condition is not *"forbid the rules that lose information."* Almost every useful rule
loses information.

---

## 3. Lenses: stop demanding injectivity, carry the complement instead

The literature settled this. A **lens** does not invert `get`; it gives `put` the original
source as an extra argument, so the information `get` discarded can be put back.

From Bohannon, Foster, Pierce, Pilkiewicz and Schmitt, *Boomerang: Resourceful Lenses for
String Data*, POPL 2008, §2 — a basic lens from `C` (concrete) to `A` (abstract) comprises:

```
l.get    ∈ C −→ A
l.put    ∈ A −→ C −→ C
l.create ∈ A −→ C
```

obeying, for every `c ∈ C` and `a ∈ A`:

```
l.put (l.get c) c = c        (GetPut)
l.get (l.put a c) = a        (PutGet)
l.get (l.create a) = a       (CreateGet)
```

The paper is explicit that non-injectivity is the expected case, and says why `put` has the
shape it does:

> "The get component of a lens may, in general, discard some of the information from the
> concrete structure while computing the abstract structure. The put component therefore
> takes as arguments not only an updated abstract structure but also the original concrete
> structure."

`GetPut` and `PutGet` together are what *well-behaved* means. Adding `PutPut` — two puts in
sequence behave as the second alone — gives *very well behaved*, which is a strong condition
most practical lenses do not satisfy and do not need.

*(The laws are quoted as Boomerang states them, with plain `=`. The earlier tree-lens paper
writes them with Kleene equality `≈`, because its lenses are partial and the law must hold
when both sides are undefined. For total lenses the two readings agree.)*

### `create` answers the question this design was going to get stuck on

An earlier sketch of this idea ran aground on: *you fix the model by adding a transition —
what requirement text does `put` write, and who assigns it an ID?* That was filed as a policy
question outside the theory. It is not. It is `create : A → C`, with its own law. The
framework already names the hole; what remains is a project decision about what `create`
should emit, not an open problem.

### The engine must retain the complement

`put` needs the discarded information. Today it is not discarded late, it is discarded
*early* — `%ignore_ws` removes whitespace before rules run, `get()` collapses newline runs,
`any => .` drops tokens with no record. None of it is recoverable. Retaining it is the one
genuinely new mechanism this design needs, and it is the subject of
[open decision 8](GOALS.md#open-decisions).

---

## 4. What this catches: a live defect in a shipped import

The case for the laws is not theoretical. Asking *"is this rule invertible?"* of
`import-scripts/dbc/dbc.ceps.lex` found a defect that had been in the tree unnoticed.

```
lexer read_signal_signedness{
 integer + => signed{1;}; .
 integer - => unsigned{0;}; .
 else => rewind exit .
}
```

In the DBC `SG_` syntax the field is `start|length@byte_order value_type`. The `integer`
matched here is the **byte order** — `@1` little-endian (Intel), `@0` big-endian (Motorola).
It is `$0`, it is matched, and it is **never emitted**.

Reproduction — one character changed in `frames.dbc`:

```
 SG_ a_signal : 0|8@1+ (1,0) [-100|100] ""  channel1     # little-endian
 SG_ a_signal : 0|8@0+ (1,0) [-100|100] ""  channel1     # big-endian
```

```
$ diff le.out be.out && echo IDENTICAL
IDENTICAL
```

Two DBC files that decode CAN frames completely differently produce a byte-identical ceps
model. Exit code 0, no diagnostic. Every ceps-side consumer of this import has been working
from signal definitions with the endianness missing.

**This is the argument in one finding.** No test caught it, because a test would have to know
to ask. The round-trip law asks automatically: reconstruct the `SG_` line from the model and
there is nothing to write between `@` and `+`.

Invertibility is not a feature request. It is a bug detector that runs on every input you
already have.

---

## 5. The invertible fragment is larger than it looks

The pessimistic reading is that making rules invertible makes them more complicated. On the
evidence in this tree, it does not — because **the rule notation is already bidirectional and
nobody has read it that way.**

Take P1 of `ears.ceps.lex`:

```
ident - integer The ident shall initialise in the ident state \. =>
  incose_init{
    req{double_quote $0 - $2 double_quote;};
    tag{$0 _ $2;};
    system{$4;};
    state{$9;};
  }; .
```

Reading it backwards: match `incose_init`, recover `$0` and `$2` from `tag`, `$4` from
`system`, `$9` from `state`, and emit

```
$0 - $2 The $4 shall initialise in the $9 state .
```

The literal tokens — `The`, `shall`, `initialise`, `in`, `the`, `state`, `.` — are written in
the pattern. **The pattern is the template.** Nothing additional is authored.

Three things that look like obstacles and are not:

- **Duplication.** `$0` appears in both `req` and `tag`. Going back, take either; checking
  that they agree is a free consistency test.
- **Redundant encodings.** `req{…}` builds a display string from the same captures `tag`
  holds structurally. The structural one wins and the string is regenerated.
- **Rules sharing an output node.** P2 and P4 both emit `incose_transition`. P4 carries
  `unwanted{true;}` and P2 does not, so the pair is separated — and that discriminator was
  already there, added for forward-direction clarity.

### The tier split is per action element, not per rule

The natural split is finer than "invertible grammars and one-way grammars". Within a single
rule:

| action element | kind | backward |
|---|---|---|
| `system{$4;}` | model construction | inverts |
| `tag{$0 _ $2;}` | model construction, **spliced** | needs a separator convention |
| `trace{print(…);}` | side effect | dropped; not part of `A` |

One rule, two readings. That is the same *one mechanism, repeated* shape the rest of the
design is held to, rather than an exception to it.

---

## 6. What actually stands in the way

Three blockers. Two were already in the audit before this document existed.

### 6.1 No list operator — hence guards — hence `dbc` does not invert

`dbc.ceps.lex` contains seven `|` characters. **Five are escaped literal pipes**, part of
DBC's own syntax (`[min|max]`, `startbit|width@`), and invert without difficulty. There are
exactly **two** real guards, at `:35` and `:36`:

```
 ident ,                    => $0; .
 ident | !read_id_list_end  => $0; /read_id_list_end=1;/ .
 any   | read_id_list_end   => rewind exit .
```

That is `list(ident, ",")`, hand-rolled as a state machine with a one-bit flag because the
engine has no separator construct.

Guarded rules are not merely non-injective — they are **not invertible in principle**. A
guard reads state that a sibling rule wrote, and that state is consumed and never emitted, so
no reverse transform can reconstruct it. [L19](AUDIT.md#l19) shows the state is worse than
hidden: it is global, shared by every lexer, and leaks across calls.

So the chain is three links long, and the far end is a conversation lost in the field:

> no `ceps → dbc` ⟵ `dbc.ceps.lex` does not invert ⟵ it uses guards ⟵ there is no list operator

One missing construct. [Open decision 1](GOALS.md#open-decisions) retires it.

### 6.2 Identifier splicing ([L15](AUDIT.md#l15))

```
act{$17 _ $19;};        // "illuminate" + "hazard_lamp"  →  illuminate_hazard_lamp
```

Backward, splitting on `_` is ambiguous as soon as either side contains `_` — and the doc
comment at `ears.ceps.lex:65` picks exactly that case, which is live in `door_lock.req:31`.
`illuminate_hazard_lamp` reads equally well as `illuminate` + `hazard_lamp` and as
`illuminate_hazard` + `lamp`.

Needs either a reserved separator that cannot occur in an identifier, or an escape, or for
the structure to be kept as structure and spliced only for display.

### 6.3 The complement is not retained

Section 3. The engine discards early and keeps no record. This is the only item of the three
that is new work rather than the removal of an existing mistake.

---

## 7. Worked example A — EARS, for readability

`get`, which exists today:

```
REQ-001
The controller shall initialise in the unlocked state.
```
```
incose_init{ req{"REQ-001"}; tag{REQ_001}; system{controller}; state{unlocked}; }
```

`put`, with the original in hand. Edit the model — `state{unlocked}` becomes
`state{locked}` — and the result is **one changed word in one line**:

```
-The controller shall initialise in the unlocked state.
+The controller shall initialise in the locked state.
```

Everything else in `door_lock.req` is untouched, and that is the point: **15 of its 35 lines
are header, rationale and a citation** that no model carries and no `Model → Text` *function*
could invent. Only `put`, holding the source, preserves them. This is what makes the output a
reviewable diff rather than a regenerated document — which is the difference between
something a requirements engineer will accept and something they will not.

This is the workflow the whole design is for:

```
req ──get──▶ model ──simulate/test──▶ defect ──repair──▶ model' ──put──▶ req'
```

And `put` being **partial** is a feature, not a gap: repair the model with a transition no
EARS pattern can express and there is nothing to write. The failure says *your fix is not
expressible as a requirement*, which in a certified process is worth more than the automation
is.

## 8. Worked example B — DBC, for the acceptance criteria

DBC is the better proving ground: machine-generated, machine-consumed, no prose, and the
round-trip is checkable byte for byte. Thousands of real files exist in the wild, which makes
it [G9](GOALS.md#g9--the-corpus-exists)'s corpus with the oracle already attached.

```
SG_ a_signal : 0|8@1+ (1,0) [-100|100] ""  channel1
```
```
sig{ name{a_signal}; start{0}; width{8}; signed{1}; scale{1}; offset{0};
     min{-100}; max{100}; unit{""}; receiver{channel1}; }
```

Ten fields in, ten fields out — and one field, the byte order, silently missing (§4). Fixing
`read_signal_signedness` to emit it is a precondition, not a consequence: **the round-trip
cannot pass until the model is complete.** That is the law doing the work a test suite would
otherwise have to be written to do.

---

## 9. Acceptance criteria

A grammar is **invertible** when every rule is linear and non-erasing over its model-producing
action elements: each matched position used in the model appears exactly once, no position
that carries information is dropped, and no guard is read. This is statically decidable per
rule, and the engine should report which tier each rule is in.

The implementation is accepted when:

1. **`GetPut` holds on the corpus.** For every file `t`, `put(get(t), t) = t`, byte for byte.
   For DBC this is exact. For documents with layout, exact after the complement is restored.
2. **`PutGet` holds.** For every model edit `a` reachable by the editing operations the tool
   offers, `get(put(a, t)) = a`.
3. **`put` is total on the invertible tier and explicitly partial outside it.** A model that
   cannot be written back produces a diagnostic naming the rule and the reason, not a
   silently truncated document — [L6](AUDIT.md#l6)'s positions are a precondition.
4. **The tier of every rule is reported.** `ears.ceps.lex` must come out fully invertible.
   `dbc.ceps.lex` must come out invertible **except** `read_id_list`, until
   [decision 1](GOALS.md#open-decisions) retires the guards.
5. **The byte-order defect is fixed and stays fixed**, because the round-trip test fails
   while it is present.
6. **Ambiguity is reported, not resolved in silence.** Running every rule against every
   corpus line and reporting multi-matches is the precondition for all of the above; it is
   independently valuable, and it would have caught [L14](AUDIT.md#l14).

On that last point the literature agrees, and the way it agrees is the most consequential
thing in this document.

Boomerang's type system requires concatenations and iterations to be **unambiguous**, because
`GetPut` fails otherwise. The paper gives a four-line counterexample — `lbogus`, built from
`(a ↔ bb | aa ↔ a | b ↔ b | ba ↔ ba)` — where an ambiguous split makes a lens that is
otherwise well-typed violate the law outright. Ambiguity detection is not a nicety attached
to this design. It is load-bearing.

And Boomerang can *decide* it. **Fact 2.1**:

> "It is decidable whether two regular languages `L1` and `L2` are unambiguously concatenable
> and whether a single language `L` is unambiguously iterable."

The proof squares an NFA and looks for a path off the diagonal. It works because Boomerang's
lenses are **regular**.

`ceps-transduce` is not regular — `call` pushes a real stack. **That is the price of the
stack, and it should be paid knowingly:** the expressiveness that makes this engine able to
read DBC's nested structure is the same expressiveness that forfeits the static check
Boomerang uses to guarantee its laws hold. Boomerang proves well-behavedness at compile
time. This design cannot, and must demonstrate it on a corpus instead.

Which turns [G9](GOALS.md#g9--the-corpus-exists) from good practice into **the substitute for
a decision procedure** — and is a third, independent reason it belongs at stage 2 of the
order, before anything behavioural moves.

---

## 10. Bibliography

Verified against the authors' canonical sources, not reconstructed from memory. Epistemic
status is given per item, in keeping with [`PRIOR-ART.md`](PRIOR-ART.md).

**Read and quoted.**

- Aaron Bohannon, J. Nathan Foster, Benjamin C. Pierce, Alexandre Pilkiewicz and Alan
  Schmitt. *Boomerang: Resourceful Lenses for String Data.* POPL 2008, January 2008.
  — The laws in §3 are quoted from §2 of this paper; the `lbogus` example in §9 from §4.
  String lenses are the closest existing fit to what `ceps-transduce` does.

**Verified bibliographically, not yet read.**

- J. Nathan Foster, Michael B. Greenwald, Jonathan T. Moore, Benjamin C. Pierce and Alan
  Schmitt. *Combinators for bidirectional tree transformations: A linguistic approach to the
  view-update problem.* ACM TOPLAS 29(3), Article 17, May 2007.
  (Extended abstract: POPL 2005. Preliminary version: PLAN-X 2004.)
  — The foundational paper. Read before relying on anything in §2 beyond the law statements.

- Davi M. J. Barbosa, Julien Cretin, Nate Foster, Michael Greenberg and Benjamin C. Pierce.
  *Matching Lenses: Alignment and View Update.* ICFP 2010, September 2010.
  (Also UPenn TR MS-CIS-10-01, January 2010.)
  — The alignment problem: which chunk of the source corresponds to which element of an
  edited view. **Note:** this is the known hard part of applying lenses to list-structured
  data, and `ears.ceps.lex` is unusually well placed, because `tag{$0 _ $2;}` appears in all
  five model-emitting rules — every model element already carries the identity of the
  requirement it came from. Alignment is solved here by provenance that was written for
  traceability reasons before any of this came up.

- Tillmann Rendel and Klaus Ostermann. *Invertible syntax descriptions: unifying parsing and
  pretty printing.* Haskell Symposium 2010, pp. 1–12. DOI 10.1145/1863523.1863525.
  — One description, both directions, in a parser-combinator setting. The closest prior art
  to the §5 claim that the grammar is already bidirectional.

**Decidability — relevant because of the misnomer.**

- Meera Blattner and Tom Head. *Single-valued a-transducers.* JCSS 15(3):310–327, 1977.
  DOI 10.1016/S0022-0000(77)80033-0. — The clean citation for the result itself.
- Eitan M. Gurari and Oscar H. Ibarra. *A note on finite-valued and finitely ambiguous
  transducers.* Mathematical Systems Theory 16(1):61–66, 1983. DOI 10.1007/BF01744569.
  — Polynomial-time procedures for the general *k*-valued case; single-valuedness is `k = 1`.
- Helmut Seidl. *Single-valuedness of tree transducers is decidable in polynomial time.*
  TCS 106(1):135–181, 1992. — A different machine model; listed for completeness.
- M. P. Schützenberger. *Sur les relations rationnelles entre monoïdes libres.* TCS
  3(2):243–259, 1976. — Traditionally credited here. **Attribution received, not checked:**
  the entry is bibliographically verified, the content is not.

Single-valuedness of a finite-state transducer is decidable, and decidable in polynomial
time. **This does not help here**, and that is the misnomer biting a third time: the engine
is not finite-state — `call` pushes a real stack ([AUDIT.md](AUDIT.md), *What this component
actually is*).

Once there is an unbounded stack the question takes the shape of context-free ambiguity,
which is undecidable — Cantor, *On the Ambiguity Problem of Backus Systems*, JACM
9(4):477–479, 1962, and Floyd, *On ambiguity in phrase structure languages*, CACM 5(10):526,
1962. **The link is a reasoned corollary, not a cited theorem.** The reduction is standard —
a pushdown transducer emitting an encoding of its derivation tree is single-valued iff the
grammar is unambiguous — but no paper stating it for this machine class was found, and the
engine's exact position between regular and Turing-complete is deliberately unpinned.

The practical consequence is firm regardless: **the corpus is the only available method.**
There is no static check that will tell you the grammar is unambiguous — see §9, where
Boomerang's Fact 2.1 shows precisely what the stack costs.

**Prior art in the neighbourhood — and one terminological trap.**

- Holger Giese, Stephan Hildebrandt and Stefan Neumann. *Model Synchronization at Work:
  Keeping SysML and AUTOSAR Models Consistent.* In *Graph Transformations and Model-Driven
  Engineering*, LNCS Festschrift for Manfred Nagl, Springer 2010, pp. 555–579.
  DOI 10.1007/978-3-642-17322-6_24.
- Holger Giese and Robert Wagner. *From model transformation to incremental bidirectional
  model synchronization.* Software & Systems Modeling 8(1), 2009. DOI 10.1007/s10270-008-0089-9.

Bidirectional transformation applied to AUTOSAR **does exist**, via Triple Graph Grammars —
the other major BX paradigm alongside lenses. Any claim this work makes must be stated
against it, not in ignorance of it. The distinction to draw is the mechanism: TGGs relate two
metamodels through a correspondence graph maintained as a third artifact; a lens is one
artifact read twice. Which is better here is not obvious and should not be assumed.

**The trap:** to the requirements-engineering community, *bidirectional* means
**traceability** — forward and backward links between artifacts, maintained by hand or by
tool (Pohl, *Requirements Traceability*, DOI 10.1007/978-3-642-12578-2_17). That is a
weaker, informal notion and a different thing entirely. Any writing aimed at that audience
has to separate the two in the first paragraph or it will be read as a traceability paper.

**Searched and not found.** No prior art on bidirectional transformation of **DBC**
specifically — no round-tripping, no bidirectional DBC ↔ ARXML — and none applying lenses
with well-behavedness laws to **requirements engineering**. Searched: arXiv and Crossref,
bibliographic queries only. Treated as *unsearched rather than absent*; the negative result
in [`PRIOR-ART.md`](PRIOR-ART.md) §6 was arrived at the hard way and the same caution
applies.

**Explicitly not claimed.** That ARXML communication matrices are commonly *generated from*
DBC, with DBC remaining the editable source. This is widely believed and was the original
motivation for the `dbc ⟷ ceps ⟷ ARXML` sketch, but no vendor or standards document stating
it could be found. The circumstantial evidence — Vector still ships CANdb++ as the DBC
authoring tool, and `canmatrix` marks its ARXML *export* as "very basic implementation"
while its DBC export carries no such caveat — is consistent with it and does not establish
it. Not to be published as fact without someone with hands-on OEM or Tier-1 tooling
experience saying so.

