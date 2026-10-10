# Model revision without erasure

**Status:** design note. Nothing here is implemented. Proposed 2026-10-10.

> You start with A, and you derive B and C. Now you revisit A and it becomes A'.
> If revising in place is forbidden — forbidden as an axiom, since nobody can stop you
> editing a file — then you append: `A, B, C, A'`. What about B and C? They get lifted
> through A' to give `A, B, C, A', B', C'`, and the whole process can be reiterated.
> That's git for models.

This note records what the notation already supports, what it does not, and what would
have to be decided before any of it is built.

The axiom is the same one as `doc/scribble-concept/`: **you do not erase a scribble.**
That document applies it to *generalization* — the concrete instance is kept beside the
rule it was generalized into. This one applies it to *revision* over time. They are the
same axiom pointed at two different axes, which is the reason to take the idea seriously
rather than treat it as a feature request.

<a name="today"></a>
## What the notation already gives you

More than expected. Three properties fall out of existing semantics, with no new syntax.

### A path denotes every match, in document order

`root.interface` is not "the interface". It is *all* nodes named `interface` below `root`,
in the order they appear in the document. Appending a revision therefore extends a
sequence rather than overwriting an entry, and the sequence is already in revision order:

```ceps
interface{ version{1;}; speed{int;}; };
interface{ version{2;}; speed{int;}; heading{int;}; };
root.interface.version.content();
```
```output
(STRUCT "interface"
  (STRUCT "version"
    (INT 1  )
  )
  (STRUCT "speed"
    (ID "int"
    )
  )
)
(STRUCT "interface"
  (STRUCT "version"
    (INT 2  )
  )
  (STRUCT "speed"
    (ID "int"
    )
  )
  (STRUCT "heading"
    (ID "int"
    )
  )
)
(INT 1  )
(INT 2  )
```

The last two lines are the whole version history of `interface`, obtained by a path
expression that nobody designed for the purpose. **Append order is document order is
revision order.** Temporal databases buy this property deliberately; here it is the
default meaning of a path.

### A revision is selectable by index

```ceps
interface{ version{1;}; };
interface{ version{2;}; };
root.interface.at(1);
```
```output
(STRUCT "interface"
  (STRUCT "version"
    (INT 1  )
  )
)
(STRUCT "interface"
  (STRUCT "version"
    (INT 2  )
  )
)
(STRUCT "interface"
  (STRUCT "version"
    (INT 2  )
  )
)
```

### A staged computation can already look backwards

`predecessor()` refers to the preceding sibling node, and is documented as the way a
staged computation is expressed (`INVENTORY.md:433`). In an append-only document where
revisions are appended as siblings, *preceding sibling* is *previous version*. The
traversal primitive the design needs exists and has existed for years.

<a name="missing"></a>
## What is missing

Three things, in the order they bite.

### 1. There is no way to say "the current one"

The most common query in any revision scheme is *the latest*. It cannot be written.

```ceps
interface{ version{1;}; };
interface{ version{2;}; };
root.interface.at(-1);
```
```output
(STRUCT "interface"
  (STRUCT "version"
    (INT 1  )
  )
)
(STRUCT "interface"
  (STRUCT "version"
    (INT 2  )
  )
)
(UNDEFINED ""
)
```

`(UNDEFINED "")` and **exit 0** — negative indices are not supported, and saying so is
not part of the behaviour. A `.last()`, or a `.size()` to compute the index from, is the
single smallest change that makes the rest of this note writable by a user.

Note that this is the house failure mode (`DEFECTS.md`, the silent-failure class) landing
on the one operation the design cannot do without.

### 2. The sequence records an order, not a revision relation

`root.interface` cannot distinguish *three revisions of one interface* from *three
different interfaces that happen to share a name*. Document order gives an order. It does
not give a claim that the second supersedes the first.

This is the argument for making the lift a recorded object rather than a recomputed one.
If `A'` carries the rewrite that produced it — *revises `A` via `L`* — then:

- the supersession relation is explicit, so the conflation above disappears;
- `B' = apply(L, B)` is reproducible, and can be checked rather than asserted;
- the history stays in one notation, because `L` is a ceps term like everything else.

Tree rewriting is not a new mechanism here. The yamdl commit of 2013-11-15 (`a65f0e4`)
is titled *"Bugfixes. Tree Rewrites for binary operators. Minor Enhancements."* The
machinery predates the question by thirteen years.

Hand-writing `B'` instead is the alternative, and it is strictly worse: nothing then
relates `B'` to `B`, and the document becomes a pile with a convention about which end is
newest.

### 3. The lift is partial, and the axiom forbids the obvious response

If `A'` removes something `B` depended on, `B'` does not exist. Append-only means `B`
cannot be deleted either. So the design needs a **status**, not a removal: `B` retained,
marked unsupported under `A'`. Truth-maintenance systems call this being *out*.

This has to be decided before anything is built, because it changes the shape of the
data rather than adding to it.

<a name="partiality"></a>
## Partiality as the diagnostic

A lift can fail. Criterion 4 below requires that a failure be *represented* rather than
dropped, and this section says how it is observed — because the mechanism already exists
in the evaluator and needs only to be made uniform.

Normalization reduces what it can and leaves what it cannot. The part it leaves is not
debris; it is **the impact report**. Change a partition, re-normalize, and whatever fails
to reduce is exactly the blast radius. No dependency tracker is required, because the
normalizer is already one.

### Where it works: an unbound identifier survives

With the binding in place the expression disappears into its value:

```ceps
val b = 2;
expr{ 1 + b; };
```
```output
(STRUCT "expr"
  (INT 3  )
)
```

Remove the binding — the first half of a rename, or the whole of a removal — and the
expression survives, naming what it is missing, in context:

```ceps
expr{ 1 + b; };
```
```output
(STRUCT "expr"
  (OPERATOR + ""
    (INT 1  )
    (ID "b"
    )
  )
)
```

Exit 0, and correctly so: nothing is wrong, something is *unknown*. The set of surviving
`(ID "b")` nodes is the set of places a lift must touch. It is greppable, and it is in
the same notation as the model.

### Where it fails: an unresolved path is annihilated

```ceps
A{ a{1;}; };
holder{ root.A.b; };
holder2{ root.A.a; };
```
```output
(STRUCT "A"
  (STRUCT "a"
    (INT 1  )
  )
)
(STRUCT "holder"
)
(STRUCT "holder2"
  (STRUCT "a"
    (INT 1  )
  )
)
```

`holder2` resolves. `holder` is **empty**, exit 0, and nothing survives to say why. A
reader cannot distinguish *`b` was removed by a revision* from *`b` never existed* from
*the path is a typo* from *the partition is not loaded*.

**So the rule this design needs is one sentence: an unresolved path must survive the way
an unbound identifier does.** `root.A.b` should normalize to a node carrying the path it
failed on, not to nothing. Until it does, half of every impact report is invisible, and
the half that is invisible is the half that involves the model rather than a binding.

This also reframes [D17](../../DEFECTS.md): its worst symptom — a fully qualified
`root.m.content()` returning empty with exit 0 — is annihilation, not collision. The
collision decides *which* lookup fails; annihilation is why nobody finds out. Fixing path
residue does not fix D17, but it converts it from a silent fault into a visible one, which
is the difference between a defect and a diagnostic.

### The asymmetry nobody expects

Partiality does not cover all revisions equally, and the ranking is counter-intuitive:

| change to `A` | residue produced | verdict |
|---|---|---|
| add a member | none needed — monotone, no dependent breaks | safe |
| remove or rename a member | every use survives as `(ID "b")` | **loud, therefore safe** |
| **change the meaning of a member** | **none** — `1 + b` still reduces, to a different number | **silent by construction** |

Removal and renaming look like the destructive cases and are in fact the benign ones,
because they are the ones partiality catches. The dangerous change is a redefinition that
keeps the shape: nothing becomes unknown, so nothing is residualized, so there is nothing
to inspect.

**This decides the append-versus-replace question, which is otherwise a matter of taste.**
The two forms defend against disjoint failures:

- **Replace the partition** — `(A')(B)(C)` — relies on residue, and therefore covers
  *structural* change.
- **Append the revision** — `(A)(B)(C)(A')` — keeps both versions present and diffable,
  and therefore covers *semantic* change, which residue cannot see.

Both must be permitted. Replacement is not the cheap option; it is a different defence
with a different blind spot, and a scheme offering only one of them is unsound for half
of the changes people actually make.

### Prior art for this part specifically

Residualization is classical: partial evaluation in the sense of Jones, Gomard and
Sestoft, where the residual program is precisely what could not be computed from the
known inputs, and symbolic execution, where unbound values stay symbolic and the residue
describes what is unknown. **Hazel** is the closest living relative — live programming
with typed holes, in which incomplete programs still evaluate and the holes are
observable rather than fatal.

The difference here is the one worth claiming: the hole is a *model* hole rather than a
program hole, and the residue is in the same notation as the model it came from, so an
impact report is queryable by the same path expressions as everything else.

<a name="closure"></a>
## Closed and open specifications

Partiality supports a definition that is worth more than the diagnostic it came from.

> A specification is **closed** if nothing residual remains in the evaluated document,
> and **open** if something does.

This is the ground-term test from term rewriting, and Maude's own distinction between a
term and a pattern. It is reached here from the other direction — from what the evaluator
happens to leave behind — which is a point in its favour: the criterion is not imposed on
the language, it is read off it.

The useful observation is that *why* something remains matters, and the evaluated document
already says why.

### Three outcomes the document already distinguishes

A binding reduces away entirely. Nothing is left, and the specification is **ground-closed**:

```ceps
val b = 1;
b + 1;
```
```output
(INT 2  )
```

An undeclared name survives as an `ID`. Something is missing that nobody has decided yet —
a **hole**, and the specification is **open**:

```ceps
b + 1;
```
```output
(OPERATOR + ""
  (ID "b"
  )
  (INT 1  )
)
```

Declare it and the residue remains, but changes character. It is now a `SYMBOL`, tagged
with its kind — not a gap in the specification but a **decision** in it, and the
specification is **closed modulo its declared parameters**:

```ceps
kind B;
B b;
b + 1;
```
```output
(OPERATOR + ""
  (SYMBOL "b" "B"
  )
  (INT 1  )
)
```

`ID` versus `SYMBOL` is meta-typing, and it works today. The distinction between *I have
not decided* and *I have decided to leave this abstract* is carried in the node type, with
the kind attached. Closure is therefore computable by walking the evaluated document,
which ceps can already do to its own documents — `.fetch_recursively_symbols()` and the
ordinary path expressions. **A closure checker is a ceps program, not a change to the
evaluator.**

### The outcome it does not distinguish, which breaks the criterion

A specification can be wrong without being open. Implicit coercion is the mechanism:

```ceps
val b = "a string";
b + 1;
1 + b;
b * 2;
```
```output
"a string1"
"1a string"
"a stringa string"
```

Exit 0, fully reduced, no residue; `*` is string repetition. By the definition above this
specification is **closed**: nothing is missing, nothing is stuck, there is nothing left
to fill in. It is also nonsense.

Note what the change was. Replacing `val b = 1;` with `val b = "a string"` is a change of
meaning that preserves shape — the bottom row of the table in
[Partiality as the diagnostic](#partiality), the row marked *no residue, silent by
construction*. The result moves from `(INT 2  )` to `"a string1"` and the specification
stays closed across the transition. The asymmetry recorded in that section predicted this
case before it was tested.

### Four states, of which one is missing

The criterion needs a third kind of residue: a term that is irreducible because **no rule
can ever apply**, as distinct from one that is irreducible because *no rule has been
written yet*.

| in the evaluated document | state | meaning |
|---|---|---|
| no residue | **ground-closed** | fully determined |
| `(ID "x")` | **open** | a hole; someone must decide |
| `(SYMBOL "x" "K")` | **closed modulo parameters** | a decision to stay abstract |
| an error term — **not implemented** | **inconsistent** | a commitment that cannot hold |

Maude has already named the missing one. A *kind* there is the error supersort: a term
that is well formed but ill sorted inhabits `[Nat]` rather than `Nat`, so a type error is
an inspectable term rather than an abort. ceps already spends the keyword `kind` on
declarations, and the third category is what a kind would be for.

### What this costs, and it is not small

**Implicit coercion is incompatible with the criterion.** Every coercion converts a term
that should have got stuck into a value, and therefore punches a hole in the diagnostic.
For closure to mean anything, `string + int` must residualize rather than oblige. That is
a breaking change to arithmetic, and it is the price of the definition; it should be
decided deliberately rather than discovered later.

It also names a pattern that runs through `DEFECTS.md` more usefully than "the
silent-failure class" does. A name collides with an SI unit and the unit wins
([D17](../../DEFECTS.md)). A path fails to resolve and evaluates to nothing instead of
residualizing. An operator meets mismatched operands and coerces instead of refusing.
These are not three absences of code. They are three instances of the same disposition —
**the evaluator is too accommodating** — and the fix direction is the same in each case:
decline, and say so.

<a name="closure-open"></a>
### The open question: closure is relative to a signature

`symbol b + 1` is a ground term, since `b` is a constant rather than a variable, so the
definition above calls it closed. But it is irreducible because `+` has no rule for
`B` and an integer. Whether that is a *decision* or a *conflict* depends on whether `+`
over `B` was ever declared — and today there is no way to declare it.

So the criterion as stated moves the ambiguity rather than removing it. Closure is
relative to a **declared signature**, not merely to the absence of `ID` nodes, and ceps
has no signature to be relative to. Until operations can be declared over kinds, a
deliberate abstraction and a genuine type error are the same picture.

This is the part of the proposal that needs design rather than implementation.

<a name="defects"></a>
## Relationship to D18, which is this idea filed as a defect

> **D18. Appending to a node silently breaks every reader that assumed a singleton.**

That is this design, stated as a fault. Append-only revision and D18 are the same
operation observed from opposite ends.

The consequence is a reframing, and it makes D18 cheaper rather than more expensive.
D18 currently reads as *appending is dangerous, so be careful*. Under this design,
appending is the point and **there are no singletons**, so the defect is not that
appending breaks singleton readers — it is that *a reader can assume a singleton silently*.
The fix is therefore not "make append safe". It is "make the singleton assumption
impossible to state without saying so", which is a smaller change, and it promotes the
discipline already recorded in the entry (*always `.at(n)`*) from a workaround to the
house rule.

**D17 gets worse here, not better.** If a model name collides with an SI unit, the path
does not lose one node — it loses the entire history, silently, exit 0. The blast radius
scales with the number of revisions. D17 is a prerequisite for this work, not an
unrelated cleanup.

<a name="prior-art"></a>
## Prior art

Stated plainly, because the distinctive part is narrow and is easier to see once the
rest is attributed.

| work | what it contributes |
|---|---|
| Truth-maintenance systems (de Kleer's ATMS) | justifications propagated when a premise changes; multiple contexts held live at once, which is `A` and `A'` both valid with dependents labelled by which they rest on |
| AGM belief revision | expansion, contraction, revision — "what else must go when I retract" is their problem statement |
| Darcs / Pijul patch theory | patches as first-class objects that commute; lifting `B` through `A → A'` is patch commutation |
| Unison | content-addressed definitions; `update` propagates dependents to new hashes — the closest working system |
| Transport in dependent type theory | tells you *when* the lift is free: if `A'` is equivalent to `A` it is mechanical, otherwise it is a genuine obligation |
| Event sourcing, bitemporal databases, Datomic | the data-management form: never delete, project the current view |
| Self-adjusting computation (Acar) | change propagation through a dependency graph |

What is not on that list: **a system in which the history, the lift and the model are the
same notation and the history is walkable by the same path expressions as the model.** In
git the history sits outside the artifact and has no semantics. Unison comes closest and
its history is still a separate layer. That is the claim worth defending, and it is
smaller and more defensible than "git for models".

<a name="criteria"></a>
## Acceptance criteria

A first implementation is done when all of the following hold.

1. **Latest is expressible.** `root.X.last()` (or `.at(root.X.size()-1)`) yields the final
   element of a nodeset, and an out-of-range index is a diagnostic with a non-zero exit
   rather than `(UNDEFINED "")`.
2. **Unresolved paths survive.** A path that resolves to nothing normalizes to a node
   carrying the path it failed on, in the same way an unbound identifier survives as
   `(ID "b")`. Nothing is silently absent from the output. This is the prerequisite for
   criterion 4 and for any impact report at all; see
   [Partiality as the diagnostic](#partiality).
3. **Supersession is explicit.** A revision records which node it supersedes and by which
   rewrite. Given the document alone, a reader can tell revision from coincidence of
   names.
4. **The lift is reproducible.** For a recorded lift `L` and dependent `B`,
   re-running `apply(L, B)` reproduces the stored `B'` byte for byte. A test asserts this
   on at least one multi-stage example.
5. **Partial lifts are represented, not dropped.** When `B` cannot be lifted through `A'`,
   `B` remains in the document, is marked unsupported, and the condition is reported.
   Nothing is silently absent.
6. **Unaffected dependents are shared, not copied.** If lifting leaves `C` unchanged, the
   document does not grow by a second copy of `C`. Without this the file is unreadable
   after a handful of revisions; `A,B,C,A',B',C',A''…` is `O(n·k)` otherwise.
7. **Order independence is checked, not assumed.** Revising `A` then `B` and revising `B`
   then `A` either agree, or the disagreement is reported. See below.
8. **Closure is reportable.** A tool — written in ceps, walking the evaluated document —
   classifies a specification as ground-closed, open, or closed modulo declared
   parameters, and lists the `ID` nodes that make it open. See
   [Closed and open specifications](#closure). The fourth state, *inconsistent*, waits on
   the signature question recorded there.
9. **Every example in this document is executed by `tools/check-doc-examples.py`** and its
   output matches.

<a name="open"></a>
## Open questions

- **Confluence.** Criterion 7 is a confluence property of the rewrite system, and it is
  the question that decides whether the scheme is sound rather than merely convenient.
  This is the one place where a model checker earns its keep, and the project's stated
  route applies: walk the model, emit a Maude specification, check it there. It is a use
  of formal tooling at the edge of the system rather than at its centre.
- **Merge.** Two parties lift the same `B` through different revisions of `A`. Git
  resolves this textually and without understanding. A typed lift admits a better answer
  in restricted cases and no answer in general; the restricted cases need naming.
- **Presentation.** A document containing its whole history is correct and unreadable.
  The projection to "current view" is a tooling question, not a language question, but it
  decides whether anyone adopts it.
- **Interaction with simulation.** If a state machine is revised mid-document, which
  version does a `Simulation` see? Criterion 1 answers it by convention (the last), but
  the convention should be stated rather than inherited from whichever reader runs first.

<a name="see-also"></a>
## See also

- `doc/scribble-concept/README.md` — the same axiom applied to generalization
- `CEPS-LANG.md#phases` — where in execution a revision would be resolved
- `DEFECTS.md` — D17 (prerequisite), D18 (this idea, filed as a fault)
