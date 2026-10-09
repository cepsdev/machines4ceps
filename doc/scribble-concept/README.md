# You don't erase a scribble

`you_dont_erase_a_scribble.ceps` is nineteen lines. It starts with two numbers
written down without a plan, and ends with a running state machine. Nothing in
between is edited and nothing is deleted — every step only *adds*.

Run it:

```sh
bin/ceps doc/scribble-concept/you_dont_erase_a_scribble.ceps
```

```
S.Initial- S.4+
S.4- S.5+
S.5- S.6+
S.Final+ S.6-
```

`X-` means *left state X*, `X+` means *entered state X*. The machine walked
`Initial → 4 → 5 → 6 → Final`.

Where did states named `4`, `5` and `6` come from? They were arithmetic four
lines earlier. That is the point of the file.

> Every output in this document is produced by `tools/check-doc-examples.py`,
> which extracts each example, runs it, and fails if the result changed.

## The idea, one append at a time

### 1. A scribble

Two numbers. No structure worth the name, no purpose yet.

```ceps
my_first_idea{a{1;};b{2;};};
```
```output
(STRUCT "my_first_idea"
  (STRUCT "a"
    (INT 1)
  )
  (STRUCT "b"
    (INT 2)
  )
)
```

`name{ ... }` is the only syntax here. It builds a node called `name` whose
content is whatever is between the braces — other nodes, or values. A ceps
document is a tree of these, and that tree is simultaneously the program, the
data, and the output.

### 2. A refinement that reads the scribble

Now something is done with it. `root` is the document itself, so
`root.my_first_idea.a` is a path to the node written a moment ago, and
`.content()` is what that node contains.

```ceps
my_first_idea{a{1;};b{2;};};
idea_refinement{root.my_first_idea.a.content() + root.my_first_idea.b.content(); };
```
```output
(STRUCT "my_first_idea"
  (STRUCT "a"
    (INT 1)
  )
  (STRUCT "b"
    (INT 2)
  )
)
(STRUCT "idea_refinement"
  (INT 3)
)
```

Two things to notice, and the second one matters more than it looks.

**`my_first_idea` is still there.** Evaluation rewrote `idea_refinement` in
place — `1 + 2` became `3` — and left everything else alone. The document now
holds both the input and the conclusion. If the `3` is wrong, the `1` and the
`2` are still on the page to argue with.

**`.content()` returned `1`, not a list containing `1`.** When a node holds
exactly one thing, `.content()` gives you that thing. This is deliberate: it
is what lets a scribble be written without ceremony. See
[Indexing, and one sharp edge](#indexing-and-one-sharp-edge) below — it has a
cost, and this file pays it.

### 3. A better idea, built from the refinement

```ceps
my_first_idea{a{1;};b{2;};};
idea_refinement{root.my_first_idea.a.content() + root.my_first_idea.b.content(); };
maybe_this_idea_is_better{for(i: 1 .. 3) {root.idea_refinement.content().at(0)+i;}};
```
```output
(STRUCT "my_first_idea"
  (STRUCT "a"
    (INT 1)
  )
  (STRUCT "b"
    (INT 2)
  )
)
(STRUCT "idea_refinement"
  (INT 3)
)
(STRUCT "maybe_this_idea_is_better"
  (INT 4)
  (INT 5)
  (INT 6)
)
```

`for(i: 1 .. 3) { ... }` runs the body for `i` = 1, 2, 3 and each result is
appended to the surrounding node. So `maybe_this_idea_is_better` holds
`3+1`, `3+2`, `3+3`.

The loop did not *mutate* anything. It produced three values, which is what a
node can hold.

### 4. Giving it a name

A `macro` names a piece of document so it can be spliced in elsewhere.
Invoking it with `name{}` inserts what it stands for.

```ceps
my_first_idea{a{1;};b{2;};};
idea_refinement{root.my_first_idea.a.content() + root.my_first_idea.b.content(); };
maybe_this_idea_is_better{for(i: 1 .. 3) {root.idea_refinement.content().at(0)+i;}};
macro give_that_idea_a_name{ root.maybe_this_idea_is_better.content();};
give_that_idea_a_name{};
```
```output
(STRUCT "my_first_idea"
  (STRUCT "a"
    (INT 1)
  )
  (STRUCT "b"
    (INT 2)
  )
)
(STRUCT "idea_refinement"
  (INT 3)
)
(STRUCT "maybe_this_idea_is_better"
  (INT 4)
  (INT 5)
  (INT 6)
)
(MACRO "give_that_idea_a_name" <ADDR>
)
(INT 4)
(INT 5)
(INT 6)
```

The last three lines are the expansion: `4`, `5`, `6` spliced into the document
at the point of the call.

`<ADDR>` stands for a pointer that differs on every run, which is why the
documentation checker masks hexadecimal addresses. It is a wart: output that
changes between two identical runs cannot be diffed, and that matters well
beyond documentation.

### 5. Turning numbers into a machine

This is the step where ceps stops looking like a configuration format.

```
lets_try_the_idea{
sm{S;
 val a = give_that_idea_a_name();
 states{Initial;Final;for(e : a ){as_identifier(text(e));} };
 t{Initial;as_identifier(text(a.content().at(0)));};
 for(e:a.content()){
    if (is_defined(next)) { t{as_identifier(text(e));as_identifier(text(next));}; }
    else {t{as_identifier(text(e));Final;};}
 }
};};
```

Line by line:

| | |
|---|---|
| `sm{S; ... }` | declares a state machine named `S`. State machines are part of the core language, not a library. |
| `val a = give_that_idea_a_name();` | binds the macro's expansion — the three integers — to `a`. |
| `as_identifier(text(e))` | `text(e)` renders the value `4` as the text `"4"`; `as_identifier` turns that text into a **name**. This is the hinge of the whole file: a number becomes an identifier becomes a state. |
| `states{Initial;Final; for ... }` | the state set is *computed*: two fixed states plus one per element of `a`. |
| `t{X;Y}` | a transition from `X` to `Y`. |
| `is_defined(next)` | inside `for(e: ...)`, `next` is the following element — defined for every element but the last. So the loop chains `4→5→6` and sends the last one to `Final`. |

What that produces:

```ceps
my_first_idea{a{1;};b{2;};};
idea_refinement{root.my_first_idea.a.content() + root.my_first_idea.b.content(); };
maybe_this_idea_is_better{for(i: 1 .. 3) {root.idea_refinement.content().at(0)+i;}};
macro give_that_idea_a_name{ root.maybe_this_idea_is_better.content();};
give_that_idea_a_name{};
lets_try_the_idea{
sm{S;
 val a = give_that_idea_a_name();
 states{Initial;Final;for(e : a ){as_identifier(text(e));} };
 t{Initial;as_identifier(text(a.content().at(0)));};
 for(e:a.content()){
    if (is_defined(next)) { t{as_identifier(text(e));as_identifier(text(next));}; }
    else {t{as_identifier(text(e));Final;};}
 }
};};
root.lets_try_the_idea.sm;
```
```output
(STRUCT "my_first_idea"
  (STRUCT "a"
    (INT 1)
  )
  (STRUCT "b"
    (INT 2)
  )
)
(STRUCT "idea_refinement"
  (INT 3)
)
(STRUCT "maybe_this_idea_is_better"
  (INT 4)
  (INT 5)
  (INT 6)
)
(MACRO "give_that_idea_a_name" <ADDR>
)
(INT 4)
(INT 5)
(INT 6)
(STRUCT "lets_try_the_idea"
  (STRUCT "sm"
    (ID "S"
    )
    (STRUCT "states"
      (ID "Initial"
      )
      (ID "Final"
      )
      (ID "4"
      )
      (ID "5"
      )
      (ID "6"
      )
    )
    (STRUCT "t"
      (ID "Initial"
      )
      (ID "4"
      )
    )
    (STRUCT "t"
      (ID "4"
      )
      (ID "5"
      )
    )
    (STRUCT "t"
      (ID "5"
      )
      (ID "6"
      )
    )
    (STRUCT "t"
      (ID "6"
      )
      (ID "Final"
      )
    )
  )
)
(STRUCT "sm"
  (ID "S"
  )
  (STRUCT "states"
    (ID "Initial"
    )
    (ID "Final"
    )
    (ID "4"
    )
    (ID "5"
    )
    (ID "6"
    )
  )
  (STRUCT "t"
    (ID "Initial"
    )
    (ID "4"
    )
  )
  (STRUCT "t"
    (ID "4"
    )
    (ID "5"
    )
  )
  (STRUCT "t"
    (ID "5"
    )
    (ID "6"
    )
  )
  (STRUCT "t"
    (ID "6"
    )
    (ID "Final"
    )
  )
)
```

Five states and four transitions, none of them written by hand. And the
arithmetic they came from is still sitting above them in the same document.

### 5a. Installing it — the least obvious line in the file

The machine appears **twice** in that output: once nested inside
`lets_try_the_idea`, and once on its own at the end. The second copy is the
work of the file's shortest line:

```
root.lets_try_the_idea.sm;
```

A path written as a statement **splices what it names into the document at that
point**. So this line lifts the generated machine out of the node that built it
and places it at the top level, which is where the simulator looks for machines.

It is load-bearing. Delete it and the file still parses, still evaluates, still
builds the machine — and then fails at the last moment:

```
***Fatal Error:Start-directive: Expression doesn't evaluate to an existing state: (ID "S" )
```

(exit code 1). The machine existed; it was simply never installed.

This is worth dwelling on, because it is the same idea as everything above.
Nothing was moved and nothing was converted. A node was *named*, and naming it
in statement position is what put it where it had to be.

### 6. Running it

```
Simulation{
    Start{root.lets_try_the_idea.sm.content().at(0);};
};
```

`Simulation{Start{...}}` names the machine to start. `root.lets_try_the_idea.sm`
is the machine node; `.content().at(0)` is its first element, which is the
identifier `S`. The machine is referred to *by navigating to it*, the same way
every other value in this file is reached.

And that is the whole file — nineteen lines, every one of them an addition to
the one before:

```ceps run
my_first_idea{a{1;};b{2;};};
idea_refinement{root.my_first_idea.a.content() + root.my_first_idea.b.content(); };
maybe_this_idea_is_better{for(i: 1 .. 3) {root.idea_refinement.content()+i;}};
macro give_that_idea_a_name{ root.maybe_this_idea_is_better.content();};
give_that_idea_a_name{};
lets_try_the_idea{
sm{S;
 val a = give_that_idea_a_name();
 states{Initial;Final;for(e : a ){as_identifier(text(e));} };
 t{Initial;as_identifier(text(a.content().at(0)));};
 for(e:a.content()){
    if (is_defined(next)) { t{as_identifier(text(e));as_identifier(text(next));}; }
    else {t{as_identifier(text(e));Final;};}
 }
};};
root.lets_try_the_idea.sm;
Simulation{
    Start{root.lets_try_the_idea.sm.content().at(0);};
};
```
```output
S.Initial- S.4+
S.4- S.5+
S.5- S.6+
S.Final+ S.6-
```

Four steps: `Initial → 4 → 5 → 6 → Final`. `X-` is *left X*, `X+` is
*entered X*; the two halves of a step are not printed exit-before-entry, so
read the names rather than the order.

## Why this file exists

### Every stage is still in the artifact

Look at the final output: the original `1` and `2`, the sum, the three derived
values, the macro, the generated machine. A conventional toolchain would have
a model file containing only the end result, and the reasoning that produced it
would live in a ticket, a wiki page, or somebody's memory — where it rots,
silently, because nothing breaks when it goes stale.

Here the reasoning is *upstream of the model in the same file*, and it is
executable. It cannot drift from what it produced, because if it drifts the
file stops running.

### Nothing was edited

Each step reads what came before and appends. No line is rewritten to
accommodate a later idea. That is a property worth having when a human changes
their mind, and it is a stronger property when a program is doing the writing:
the medium's only operation is accretion, so a generator cannot silently
destroy work that already runs.

### Data, names, structure and behaviour are one language

`4` is an integer, then a name, then a state, then something a trace reports
entering — without leaving the language, without a code generator, and without
a separate model format. `as_identifier(text(e))` is the whole bridge.

## Indexing, and one sharp edge

`.content()` yields what a node holds. `.at(n)` selects the `n`-th element,
counting from zero.

```ceps
a{10;20;30;};
first{root.a.content().at(0);};
second{root.a.content().at(1);};
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
(STRUCT "second"
  (INT 20)
)
(STRUCT "third"
  (INT 30)
)
```

`.at(n)` is the indexer. **`.content(n)` is not** — it accepts the argument and
ignores it. See [`DEFECTS.md`](../../DEFECTS.md), D18.

And here is the edge this file walks along. Because `.content()` collapses a
single element to that element, an expression that works today can stop working
when the node it reads *grows* — and it does so quietly:

```ceps
a{1;5;};
b{2;};
sum_unchanged{root.a.content() + root.b.content();};
sum_with_at{root.a.content().at(0) + root.b.content().at(0);};
```
```output
(STRUCT "a"
  (INT 1)
  (INT 5)
)
(STRUCT "b"
  (INT 2)
)
(STRUCT "sum_unchanged"
  (OPERATOR + ""
    (NODESET ""
      (INT 1)
      (INT 5)
    )
    (INT 2)
  )
)
(STRUCT "sum_with_at"
  (INT 3)
)
```

`sum_unchanged` would have been `3` if `a` held only `1`. Adding `5` changed
its meaning without changing its text, and ceps exits 0 regardless.

The half-evaluated `(OPERATOR + ...)` is not nothing — it shows exactly where
evaluation stopped, which is how the problem is usually found. But it does not
*announce* itself, and that is the defect: see D18 again, which proposes
rejecting a sequence of length ≠ 1 wherever a single value is required.

**Practical rule until then:** write `.at(n)` whenever the node you are reading
could ever gain a second element. In this very file the rule is applied
inconsistently — no `.at(0)` on `a` and `b`, `.at(0)` on `idea_refinement` —
because `a` and `b` are leaves that will not grow, and `idea_refinement` might.
That judgement is nowhere checked.

## See also

- [`CEPS-LANG.md`](../../CEPS-LANG.md) — the language reference this example belongs to
- [`DEFECTS.md`](../../DEFECTS.md) — D18 for the `.content()` behaviour above
- [`tools/check-doc-examples.py`](../../tools/check-doc-examples.py) — what keeps this file honest
