# Recursive State Machines: the `c{}` Construct

**Status:** design specification, not yet implemented.
**Scope:** `core/` — parser, build phase, simulation loop, trace, coverage. `cppgen` is addressed in §15 but not specified here.

---

## 1. Summary

This document specifies `c{}` (aliases `call`, `Call`), a new transition kind that
transfers control to a state machine, suspends the caller, and resumes the caller
when the callee reaches `Final`.

With `c{}`, a ceps state machine becomes a **Recursive State Machine** in the sense
of Alur et al. [1]: the set of execution traces moves from regular to context-free,
and — because calls and returns are syntactically distinct — specifically to the
**visibly pushdown** subclass [4]. That subclass is closed under intersection and
complement and has a decidable inclusion problem, which is what makes the construct
analysable rather than merely expressive.

The construct is deliberately minimal:

- exactly one activation stack,
- blocking calls,
- no parameters, no return values,
- no concurrency.

Everything excluded is listed in §14 with the reason.

---

## 2. Motivation

### 2.1 ceps already needs this and already has it, in the wrong place

`import-scripts/dbc/dbc.ceps.lex` contains ten `lexer` blocks wired together with a
literal `call` keyword and `rewind exit` as the return:

```
lexer read_signal {
   ...                call read_signal_signedness   endl
   blank              call read_signal_factor_offset endl
   blank              call read_signal_min_max      endl
   blank receiver{ /read_id_list_end=0;/ call read_id_list };.
 else => rewind exit .
}
```

That is recursive descent with a call/return discipline, implemented in a separate
sublanguage because the state-machine layer could not express it. `c{}` promotes the
same primitive one level up.

### 2.2 What becomes expressible

| Capability | Without `c{}` | With `c{}` |
|---|---|---|
| Nested structure (balanced delimiters, nested document sections) | impossible — a flat SM is a finite automaton | direct |
| Subroutine reuse | duplicate the states at each use site | one definition, *n* call sites |
| Protocol layering (ISO-TP under UDS; nested diagnostic sessions) | flattened by hand into mode variables | one machine per layer |
| Dynamic nesting depth | not representable | bounded only by the frame arena |
| Context-sensitive analysis ("reachable *when called from here*") | not expressible | CFL-reachability / IFDS [6] |

### 2.3 What stays impossible

`c{}` is a *call*, not a *spawn*. The caller blocks. Modelling entities that outlive
their parent (a framework contract spawning call-offs; one line of control per
incoming message) requires dynamic process creation, which is explicitly out of
scope — see §14.4.

---

## 3. Grammar

```
call-transition ::= ("c" | "call" | "Call") "{" from ";" callee ";" ret ";" tail "}" ";"

from    ::= qualified-id        -- a state of the enclosing machine
callee  ::= qualified-id        -- a state machine
ret     ::= qualified-id        -- a state of the enclosing machine
tail    ::= { guard | event | action }   -- as for t{}, in any order, possibly empty
```

The first three arguments are **positional and mandatory**. The tail is classified by
AST node kind, reusing the existing logic in `core/src/sm_sim_process_sm.cpp:75-116`
without modification:

| Node kind | Meaning |
|---|---|
| `symbol` of kind `Guard` | named guard |
| `symbol` of kind `Event` | named event |
| `binary_operator`, `unary_operator`, `int_literal`, `string_literal`, `float_literal`, `func_call` | anonymous inline guard, named by `gen_guard_id` |
| `identifier` | looked up in `events()` then `actions()`; error if neither |

### 3.1 Why `ret` is mandatory

Two independent reasons.

**Grammatical.** With the tail classified by kind, a bare identifier in position 2
would be ambiguous between "return state" and "action or event name". Making `ret`
positional removes the ambiguity without a lookahead rule.

**Semantic.** `c{from; callee;}` does not say where control resumes. In a machine with
several call sites to the same callee this is not recoverable from context.

### 3.2 Each `c{}` is a box

In RSM terminology [1] a *box* is an occurrence of a machine inside another machine.
Here, every `c{}` transition is exactly one box. Consequences:

- the number of boxes is known at build time and equals the number of `c{}` nodes,
- two call sites to the same callee are distinct boxes, which is what per-box
  coverage (§10.2) needs,
- no new declaration syntax is required in `states{}`.

### 3.3 Example

```
sm{quicksort;
  states{Initial; Part; LDone; RDone;};

  t{Initial; Part;  n >  1;};
  t{Initial; Final; n <= 1;};

  t{Part;  Part;   PARTITIONED;};   -- ordinary work
  c{Part;  quicksort; LDone;};      -- call, resume at LDone
  c{LDone; quicksort; RDone;};      -- call, resume at RDone
  t{RDone; Final;};
};
```

---

## 4. Name resolution

`t{}` resolves both endpoints through `current_statemachine->lookup()`
(`core/src/sm_sim_process_sm.cpp:58-62`), i.e. in the **state namespace of the
enclosing machine**. A recursive callee is not a state of itself, so a recursive
`t{L; quicksort;}` cannot resolve under any spelling. This is an independent reason
why `c{}` must be a distinct node kind rather than an overload of `t{}`.

Resolution rules for `c{}`:

| Argument | Namespace | Failure diagnostic |
|---|---|---|
| `from` | `current_statemachine->lookup()` | `Statemachine '<id>': '<from>' is not a state.` |
| `callee` | dot-separated path, resolved relative to the enclosing machine, then from the root | `Statemachine '<id>': '<callee>' is not a state machine.` |
| `ret` | `current_statemachine->lookup()` | `Statemachine '<id>': '<ret>' is not a state.` |

`callee` resolving to the enclosing machine itself is legal and is the direct-recursion
case.

An id is a dot-separated path, resolved first relative to the enclosing machine and then
from the root. All of these name the same machine from inside `A`:

```
sm{A; states{Initial;}; sm{B; states{Initial;};}; t{Initial;B;};   };   // bare/relative
sm{A; states{Initial;}; sm{B; states{Initial;};}; t{Initial;A.B;}; };   // root-qualified
```

`c{}` must use exactly this rule, unchanged. The bare form is the one that matters here:
a directly recursive `c{}` names its own enclosing machine, and the relative path for
that is the bare name.

`SKILL.md:114` gives only the qualified form and `SKILL.md:116` states a scoping rule
strict enough to contradict it; see `DEFECTS.md` D10. The resolution behaviour itself is
correct.

---

## 5. Static checks

Performed during the build phase, after the index assignment of
`core/src/state_machine_simulation_core_buildsms.cpp:250-270`.

### 5.1 Errors

| # | Condition | Message |
|---|---|---|
| E1 | `from` or `ret` resolves to a state machine rather than a plain state | `'<id>' is a state machine; c{} requires plain states for source and resumption.` |
| E2 | `c{}` occurs inside a machine carrying `THREAD` or `IN_THREAD` | `c{} is not permitted inside a thread region (would require more than one activation stack).` |
| E3 | the callee has no path from its `Initial` to a `Final` | `State machine '<callee>' has no reachable Final state and can never return.` |
| E4 | the callee's subtree is not index-contiguous (§6.1) | `State machine '<callee>' does not occupy a contiguous index range; mixing cover and non-cover machines inside a callee is unsupported.` |

E2 is what keeps the model single-stack. Two orthogonal regions of one machine each
issuing a call would produce two concurrent activation stacks over shared system
state, which is undecidable [7].

E3 is cheap (reachability on the callee's own transition relation) and catches the
most common authoring mistake.

### 5.2 Warnings

| # | Condition | Message |
|---|---|---|
| W1 | a `c{}` whose `ret` has no outgoing transition and is not `Final` | `Resumption state '<ret>' has no outgoing transition.` |
| W2 | a callee reachable from no `c{}` and from no `Start{}` | `State machine '<callee>' is never entered.` |
| W3 | mutual recursion with no guard on any edge of the cycle | `Recursive cycle <A> -> <B> -> <A> has no guarded edge; depth is bounded only by --max_recursion_depth.` |

---

## 6. Runtime model

### 6.1 Index layout and the interval invariant

`compute_state_ids_fn` (`core/src/state_machine_simulation_core_buildsms.cpp:250-270`)
assigns a machine's own index, then the indices of its direct states in `order_`
order, before `traverse_sm` (`:126`, DFS pre-order) descends into nested machines.

```
[M][M.s1][M.s2]...[M.sn][ subtree of first nested sm ][ subtree of next ] ...
```

**Interval invariant.** Every machine `M` occupies a half-open index range
`[M.idx_, M.idx_ + width(M))`, where `width(M)` is the number of indices in `M`'s
subtree. `width` is known at build time.

**Caveat — the invariant is per cover class.** The numbering runs `traverse_sms`
twice, with `non_cover_sm` true and then false:

```cpp
if (cur_sm->cover() && non_cover_sm) return;
if (!cur_sm->cover() && !non_cover_sm) return;
```

A covering machine nested inside a non-covering one is skipped in pass 1 and numbered
in pass 2, at the tail. Such a subtree is therefore **not** contiguous. Check E4
detects this. Two acceptable resolutions:

- reject (E4), which is what this specification requires for v1;
- renumber so that a callee's subtree is contiguous regardless of cover class, which
  is a larger change to the build phase and would invalidate
  `start_of_covering_states` as a single boundary.

**Sharing.** `traverse_sm` (`:126`) deduplicates through an `unordered_set`, so a machine
referenced from two places is numbered once. For recursion this is exactly right:
the callee *is* the same module, re-entered at a different depth, reusing the same
index range. For a genuinely shared submachine it means the second reference points
outside its own parent's interval; E4 catches the cases where this matters.

### 6.2 The frame

A frame is the saved content of the callee's index range:

```c
frame[d][0 .. width(callee)-1]  ==  current_states[callee.idx_ .. callee.idx_+width-1]
```

`current_states` is `std::vector<std::uint8_t>` (`core/include/sm_execution_ctxt.hpp:39,234`),
so a frame is a byte slice and push/pop are `memcpy`.

Alongside the slice, the activation record stores:

```c
struct activation_record {
    int ret;        // resumption state index in the caller
    int callee;     // callee machine index
    int lo;         // == callee, cached
    int width;      // cached, avoids a lookup on pop
};
```

### 6.3 The activation stack

One arena-allocated block, sized at startup:

```
frames        : uint8_t[max_depth][max_width]      // max_width = max over all callees
records       : activation_record[max_depth]
depth         : int
```

`max_depth` is a command-line option, `--max_recursion_depth`, default 256. No
allocation occurs during simulation.

### 6.4 The `SUSPENDED` flag

Add to the `inf` bitfield in `core/include/sm_execution_ctxt.hpp` (currently
`INIT=0` … `HAS_LABEL=16`):

```c
static constexpr unsigned int SUSPENDED = 17;
```

**Governing rule.**

> A state that is marked `SUSPENDED` is *present* but *not yet entered*. It is
> present for the purposes of `empty()`, hierarchy bookkeeping and the active-state
> display. It is not entered for the purposes of `on_enter{}`, outgoing-transition
> enabling and `visit_state`. Entering happens when `SUSPENDED` is cleared.

This single rule resolves four problems that would otherwise need separate handling:

1. **The configuration hole.** Between push and pop the caller has left `from` and has
   not reached `ret`. `empty()` (`core/include/sm_execution_ctxt.hpp:167`) declares a
   machine finished when no child is active, so the caller would be torn down
   mid-call. Setting `current_states[ret] = 1` at push time closes the hole.
2. **The race.** `t{ret; ...}` must not fire while the call is pending. A suspended
   state has no enabled outgoing transitions.
3. **The return handler.** `on_enter{}` of `ret` fires when `SUSPENDED` clears, i.e.
   on return — which is where a return handler belongs. No new syntax is needed.
4. **Coverage timing.** `visit_state(ret)` happens on return, not on call, so the
   counter means "resumed here", not "attempted a call from here".

---

## 7. Operational semantics

### 7.1 The frozen set

While `depth > 0`, the simulation steps only states inside the **top frame's
interval** `[records[depth-1].lo, +width)`, plus the *ancestors* of the calling
machine (§7.6). Everything else is frozen.

This is the decision that makes the model decidable. The alternative — leaving the
caller's other active states live — produces a tree of concurrent activations rather
than a stack, which is the undecidable multi-stack case [7].

Implementation: bound the scan in the simulation loop to the active interval instead
of `[start_of_covering_states, number_of_states)`.

### 7.2 Enabling

A `c{}` is enabled under exactly the conditions of a `t{}` with the same tail:
`current_states[from] == 1`, `from` not `SUSPENDED`, guard true, event matching.
**The guard is evaluated in the caller's frame, before the push.**

### 7.3 Push

Executed in `compute_transition_kernel`
(`core/src/sm_sim_core_simulation_loop.cpp:358`), the single point where transitions
are applied.

```
 1. if depth == max_depth -> fatal, see D1
 2. current_states[from] = 0
 3. current_states[ret]  = 1 ; set_inf(ret, SUSPENDED, true)
 4. memcpy(frames[depth], &current_states[callee.lo], callee.width)
 5. memset(&current_states[callee.lo], 0, callee.width)
 6. current_states[callee]                 = 1
    current_states[initial_state[callee]]  = 1
 7. records[depth] = { ret, callee, lo, width } ; ++depth
 8. run the transition's actions (caller side)
 9. visit_transition(call edge)
    visit_state(callee) ; visit_state(initial_state[callee])
    visit_call_site(box)
10. emit the depth-tagged trace records of §9.2: `from-`, `ret~`, and the
    callee entries at `@depth`
11. run on_exit{} of from, on_enter{} of callee and its Initial
12. end the microstep  (§7.5)
```

Step 11 uses the ordinary diff machinery (`compute_entered_states` /
`compute_exit_states`, `core/src/sm_sim_core_simulation_loop.cpp:500,556`) — the push
is a genuine entry and needs no special treatment.

### 7.4 Pop

Triggered when `current_states[Final]` of the top frame's callee becomes 1.

```
 1. run on_exit{} down the callee's active configuration (ordinary exit semantics)
 2. --depth ; let r = records[depth]
 3. memcpy(&current_states[r.lo], frames[depth], r.width)
       -- EXCLUDED from the enter/exit diff: the restored states were never left
 4. clear_inf(r.ret, SUSPENDED)
       -- this is the entry of r.ret: run on_enter{} of r.ret, visit_state(r.ret)
 5. visit_transition(return edge) ; visit_return(box)
 6. emit the depth-tagged trace records of §9.2: the callee's exits at `@depth`,
    then `ret+` at the caller's depth
 7. end the microstep  (§7.5)
```

**Step 3 is the one place where a naive implementation introduces two bugs at once.**
If the restore participates in the diff, `compute_entered_states` sees the caller's
previously active states appear and fires `on_enter{}` for states that never left.
If the restore is suppressed wholesale, `visit_state` — which is called from inside
`compute_entered_states` (`:516`, `temp[state] = 1;execution_ctxt.visit_state(state);`)
— stops counting. The suppression must apply to the **actions and the trace**, never
to the coverage counters.

### 7.5 Step boundaries

Push and pop each terminate the current microstep. They are **not** part of the
epsilon-closure fixpoint described at
`core/src/sm_sim_core_simulation_loop.cpp:822`.

Rationale: if they were, an entire recursion would complete inside a single macrostep.
No event could interleave, no intermediate configuration would be observable, and a
runaway recursion would diverge inside the fixpoint rather than hitting the depth cap
cleanly.

### 7.6 `Final`

`Final` is context-dependent:

| Condition | Behaviour |
|---|---|
| `depth > 0` and `records[depth-1].callee` is this machine | return (§7.4) |
| otherwise | terminate the machine, as today |

### 7.7 Ancestor preemption

Transitions whose source is an **ancestor** of the calling machine remain enabled
during a call. Without this, a supervisory edge such as `t{Running; Aborted; ABORT;}`
on an enclosing machine would be dead for the entire duration of a recursion.

When such a transition fires, every frame whose callee interval lies inside the
exited subtree is discarded:

```
while (depth > 0 && records[depth-1].lo >= exited.lo
                 && records[depth-1].lo <  exited.lo + exited.width)
    --depth;              // frames are dropped, not restored
```

Because subtrees are index intervals (§6.1), the containment test is an integer
comparison. The exit actions of the discarded machines run through the ordinary
`remove_children` path (`core/include/sm_execution_ctxt.hpp:157`).

Each dropped frame is marked with the `!` sigil (§9.5) so that the trace
remains a well-matched nested word under the extended alphabet.

---

## 8. Interaction with existing machinery

| Mechanism | Location | Effect of `c{}` |
|---|---|---|
| `process_transition` | `sm_sim_core_simulation_loop.cpp:383` | unchanged for `t{}`; `c{}` dispatches to push |
| epsilon fixpoint | `:822` | push/pop are boundaries, not fixpoint steps |
| `restart_event` path | `:993-1007` | structurally identical to a push without the save; reuse |
| `compute_entered_states` | `:500` | unchanged on push; excluded on pop except for `visit_state` |
| `empty()` | `sm_execution_ctxt.hpp:167` | unchanged — `SUSPENDED` keeps the caller non-empty |
| `remove_children` | `sm_execution_ctxt.hpp:157` | unchanged; used by ancestor preemption |
| `shadow_state` / `shadow_transitions` | `:402-416` | orthogonal; shadow chains live at disjoint indices |
| threads / regions (`THREAD`, `JOIN`, `REGION`) | flags 7-10 | `c{}` forbidden inside, check E2 |
| `VISITED` (flag 5) | `sm_execution_ctxt.hpp:22` | sticky history, **not** stacked — see §10.1 |
| `current_states` | 12 files | unchanged shape: still one flat vector |
| `start_of_covering_states` | `buildsms.cpp:270` | unchanged; constrains callees via E4 |

Note the last-but-one row. `c{}` does not change the shape of `current_states`; it
adds a stack beside it. This is the reason `c{}` is a tractable change and dynamic
process creation is not.

---

## 9. Execution trace

### 9.1 What exists today

`log_triggered_transitions` (`core/src/sm_sim_core_simulation_loop.cpp:245`) begins
with a bare `return;` on line 248 and is dead code. The actual trace comes from
`log_state_changes` (`:481`), a delta of the flat state set:

```cpp
for(size_t z = 0; z != current_states.size(); ++z){
   if (current_states[z] == temp[z]) continue;
   ss << idx_to_state_id[z];
   if (temp[z]) ss << "+ "; else ss << "- ";
}
```

It is called once per step (`:867`, `:1022`) and `info` newline-terminates by default
(`core/include/state_machine_simulation_core.hpp:306`), so **one line is one
microstep**. That separator is already correct and needs no change.

What the format cannot do is represent depth. The deltas are index-based and indices
repeat at every depth, so under `c{}` a push and a pop are indistinguishable from a
machine restart and the nesting is absent.

### 9.2 The format: depth-tagged state deltas

The change is a suffix on the state name, not a new record kind.

```
<qualified-state-id> [ "@" <depth> ] <sigil>
```

| Sigil | Meaning |
|---|---|
| `+` | entered |
| `-` | exited |
| `~` | suspended: present, not entered, a call is pending here |

**`@0` is omitted.** A model containing no `c{}` therefore produces a byte-identical
trace to the current implementation, which is acceptance criterion A12 satisfied by
construction rather than by a special case.

A push and a pop look like this:

```
quicksort.Part-  quicksort.LDone~  quicksort@1+  quicksort.Initial@1+
...
quicksort.Initial@1-  quicksort@1-  quicksort.LDone+
```

### 9.3 Why this is sufficient

**The depth tag logs the logical configuration, not the array.** On a push the
callee's slice is saved and zeroed, so a diff of the *physical* array reports every
saved state as exited. Logically those states never left — they are in a frame, still
active, merely not on top. Tagging by depth means the trace records the union over
frames, and the saved states correctly produce no record at all.

This is why no burst suppression is needed in the *format*. It is still needed in the
*implementation*: `log_state_changes` diffs `temp` against `current_states`, both
physical, so the save and the restore must be excluded from the diff (§7.4, step 3).
Tagging the output does not change what is being diffed.

**The depth sequence is a Dyck word**, so the call/return/internal partition required
for the nested-word reading [4] is *derivable* rather than declared:

| Relation to previous record | Classification |
|---|---|
| depth increased | call |
| depth decreased | return |
| depth unchanged | internal |

One record of lookahead, no unbounded context. Separate `call{}` / `ret{}` records
would be a redundant encoding of information the depth tag already carries.

**The box is identified by the `~` record.** `quicksort.LDone~` names the resumption
state, and a resumption state belongs to exactly one `c{}`. Without `~` the box would
be recoverable only from the preceding `-` record, which is ambiguous when two `c{}`
share a source state, and not recoverable at all if the run ends mid-call. One record
per call closes the gap.

### 9.4 Properties the format must have

1. **Well-matched.** The depth sequence is a balanced Dyck word; every increase is
   eventually matched by a decrease, except for frames live at termination.
2. **Nested word.** With the partition of §9.3 the trace is a nested word in the sense
   of [4]. This is the property that makes VPA-expressed requirements checkable
   against a recorded trace.
3. **Replayable.** The trace plus the model determines the configuration at every
   point, including the depth and the content of every frame.
4. **Non-recursive models unaffected.** See A12.

### 9.5 Preemption

Ancestor preemption (§7.7) discarding *k* frames produces *k* depth decreases, which
is indistinguishable from *k* ordinary returns. The word remains well-matched and
analysis is unaffected, but a reader cannot tell an abort from *k* completions.

A discarded frame is therefore marked by exiting its states with `!` rather than `-`:

```
quicksort.Part@3!  quicksort@3!  quicksort.LDone@2!  quicksort@2!  Main.Aborted+
```

`!` classifies as a return for the Dyck reading. It carries no information the
analysis needs and all the information a reader needs.

### 9.6 Two pre-existing defects in the trace

Both are independent of `c{}` but constrain acceptance criterion A12, which requires a
byte-identical trace for non-recursive models. A12 must be read against *corrected*
behaviour, not against the current output.

**The initial entry is logged only for covering models.** The activation of the
`Start{}` machines happens in a block gated on `start_of_covering_states_valid()`
(`core/src/sm_sim_core_simulation_loop.cpp:852-869`). For a model with no `cover{}`
and no `concept`, the block is skipped and the first line of the trace is lost:

```
$ ceps nest.ceps                     # sm{A; states{Initial;}; sm{B; ...}; t{Initial;B;};};
A.Initial- A.B+ A.B.Initial+         # "A+ A.Initial+" is missing

$ ceps cov.ceps                      # the same model with `concept`
A+ A.Initial+
A.B+ A.B.Initial+ A.Initial-
```

**A trailing line duplicates the final exits.** With a covering machine, the last
real step is followed by a spurious line repeating its `-` records. Minimal
reproducer, flat and without nesting:

```
sm{A; concept; states{Initial;X;}; t{Initial;X;}; };
Simulation{Start{A;};};
```
```
A+ A.Initial+
A.Initial- A.X+
A.Initial-            <-- spurious
```

`do_exit_impl` does not log, so this is not the post-loop exit path; it is a further
iteration of the main loop re-reporting a diff that has already been reported.

### 9.7 The live log

`log_current_states` (`:262`) writes `current_states` wholesale to the live logger.
Under recursion that is the physical array, i.e. the top frame only — the caller
appears to have vanished. The binary frame format must carry the depth, and for the
monitor view, the frame stack.

---

## 10. Coverage

### 10.1 What survives unchanged

`visit_state` is a counter, not a flag:

```cpp
void visit_state(size_t idx){
    if (idx < start_of_covering_states) return;
    ++coverage_state_table[idx - start_of_covering_states];
}
```
— `core/include/sm_execution_ctxt.hpp:287-290`

Recursion simply increments it more often. Nothing breaks.

`VISITED` (flag 5) is sticky history: `remove_children` sets it to `true` while
zeroing the state. It must **not** be stacked. Leaving it unstacked is precisely the
desired behaviour — the callee's history survives the pop.

### 10.2 What degrades, and the fix

The report computes

```cpp
number_of_states_covered += ctx.coverage_state_table[i] != 0;
```
— `core/src/state_machine_simulation_core.cpp:2034`

A state counts as covered if it was reached **once, anywhere**. Exercising a callee
from one call site therefore marks its states covered for every call site, including
ones never taken. The metric becomes least informative exactly where the model became
most expressive.

**Fix: context-sensitive coverage at depth k = 1, keyed by box.** Each `c{}` is a box
(§3.2) with a build-time ordinal, and each callee's subtree is an index interval of
known width (§6.1), so the table is statically sized:

```c
coverage_box_state_table       : int[n_boxes][max_width]
coverage_box_transition_table  : int[n_boxes][max_trans_width]
```

k = 1 is the deliberate choice: k = 2 multiplies the table by the call-graph
branching factor, and unbounded context is infinite under recursion.

### 10.3 The filter bug

Transition coverage currently skips an edge if either endpoint carries `INIT`,
`FINAL`, `DONT_COVER` **or `SM`**:

```cpp
if (ctx.get_inf(to_state,DONT_COVER) || ctx.get_inf(to_state,INIT) ||
    ctx.get_inf(to_state,FINAL)      || ctx.get_inf(to_state,SM) ) continue;
```
— `core/src/state_machine_simulation_core.cpp:2072-2082`

A call edge targets a machine, so `is_sm(to)` holds and the edge is excluded. A return
edge originates at `Final` and is excluded too. **Under the current filter, calls and
returns would be the only edges in a recursive model that are silently not counted.**

The `SM` exclusion exists because compound-state entry edges are bookkeeping. A call
edge is not bookkeeping. Required change: exempt transitions of kind `c{}` from the
`SM` and `FINAL` exclusions.

### 10.4 The denominator

`number_of_states_to_cover` is computed by counting table entries
(`:2035`). Under context-sensitive coverage the denominator becomes the number of
**reachable** `(box, state)` pairs, which is emphatically not `n_boxes × width` —
most boxes cannot reach most states. Without a call-graph reachability pass the ratio
sits permanently below 100 % and carries no information.

One call-graph reachability pass answers both this and the related question of whether
a callee reachable only from an unreachable box belongs in the denominator. It is the
same pass that implements check E3.

### 10.5 New criteria

| Criterion | Definition | Cost |
|---|---|---|
| Call-site coverage | every `c{}` taken at least once | one counter per box |
| Return coverage | every box observed a return | one counter per box |
| Depth profile | maximum depth reached, per callee | one `max` per push |

Return coverage is the runtime counterpart to check E3. The depth profile is how
`--max_recursion_depth` gets sized from evidence rather than guessed.

---

## 11. Diagnostics

| # | Trigger | Message |
|---|---|---|
| D1 | `depth == max_depth` at a push | `***Fatal Error: recursion depth 256 exceeded at c{Part; quicksort; LDone;}. Call chain: quicksort -> quicksort -> ... (--max_recursion_depth to raise).` |
| D2 | `Final` reached with `depth > 0` but the top frame's callee is a different machine | `***Internal Error: return from '<x>' but top activation record is '<y>'.` (indicates a bug, not a user error) |
| D3 | pop with `depth == 0` | `***Internal Error: return with empty activation stack.` |

D1 must print the call chain. A depth-exceeded message without the chain is close to
useless for a model the author did not expect to recurse.

---

## 12. Worked example

### 12.1 Model

```
sm{Count;
  states{Initial; Down; Back;};
  t{Initial; Final; n <= 0;};
  t{Initial; Down;  n >  0;};
  c{Down;    Count; Back;};
  t{Back;    Final;};
};

Simulation{ Start{Count;}; };
```

with `n` decremented in `on_enter{}` of `Down` and initialised to 2.

### 12.2 Trace

One line per microstep, as today. `@0` is omitted.

```
depth  trace line
-----  --------------------------------------------------------------
  0    Count+ Count.Initial+
  0    Count.Initial- Count.Down+                              n := 1
  1    Count.Down- Count.Back~ Count@1+ Count.Initial@1+
  1    Count.Initial@1- Count.Down@1+                          n := 0
  2    Count.Down@1- Count.Back@1~ Count@2+ Count.Initial@2+
  2    Count.Initial@2- Count.Final@2+
  1    Count.Final@2- Count@2- Count.Back@1+
  1    Count.Back@1- Count.Final@1+
  0    Count.Final@1- Count@1- Count.Back+
  0    Count.Back- Count.Final+
```

### 12.3 Observations

- The depth sequence `0 0 1 1 2 2 1 1 0 0` is a Dyck word. Increases are calls,
  decreases are returns, no separate record kind is required.
- **The saved frames emit nothing.** During the call at depth 1, `Count.Back` and
  `Count` at depth 0 are still logically active and correctly produce no records.
  There is no burst to suppress.
- `Count.Back~` is the push. `Count.Back` is *present* — so `empty(Count)` is false
  and the caller is not torn down — but *not entered*, so `t{Back; Final;}` cannot
  fire and `on_enter{Back}` has not run. The record also names the box: a resumption
  state belongs to exactly one `c{}`.
- `Count.Back+` is the return. `SUSPENDED` clears, `on_enter{Back}` runs for the
  first time, and `t{Back; Final;}` becomes enabled in the *next* microstep.
- `Count.Down` occurs at depth 0 and depth 1 and is distinguishable in the trace.
  The flat coverage counter cannot distinguish them; the per-box table can.
- The first two and last two lines carry no `@` and are byte-identical to what the
  current implementation would emit for a non-recursive model (A12).

## 13. Acceptance criteria

Each is a falsifiable test, not a review item.

| # | Criterion |
|---|---|
| A1 | §12.1 run with `n = k` produces exactly `k` `call` and `k` `ret` records, balanced, with depths `1..k` and back. |
| A2 | `on_enter{}` of the resumption state fires **exactly once per return**. A counter incremented in `on_enter{Back}` reads exactly `k` after the run. |
| A3 | No `on_enter{}` of a *restored* caller state fires on a pop. A counter in `on_enter{Down}` reads exactly `k`, not `2k`. |
| A4 | State and transition coverage counters for the callee are **non-zero** after a run, i.e. the suppression of §7.4 step 3 did not disable `visit_state`. |
| A5 | The call edge and the return edge appear in the transition-coverage list (§10.3 regression test). |
| A6 | `n = max_depth + 1` produces diagnostic D1 with a non-empty call chain and exit code 1 — not a segfault, not a `max_number_of_active_transitions` error. |
| A7 | A model with a callee that cannot reach `Final` is rejected at build time by E3 with exit code 1. |
| A8 | A `c{}` inside a thread region is rejected by E2. |
| A9 | During a call, a transition on an ancestor machine triggered by an event still fires, and the frames below are discarded, each marked with `!` (§9.5). |
| A10 | While `depth > 0`, no transition outside the top frame's interval and outside the ancestor chain fires. Test: a sibling state of the caller with an enabled epsilon transition does not move. |
| A11 | Replaying a recorded trace against the model reconstructs the identical sequence of configurations **including depth**. |
| A12 | A model with no `c{}` produces a byte-identical trace and byte-identical coverage figures to the current implementation, **after the two defects of §9.6 are fixed**. |

A12 is the regression gate: the feature must be invisible when unused.

---

## 14. Out of scope

### 14.1 Parameters and return values

v1 passes nothing. System state remains global, which means **recursion is correct for
control flow and wrong for data**: a recursive call clobbers the caller's variables.
The `n` in §12.1 works only because it is a counter that is *meant* to be shared.

This is a genuine hole and must be documented in user-facing material, not left to be
discovered. The natural v2 form is copy-in/copy-out on the call:

```
c{Part; quicksort; LDone; in{ n = n - 1; }; };
```

### 14.2 Non-local exit

`Final` returns exactly one level. Unwinding *n* frames to a handler is not provided.
§7.7 covers the practically important case (an ancestor aborting) without a general
mechanism.

### 14.3 Events during a call

The event queue is global and the caller is frozen. An event that only the caller
could consume is consumed and discarded with no effect. UML's deferred-event mechanism
would address this; it is not specified here.

### 14.4 Concurrency

No `fork`, no `spawn`, and `c{}` inside a thread region is an error (E2). Finite state
plus dynamic process creation is a Petri net; **adding recursion to it yields a dynamic
network of pushdown systems, which is undecidable when the lines synchronise** [8] —
and ceps lines synchronise through system state and the shared event queue. The two
features are individually tractable and jointly lethal. If dynamic creation is ever
added, this interaction must be resolved explicitly, not discovered.

### 14.5 Supervisory transitions

Outside the ancestor chain of §7.7, everything is frozen during a call. A monitor
machine that is a *sibling* rather than an ancestor will not run. Authors who need
continuous supervision during a recursion must place the supervisor on an ancestor.

---

## 15. Implementation plan

Ordered so that each step is independently testable.

| Step | Work | Files | Approx. |
|---|---|---|---|
| 1 | `SUSPENDED` flag; scan loop and `empty()` honour it | `sm_execution_ctxt.hpp`, `sm_sim_core_simulation_loop.cpp` | 40 |
| 2 | Parse `c` / `call` / `Call`; three positional args; reuse the tail loop | `sm_sim_process_sm.cpp` | 90 |
| 3 | Build-phase: box ordinals, `width()` per callee, interval-contiguity check E4 | `state_machine_simulation_core_buildsms.cpp` | 120 |
| 4 | Frame arena, `activation_record`, `--max_recursion_depth`, diagnostic D1 | `sm_execution_ctxt.hpp`, `cmdline_utils.{hpp,cpp}` | 110 |
| 5 | Push and pop in `compute_transition_kernel`; diff exclusion on pop | `sm_sim_core_simulation_loop.cpp` | 180 |
| 6 | Bound the scan to the top frame's interval | `sm_sim_core_simulation_loop.cpp` | 30 |
| 7 | Static checks E1-E3, W1-W3; call-graph reachability pass | `state_machine_simulation_core_buildsms.cpp` | 160 |
| 8 | Depth-tagged trace (`@d`, `~`, `!`); exclude save/restore from the diff | `sm_sim_core_simulation_loop.cpp`, livelog | 130 |
| 9 | Coverage: filter fix (§10.3), per-box tables, denominator via reachability | `state_machine_simulation_core.cpp` | 200 |
| 10 | Ancestor preemption and `!` records | `sm_sim_core_simulation_loop.cpp` | 80 |
| 11 | `--dot_gen`: render boxes distinctly from compound states | `dot_gen` | 60 |
| 12 | Acceptance tests A1-A12 | `examples/`, test harness | — |

Steps 1-6 produce a working recursive simulator. Step 9 is the one with a trap in it
(§7.4 step 3 interacts with §10.2); do not merge it before A3 and A4 both pass.

`cppgen` is a separate exercise. A callee becomes a C++ function and the native stack
supplies the activation records, so control flow comes nearly free — but the global
`systemstates` namespace means the data hole of §14.1 appears there too, in exactly
the same shape.

---

## 16. References

1. R. Alur, M. Benedikt, K. Etessami, P. Godefroid, T. Reps, M. Yannakakis.
   *Analysis of Recursive State Machines.* ACM TOPLAS 27(4), 2005.
2. R. Alur, M. Yannakakis. *Model Checking of Hierarchical State Machines.*
   ACM SIGSOFT FSE, 1998.
3. W. A. Woods. *Transition Network Grammars for Natural Language Analysis.*
   CACM 13(10), 1970. — Recursive Transition Networks; the closest notational ancestor.
4. R. Alur, P. Madhusudan. *Visibly Pushdown Languages.* ACM STOC, 2004.
   — the closure properties that motivate a syntactically distinct `c{}`.
5. A. Bouajjani, J. Esparza, O. Maler. *Reachability Analysis of Pushdown Automata:
   Application to Model-Checking.* CONCUR, 1997.
6. T. Reps, S. Horwitz, M. Sagiv. *Precise Interprocedural Dataflow Analysis via
   Graph Reachability.* ACM POPL, 1995. — IFDS; the basis for context-sensitive
   coverage and analysis.
7. G. Ramalingam. *Context-Sensitive Synchronization-Sensitive Analysis is
   Undecidable.* ACM TOPLAS 22(2), 2000. — why §7.1 and check E2 exist.
8. A. Bouajjani, M. Müller-Olm, T. Touili. *Regular Symbolic Analysis of Dynamic
   Networks of Pushdown Systems.* CONCUR, 2005. — why §14.4 excludes dynamic creation.
9. S. Qadeer, J. Rehof. *Context-Bounded Model Checking of Concurrent Software.*
   TACAS, 2005. — the escape hatch if the single-stack restriction ever has to be
   relaxed.
