# Step by step: from a requirements document to a running state machine

**Who this is for.** You write requirements, or you review them. You can read code — if
you have written TypeScript you will be fine — but you do not want a lecture on parsing
theory. Everything here is explained in terms you already use.

**What you will end up with.** A `.req` file you can hand to a requirements engineer, and
a command that turns it into a state machine you can watch run, where every step names the
requirement that allowed it.

**Time.** About twenty minutes if you type it out.

---

## The shape of the whole thing

The whole program is one line, and it reads as a sentence:

```
ceps   ears.ceps.lex   door_lock.req   extract.ceps   simulate.ceps
       how to read     what to read    what to make   what to do
```

Four files, four clauses. Each is a layer laid on the ones before it, and each sees only
what is already underneath: the grammar knows nothing about state machines, the document
knows nothing about the grammar reading it, the transform walks whatever the document
left behind.

That is the entire mechanism — **one operation, *add this to the model*, applied four
times.** There is no build file, no configuration, no plugin registration, no
`--grammar=`, `--input=`, `--transform=`. The order *is* the composition, which is why
nothing ever has to declare what depends on what.

The same thing drawn as data moving:

```
door_lock.req  ──[ ears.ceps.lex ]──>  a tree of data  ──[ extract.ceps ]──>  a state machine
   English              grammar            (the model)         transform           that runs
```

Nothing in there is text templating, and there is no code-generation step hiding
anywhere.

Two consequences, both worth more than they look.

**Nothing intermediate exists.** No `.ast`, no `.json`, no generated source to name,
version, rebuild or delete. The model is handed from layer to layer inside a single run
of a single program.

**Every position is a free variable.** Swap `door_lock.req` for another system,
`simulate.ceps` for another scenario, `extract.ceps` for another reading of the same
requirements, `ears.ceps.lex` for another requirement dialect — four independent axes.
And layers can be *added*: `./run.sh --bad` is not a mode, it simply puts one more
document into the sentence,

```sh
ceps ears.ceps.lex door_lock.req nonconforming.req extract.ceps simulate.ceps
```

which is why demonstrating the conformance report in Step 8 took no code at all.

One qualifier. The word order is rigid, and it is not quite the order you would speak:
the grammar comes *before* the document it reads, where English would say *read
`door_lock.req` using `ears.ceps.lex`*. Get it wrong and you get parse errors rather than
a clear complaint, so keep to **grammar, then documents, then transforms.**

---

## Step 1 — Get ceps running

From the repository root:

```sh
./make_machines.sh          # or: make -j TARGET=bin
```

That produces `bin/ceps`. The example's `run.sh` finds it automatically, or you can point
at your own build with `CEPS=/path/to/ceps ./run.sh`.

Check it works:

```sh
cd examples/incose/hello_world
./run.sh
```

If you see a trace with `[REQ-002]` in it, everything below will work.

---

## Step 2 — Write one requirement

Start with a single sentence in `door_lock.req`:

```
REQ-002
While the controller is unlocked, when lock_requested is received,
the controller shall transition to the locked state.
```

This is the EARS *state-driven + event-driven* pattern. It has four pieces of information
in it, and your job in Step 3 is to tell the tool where they are:

| piece | value here | becomes |
|---|---|---|
| identifier | `REQ-002` | the trace label, and the name of the generated action |
| precondition state | `unlocked` | the state the transition leaves from |
| trigger | `lock_requested` | the event |
| response state | `locked` | the state the transition arrives at |

Two notes before you write more sentences.

**Use `transition to`, not `enter`.** The pattern language reserves `enter` and `exit` for
its own purposes, so those two words cannot appear as literal text in a requirement
sentence. This is a genuine constraint the prototype works around, not a style choice.
(`SPEC.md` §3.3.)

**`//` comments are allowed** in a `.req` file, which is useful for keeping notes beside
requirements.

---

## Step 3 — Write the rule that reads it

Open `ears.ceps.lex`. The file starts with two declarations:

```
%ext_req        // this grammar claims any file ending in .req
%ignore_ws      // whitespace and line breaks are not significant
%%
```

Then come the rules. Here is the one that reads REQ-002:

```
ident - integer While the ident is ident , when ident is received ,
  the ident shall transition to the ident state \. =>
  incose_transition{
    req{double_quote $0 - $2 double_quote;};
    tag{$0 _ $2;};
    system{$5;};
    from{$7;};
    on{$10;};
    to{$20;};
  }; .
```

(The real rule in the file has one more field, `trace{…}`, which Step 6 explains. Ignore
it for now.)

### 3.1 The left-hand side is a pattern over words

Everything before `=>` describes the shape of a sentence, one word at a time:

- a bare word like `While` or `shall` must appear **literally**;
- `ident` matches **any identifier** — this is where the information is;
- `integer` matches a number;
- `,` matches a comma;
- `\.` matches a full stop. The backslash is needed because `.` means something else.

So the pattern reads: *an identifier, a hyphen, a number, the word `While`, the word
`the`, any identifier, the word `is`, any identifier, a comma…* and so on. If the sentence
in front of it has that shape, the rule fires.

### 3.2 Counting the slots — the one thing people get wrong

`$0`, `$7`, `$20` refer to matched words **by position, counting from zero, including
punctuation**. Punctuation is the part everyone forgets. Here is the full count for the
rule above:

| | | | | |
|---|---|---|---|---|
| `$0` ident → `REQ` | `$1` `-` | `$2` integer → `002` | `$3` `While` | `$4` `the` |
| `$5` ident → `controller` | `$6` `is` | **`$7` ident → `unlocked`** | `$8` `,` | `$9` `when` |
| **`$10` ident → `lock_requested`** | `$11` `is` | `$12` `received` | `$13` `,` | `$14` `the` |
| `$15` ident → `controller` | `$16` `shall` | `$17` `transition` | `$18` `to` | `$19` `the` |
| **`$20` ident → `locked`** | `$21` `state` | `$22` `.` | | |

That is why the rule says `from{$7;}`, `on{$10;}`, `to{$20;}`.

Now compare the *unwanted behaviour* rule (REQ-004), which says `if … then …` instead of
`when …`. That extra word `then` shifts everything after it by one, so the response state
is at `$21`, not `$20`. **Off-by-one here is the most common mistake when adding a
pattern.** If a rule produces the wrong field, count again before suspecting anything else.

### 3.3 The right-hand side writes data

Everything after `=>` is emitted whenever the rule fires. `incose_transition{ … }` is a
labelled bag of fields — if you think in TypeScript, imagine it producing:

```ts
{ incose_transition: { req: "REQ-002", from: unlocked, on: lock_requested, to: locked } }
```

with one difference that matters later: `unlocked` is a **name**, not the string
`"unlocked"`.

Two small devices in there:

- `double_quote $0 - $2 double_quote` glues the pieces into the string `"REQ-002"`.
- `$0 _ $2` glues them with an underscore into the **name** `REQ_002`. A hyphen cannot
  appear in a name, which is why the identifier is carried in both forms.

### 3.4 Rule order, and the rule that catches failures

Rules are tried **in the order they appear**. The last real rule in the file is:

```
ident - integer =>
  incose_nonconforming{ req{double_quote $0 - $2 double_quote;}; … }; .
```

It matches nothing but a requirement identifier, so it fires for any requirement whose
sentence matched none of the patterns above it. **It must stay last**, because it is a
prefix of every other rule — put it first and it would swallow everything.

This rule is what makes failure visible. A grammar here is deliberately *partial*: it
reads what it recognises and is allowed to skip the rest. That is fine for importing a CAN
database, where you only want three fields out of a hundred. It is dangerous for
requirements, where a skipped sentence means a missing transition in a machine that still
runs and still passes its tests. So instead of skipping silently, the document reports what
it could not read.

---

## Step 4 — Look at what you built

Before building any machine, check that the reading worked:

```sh
ceps ears.ceps.lex door_lock.req --pe
```

```
(STRUCT "incose_transition"
  (STRUCT "req" "REQ-002")
  (STRUCT "tag"  (ID "REQ_002"))
  (STRUCT "system" (ID "controller"))
  (STRUCT "from" (ID "unlocked"))
  (STRUCT "on"   (ID "lock_requested"))
  (STRUCT "to"   (ID "locked"))
  …)
```

This is the model: an ordinary tree, the sort of thing `JSON.parse` would give you.
`STRUCT` is a labelled node, `ID` is a name. **Run this whenever a rule misbehaves** — it
shows you exactly what the grammar understood, which is almost always enough to find the
problem.

---

## Step 5 — Walk the tree and build the machine

Open `extract.ceps`. This is where the model becomes a machine.

### 5.1 Looping over the model

```
static_for(e : root.incose_transition){ … }
```

Read it as `for (const e of root.incose_transition)`. The difference from an ordinary loop
is *when* it runs: it runs while the document is being built, and whatever appears in the
body becomes part of the document. A loop that writes three transitions leaves three
transitions behind.

`root` is the whole tree. `root.incose_transition` selects every node with that label —
one per matching requirement.

### 5.2 Getting values out, and putting names back in

Three functions do all the work:

| | TypeScript equivalent | |
|---|---|---|
| `text(x)` | `String(x)` | the characters of a node |
| `as_identifier("locked")` | — | turns those characters back into the **name** `locked` |
| `as_symbol("lock_requested","Event")` | — | turns them into the **event** `lock_requested` |

The middle one is the one that needs explaining. There is a real difference between the
string `"locked"` and the name `locked`, the same difference as between `obj["locked"]`
and a variable called `locked`. A state machine needs names. Pulling a value out of the
tree gives you text, so you have to convert it back — that is all `as_identifier` is for.

### 5.3 The state set, and why it is built from preconditions only

```
incose_precondition_states{
  root.incose_transition.from.content();
  root.incose_action.from.content();
};

incose_states{
  root.incose_precondition_states.content().sort().unique();
};
```

Collect every `from` slot, sort, drop duplicates. `.sort()` and `.unique()` behave as the
array methods you expect.

Look closely at what is **not** collected: the `to` slots. A state exists only if some
requirement says `While the controller is <state>`. Arriving somewhere does not bring it
into existence.

This is deliberate, and it is the most useful single decision in the example. Because `to`
never creates a state, a requirement that sends the machine somewhere nobody declared
leaves a dangling reference, and the ordinary machine-building step rejects it:

```
***Fatal Error:Statemachine 'controller': frozen is not a state (or statemachine).
```

**No code implements that check.** There is no validator. The error happens because the
state set was built honestly and the normal semantics did the rest.

### 5.4 Building the machine

```
sm{
  as_identifier(text(init.system.content()));

  states{
    Initial;
    static_for(st : root.incose_states.content()){ as_identifier(text(st)); }
  };

  static_for(e : root.incose_transition){
    t{ as_identifier(text(e.from.content()));
       as_identifier(text(e.to.content()));
       as_symbol(text(e.on.content()),"Event");
       as_identifier(text(e.tag.content())); };
  }
};
```

`sm{…}` is a state machine, `states{…}` its states, and `t{from; to; event; action;}` one
transition. These are exactly what you would write by hand — the loop is simply writing
them for you.

The last entry in each `t{}` is the name of an action to run when the transition fires.
That is the hook Step 6 uses.

---

## Step 6 — Put the requirement identifier into the trace

Each requirement gets an action named after it, which prints its identifier:

```
Actions{
  static_for(e : root.incose_transition){
    make_struct(text(e.tag.content()), e.trace.content());
  }
};
```

`make_struct(name, body)` builds a node whose **label is computed** — in TypeScript terms,
an object with a computed key, `{ [name]: body }`. So REQ-002 yields an action called
`REQ_002`, which is the name referenced from the transition in Step 5.4.

**One wart, stated plainly.** The text that action prints was assembled back in
`ears.ceps.lex`, in a `trace{print(…)}` field, rather than here where it belongs. The
reason is a limitation: `print` will not accept an argument computed from the loop
variable. The grammar can build the string because it is pasting together matched words,
and the result sits inertly in the tree until it is spliced into the action body here.
It works, but the grammar now knows about tracing, which is not its job. `SPEC.md` §7.1
records this as a compromise rather than a design.

---

## Step 7 — Run it

`simulate.ceps` is the stimulus — the sequence of events to feed the machine:

```
Simulation{
 Start{controller;};

 lock_requested;     // REQ-002  unlocked -> locked
 tamper_detected;    // REQ-004  locked   -> alarming   (unwanted behaviour)
 lock_requested;     // REQ-006  alarming, action response, no state change
 unlock_requested;   // REQ-005  alarming -> unlocked
 lock_requested;     // REQ-002  unlocked -> locked
 crash_detected;     // REQ-007  any state -> unlocked
};
```

```sh
./run.sh
```

Reading the trace: `controller.locked+` is an **entry** into `locked`, `controller.locked-`
an **exit**. Entry is printed before exit, so a transition shows as `locked+` then
`unlocked-`. Lines in brackets are the requirements.

You can also see the machine rather than run it:

```sh
ceps ears.ceps.lex door_lock.req extract.ceps --dot_gen --ignore_simulations
dot -Tpng out.dot -o out.png
```

That diagram is worth a moment: nobody drew it, and it cannot disagree with the
requirements, because it was made from them.

---

## Step 8 — Two experiments that should fail

Understanding a tool means knowing what it refuses.

**Experiment 1 — a state nobody declared.** Append to `door_lock.req`:

```
REQ-008
While the controller is locked, when freeze_requested is received,
the controller shall transition to the frozen state.
```

Run it. The run fails with `frozen is not a state`, exit code 1, because no requirement
ever said `While the controller is frozen`. Fix it by adding a requirement that gives
`frozen` behaviour of its own — the error is telling you the specification is incomplete,
which it is.

**Experiment 2 — a sentence outside the patterns.**

```sh
./run.sh --bad
```

```
=== INCOSE conformance report ===
[REQ-101] NON-CONFORMING: matches no accepted pattern, no model element generated
=================================
```

The machine still runs, but it cannot pretend REQ-101 was handled. Open
`nonconforming.req` and you will see why it fails:

```
REQ-101
The system shall not unlock the doors while vehicle speed is greater than 5 km/h
unless crash mode is active.
```

It joins clauses with the combinators *while* and *unless* (against INCOSE rule R19), it
carries two separate thoughts — a speed interlock and a crash override — in one sentence
(R18), and it states a prohibition rather than a behaviour. The tool is not judging its
grammar: it simply matches no pattern, and says so.

---

## Step 9 — Point it at your own document

`ears.ceps.lex` and `extract.ceps` contain no door-lock vocabulary. To use your own
requirements:

1. Write them as `yours.req`, using the five patterns in Appendix B.
2. Make sure exactly one requirement uses the initialisation pattern —
   `The <system> shall initialise in the <state> state.` The subject of that sentence
   names the machine. Without it you get no machine at all.
3. Make sure every state you transition *to* is also a state some requirement
   transitions *from*, or declare behaviour for it.
4. Write a `Simulation{ Start{<system>;}; … }` block with your events.
5. Run `ceps ears.ceps.lex yours.req extract.ceps yours_sim.ceps`.

If a requirement does not come through, run with `--pe` and no transform (Step 4) and look
at what the grammar actually understood.

---

## Appendix A — Things that will bite you

1. **Never use a single-letter name.** `s` means *seconds* and `m` means *metres*; ceps has
   a units system and it will quietly take the letter. A loop variable called `s` evaluates
   to the number 1 with a unit attached, with no error — which is why `extract.ceps` uses
   `st`. This costs people an afternoon. Use names of two letters or more, always.
2. **`enter` and `exit` cannot appear in requirement sentences.** The pattern language
   claims them. Hence `transition to`.
3. **Count punctuation when numbering `$N`.** See §3.2.
4. **The catch-all rule stays last.** See §3.4.
5. **Argument order is load order.** Grammar, then documents, then transforms — see
   "The shape of the whole thing" above, where this is the design rather than the trap.
6. **A transition with only two fields has no slot for an action.** `t{from; to;}` is the
   initialisation shape, which is why REQ-001 produces no trace line.

## Appendix B — The five accepted patterns

```
REQ-nnn
The <system> shall initialise in the <state> state.                       (initialisation)

REQ-nnn
While the <system> is <state>, when <trigger> is received,
the <system> shall transition to the <state> state.                       (transition)

REQ-nnn
While the <system> is <state>, if <trigger> is received,
then the <system> shall transition to the <state> state.           (unwanted behaviour)

REQ-nnn
While the <system> is <state>, when <trigger> is received,
the <system> shall <verb> the <object>.                     (action; state unchanged)

REQ-nnn
When <trigger> is received,
the <system> shall transition to the <state> state.      (applies in every state)
```

The action pattern names its action `<verb>_<object>`, so *shall illuminate the
hazard_lamp* becomes `illuminate_hazard_lamp`.

---

## Where to go next

- `SPEC.md` — the specification. §4 gives the extraction rules precisely, §9 the acceptance
  criteria you can re-run, §10 what this deliberately does not do.
- `README.md` — the short version, and an argument for why this is worth your attention.
- `INVENTORY.md` §4.1 — the other grammars in this repository, reading Gherkin feature
  files and CAN databases. Same machinery, different notations.
