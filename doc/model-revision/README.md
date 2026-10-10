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
2. **Supersession is explicit.** A revision records which node it supersedes and by which
   rewrite. Given the document alone, a reader can tell revision from coincidence of
   names.
3. **The lift is reproducible.** For a recorded lift `L` and dependent `B`,
   re-running `apply(L, B)` reproduces the stored `B'` byte for byte. A test asserts this
   on at least one multi-stage example.
4. **Partial lifts are represented, not dropped.** When `B` cannot be lifted through `A'`,
   `B` remains in the document, is marked unsupported, and the condition is reported.
   Nothing is silently absent.
5. **Unaffected dependents are shared, not copied.** If lifting leaves `C` unchanged, the
   document does not grow by a second copy of `C`. Without this the file is unreadable
   after a handful of revisions; `A,B,C,A',B',C',A''…` is `O(n·k)` otherwise.
6. **Order independence is checked, not assumed.** Revising `A` then `B` and revising `B`
   then `A` either agree, or the disagreement is reported. See below.
7. **Every example in this document is executed by `tools/check-doc-examples.py`** and its
   output matches.

<a name="open"></a>
## Open questions

- **Confluence.** Criterion 6 is a confluence property of the rewrite system, and it is
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
