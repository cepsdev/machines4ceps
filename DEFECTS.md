# Known defects

Defects found while auditing `core/` and `vm/` and while specifying `c{}`
(see `RECURSIVE-STATE-MACHINES.md`). Every entry below was reproduced against
`bin/ceps`, version 0.8.1.3.3 (built Aug 27 2026), on Linux.

Line numbers refer to the working tree at the time of writing.

| # | Severity | Area | Summary |
|---|---|---|---|
| [D1](#d1) | High | VM | `bgt`, `bneq`, `callx` are implemented but not declared, so they cannot be assembled |
| [D2](#d2) | High | Simulation | Crash (SIGSEGV) when a covering machine has no `Initial` state |
| [D3](#d3) | High | Trace | The initial-entry line is logged only for covering models |
| [D4](#d4) | Medium | Trace | A spurious trailing line repeats the final exits in covering models |
| [D5](#d5) | Medium | Coverage | Transition coverage prints `-nan` when the denominator is zero |
| [D6](#d6) | Medium | Coverage | The transition-coverage filter silently drops edges at `SM`/`FINAL` endpoints |
| [D7](#d7) | Low | Trace | `log_triggered_transitions` is disabled by an undocumented `return;` |
| [D8](#d8) | Low | CLI | Three of four report-format flags are parsed but never consumed |
| [D9](#d9) | Low | CLI | `log_verbosity` cannot be set from the command line |
| [D10](#d10) | Low | Docs | `SKILL.md` is incomplete and self-contradictory on submachine references |
| [D11](#d11) | Low | Internals | State-index intervals are not contiguous across cover classes |

---

<a name="d1"></a>
## D1. `bgt`, `bneq` and `callx` are implemented but not declared

**Severity:** High — three working instructions are unreachable.
**Area:** Oblectamenta VM.
**Files:** `core/include/oblectamenta_decls.ceps`, `core/include/vm/vm_base.hpp`.

Opcode declarations are injected from `core/include/oblectamenta_decls.ceps`, which is
`#include`d as a string literal and parsed before user input
(`core/src/state_machine_simulation_core.cpp:175-186`). An opcode absent from that file
is never given the `OblectamentaOpcode` kind, so the assembler sees a plain identifier.

Diffing the assembler's opcode table in `core/include/vm/vm_base.hpp` against the
declaration file yields 26 undeclared names, 23 of which are operand-kind variants
(`ldi32imm`, `pushi64reg`, `haltimm`, …) whose base names *are* declared. Three are
genuinely missing:

- `bgt` — enum `vm_base.hpp:64`, method `:429`, assembler entry `:706`
- `bneq`
- `callx`

### Reproducer

```
kind Event;Event E;
sm{S;states{Initial;};Actions{a{oblectamenta{text{asm{
 bgt(0);
};};};};};t{Initial;Initial;E;a;};};
Simulation{Start{S;};E;};
```

### Observed

The parse tree shows the opcode demoted to an identifier:

```
(FUNC_CALL (ID "bgt") (CALL_PARAMETERS (NUMBER 0)))
```

whereas the declared sibling `blt` yields

```
(FUNC_CALL (SYMBOL "blt" "OblectamentaOpcode") (CALL_PARAMETERS (NUMBER 0)))
```

`SYMBOL … "OblectamentaOpcode"` versus bare `ID` is the whole defect, and it is visible
independently of whether the surrounding program assembles.

### Fix

Add `bgt`, `bneq` and `callx` to `core/include/oblectamenta_decls.ceps`. `bgteq`,
`blteq` and `blt` are already on line 20; `bgt` belongs beside them.

### Note

A regression guard is cheap and worth having: the declaration file and the assembler
table are two lists that must agree, and nothing currently checks that they do. The
`comm`-based diff used to find this can be run in CI.

---

<a name="d2"></a>
## D2. SIGSEGV when a covering machine has no `Initial` state

**Severity:** High — crash, exit code 139.
**Area:** Simulation startup.

### Reproducer

```
sm{Main;concept;states{Idle;Run;};t{Idle;Run;};};
Simulation{Start{Main;};};
```

```
$ ceps crash.ceps ; echo $?
Segmentation fault (core dumped)
139
```

### Trigger

The crash needs **both** conditions. Either alone is harmless:

| Model | Result |
|---|---|
| `concept`, states `Idle;Run;` (no `Initial`) | **SIGSEGV** |
| `concept`, states `Initial;Run;` | exit 0 |
| no `concept`, states `Idle;Run;` | exit 0 |

So it is the covering path specifically that assumes an `Initial` state exists.

### Analysis

The pre-loop block at `core/src/sm_sim_core_simulation_loop.cpp:852-870` runs only for
covering models. It activates top-level covering machines and then calls
`compute_entered_states`, which descends to each machine's initial state. With no
`Initial` present the lookup has no valid result and the index derived from it is used
unguarded.

A diagnostic would be better than a crash here. If a covering machine without `Initial`
is meaningless, reject it at build time with a named error; if it is meaningful, the
descent needs a guard.

---

<a name="d3"></a>
## D3. The initial-entry line is logged only for covering models

**Severity:** High — the execution trace is incomplete, and silently so.
**Area:** Trace / logging.
**File:** `core/src/sm_sim_core_simulation_loop.cpp:852-870`.

The only pre-loop call to `log_state_changes` sits at `:867`, inside a block guarded by

```cpp
if(execution_ctxt.start_of_covering_states_valid()){
```

`start_of_covering_states` is valid only when at least one machine opts in to coverage,
via `cover{}` or `concept` (`core/src/sm_sim_process_sm.cpp:502`). For every other model
the whole block is skipped and the entry of the `Start{}` machines is never logged —
although the machines are plainly entered, since their exits are reported afterwards.

**Trace logging of the initial configuration is accidentally coupled to the coverage
opt-in.** The two features are unrelated.

### Reproducer

```
sm{A; states{Initial;}; sm{B; states{Initial;};}; t{Initial;B;}; };
Simulation{Start{A;};};
```

### Observed

```
A.Initial- A.B+ A.B.Initial+
```

### Expected

```
A+ A.Initial+
A.Initial- A.B+ A.B.Initial+
```

Adding `concept` to `A` makes the missing line appear, confirming the cause:

```
A+ A.Initial+
A.B+ A.B.Initial+ A.Initial-
```

This is not a verbosity setting. `log_verbosity` defaults to `0` and is read only at
`:265` and `:293`; neither path governs this block, and no command-line flag sets it
(see [D9](#d9)).

### Consequence

Any consumer that reconstructs the configuration by replaying the trace starts from the
wrong state. It also affects the acceptance criteria in `RECURSIVE-STATE-MACHINES.md`:
criterion A12 demands a byte-identical trace for models that do not use `c{}`, which
would enshrine the missing line. A12 is therefore stated against corrected behaviour,
and this defect is referenced from §9.6 of that document.

---

<a name="d4"></a>
## D4. A spurious trailing line repeats the final exits

**Severity:** Medium — the trace reports a microstep that did not happen.
**Area:** Trace / logging.
**File:** `core/src/sm_sim_core_simulation_loop.cpp:1022`.

In covering models the last genuine step is followed by an extra line repeating its `-`
records. One line is one microstep (`log_state_changes` emits a single `info()` per step,
and `info` is newline-terminating, `core/include/state_machine_simulation_core.hpp:306`),
so this reports a microstep that never occurred.

### Reproducer

Flat, no nesting, no submachines:

```
sm{A; concept; states{Initial;X;}; t{Initial;X;}; };
Simulation{Start{A;};};
```

### Observed

```
A+ A.Initial+
A.Initial- A.X+
A.Initial-            <-- spurious
```

### Expected

```
A+ A.Initial+
A.Initial- A.X+
```

### Analysis

It is not the post-loop exit path: `do_exit_impl` performs no logging. It is a further
iteration of the main loop re-reporting a diff that has already been reported, which
means `temp` and `current_states` diverge again at that index after the `memcpy` at
`:1031`. The loop body between `:1022` and `:1031` calls
`run_execution_context_loop_cover_state_changed_handlers(temp)`, which runs *after* the
log call and may mutate `temp`; that ordering is the first thing to check with a
breakpoint at `:1022`.

Also present in the nested case, so it is independent of nesting:

```
sm{A; concept; states{Initial;}; sm{B; concept; states{Initial;};}; t{Initial;B;}; };
Simulation{Start{A;};};
```
```
A+ A.Initial+
A.Initial- A.B+ A.B.Initial+
A.Initial-            <-- spurious
```

---

<a name="d5"></a>
## D5. Transition coverage prints `-nan`

**Severity:** Medium — a report line that is not a number.
**Area:** Coverage reporting.
**Files:** `core/src/state_machine_simulation_core.cpp:2066`, `:2114-2115`, `:1843`.

When every transition is excluded by the endpoint filter, the denominator is zero and the
percentage is computed anyway.

### Reproducer

```
sm{A; concept; states{Initial;X;}; t{Initial;X;}; };
Simulation{Start{A;};};
```

### Observed

```
State Coverage: 1 ( 100% )
Transition Coverage: -nan ( -nan% )
```

The model's single transition starts at `Initial`, which carries the `INIT` flag, so the
filter skips it and nothing is left to cover.

### Analysis

There *is* a guard on the printing side — `print_report_coverage` returns early unless
both `valid` flags are set (`:1843`). It does not fire, because the flags do not mean
what the guard assumes:

```cpp
:2066   if( (transition_coverage_defined = ctx.start_of_covering_transitions_valid()) ){
```

`transition_coverage_defined` records that the *covering index range* is valid — not that
anything survived the endpoint filter of [D6](#d6). A model can therefore have a valid
range and still a denominator of zero. The division is then performed unguarded:

```cpp
:2114   state_coverage      = (double)number_of_states_covered      / (double)number_of_states_to_cover;
:2115   transition_coverage = (double)number_of_transitions_covered / (double)number_of_transitions_to_cover;
```

and `-nan` sails straight through the `valid` check.

### Fix

Redefine the flags to mean what the guard needs: `*_defined` should be
`number_of_*_to_cover > 0`, evaluated after the filter loops. That fixes the division and
the printing guard together, and it makes the `valid` field in the structured report
honest for machine consumers.

`-nan` also makes the report unparseable for any downstream tool, which matters more than
the cosmetics — see [D8](#d8).

---

<a name="d6"></a>
## D6. The transition-coverage filter drops edges at `SM` and `FINAL` endpoints

**Severity:** Medium — coverage is silently overstated.
**Area:** Coverage.
**File:** `core/src/state_machine_simulation_core.cpp:2072-2082`.

A transition is excluded from the denominator if *either* endpoint carries
`DONT_COVER`, `INIT`, `FINAL` or `SM`:

```cpp
if ( ctx.get_inf(from_state,...DONT_COVER) ||
     ctx.get_inf(from_state,...INIT)       ||
     ctx.get_inf(from_state,...FINAL)      ||
     ctx.get_inf(from_state,...SM) ) continue;
```

and symmetrically for `to_state`.

Two things are worth separating here.

**The inconsistency.** The corresponding `SM` test in the *state*-coverage filter at
`:2033` is commented out, while the one in the transition filter is live. Whatever the
intent, the two filters disagree about whether submachine states count.

**The consequence for recursion.** Excluding `SM` endpoints means every edge whose target
is a submachine is invisible to coverage. Under the `c{}` proposal, call edges target a
machine and return edges originate from `Final` — so *both* kinds of edge introduced by
recursion would be the only edges in the model not counted, and the figure would look
perfect while the recursive structure went entirely untested. This is tracked as §10.3 of
`RECURSIVE-STATE-MACHINES.md`; it needs a decision before `c{}` is implemented.

Excluding `INIT` and `FINAL` is defensible. Excluding `SM` is the questionable one.

---

<a name="d7"></a>
## D7. `log_triggered_transitions` is disabled by an undocumented `return;`

**Severity:** Low.
**Area:** Trace.
**File:** `core/src/sm_sim_core_simulation_loop.cpp:245-248`.

```cpp
static void log_triggered_transitions(State_machine_simulation_core* smc,
                                      executionloop_context_t & execution_ctxt,
                                      std::vector<int> & triggered_transitions,
                                      std::vector<int>::iterator end_of_trans_it){
return;
if (smc->live_logger() || !smc->quiet_mode()){
   ...
```

The unconditional `return;` on the first line disables the whole body.

### This is probably intentional

`SKILL.md` states the omission as a design decision, not an oversight: execution traces
are compact projections onto sequences of sets of state changes, they "omit a lot of
potentially interesting information like triggered events, taken transitions, evalutated
guards etc. This is intentionally so, execution traces are not diagnositc traces", and
**observer state machines are presented as the canonical way to enrich them**.

So the defect here is not the behaviour — it is that a deliberate decision is recorded as
a `return;` statement in front of fifteen lines of live-looking code, with no comment
saying so. The next reader will take it for an oversight, as this one did.

### Fix

Either delete the function and note the decision where the trace format is documented, or
keep it behind an explicit opt-in. The consequence worth weighing is that an observer must
be added to the *model* in order to diagnose it, so an automated consumer cannot recover
the taken edge from a run it has already performed. See `ROADMAP.md` phase 2.3.

---

<a name="d8"></a>
## D8. Three of the four report-format flags are inert

**Severity:** Low as a bug, high as an opportunity — see the note below.
**Area:** CLI / reporting.

Four output formats are offered. Only one is implemented:

| Flag | Declared | Parsed | Consumed |
|---|---|---|---|
| `--report_format_sexpression` | yes | yes | **yes**, `core/src/state_machine_simulation_core.cpp:1863` |
| `--report_format_json` | `core/include/cmdline_utils.hpp:83` | `core/src/cmdline_utils.cpp:244` | **no** |
| `--report_format_ceps` | yes | yes | **no** |
| `--report_format_xml` | yes | yes | **no** |

Passing any of the latter three is accepted and silently changes nothing; the caller gets
the default human-readable report and no signal that the request was ignored. An
accepted-but-inert flag is worse than an unrecognised one.

### Note: the fix is far smaller than it looks

The report is **already structured data**. `print_report` receives a
`ceps::ast::Nodeset` with a settled schema —
`report["summary"]["coverage"]["state_coverage"]["ratio" | "valid" | "percentage"]`,
`report["summary"]["general"]["states_total"]`, `report["summary"]["categories"]` —
and the s-expression branch is nothing but a pretty-print of that node set:

```cpp
:1863   if (result_cmd_line.report_format_sexpression) {
            os << ceps::ast::Nodebase::pretty_print << report;
            return;
        }
```

A Nodeset-to-JSON serialiser already exists as `ceps2json`
(`core/src/api/websocket/ws_api.cpp:151`, with `escape_json_string` at `:49` and
`fast-json.hpp` behind it). So `--report_format_json` is a branch alongside the existing
one that calls a function already written and already exercised by the websocket API —
wiring, not writing. `--report_format_ceps` is similar, the node set being ceps already.

This matters out of proportion to its severity: a stable, versioned machine-readable
report is the interface every automated consumer needs, and it is nearly free. See
`ROADMAP.md` phase 1A.

---

<a name="d9"></a>
## D9. `log_verbosity` cannot be set from the command line

**Severity:** Low.
**Area:** CLI.

`log_verbosity` is an internal field defaulting to `0`
(`core/include/state_machine_simulation_core.hpp:247`) and is read at
`core/src/sm_sim_core_simulation_loop.cpp:265` and `:293`. No command-line flag assigns
it — there is no `-v`, `--verbosity` or `--log_verbosity`.

The guarded output is therefore unreachable in a normal build. This is what made [D3](#d3)
hard to diagnose: the natural first hypothesis for a missing trace line is a verbosity
setting, and there is no way to rule it out from outside.

---

<a name="d10"></a>
## D10. `SKILL.md` is incomplete and self-contradictory on submachine references

**Severity:** Low — documentation only. No model is rejected because of it.
**Area:** Documentation.
**File:** `SKILL.md:114` and `SKILL.md:116`.

Line 114 says:

> A sub machine B of machine A is refered to by A.B .

Line 116, two bullets later, says:

> all state ids (composite and non-composite) are interpreted relative to the enclosing
> sm-block (lexical scope)

Taken together these imply that `A.B` written inside `A` resolves as `A` → `A` → `B`,
which does not exist. A reader who reasons that far concludes nested machines cannot be
referenced from their own parent.

### What actually happens

Resolution accepts both a path relative to the enclosing machine and one qualified from
the root, and the bare name is simply the one-element relative path. All three of these
are accepted:

| Written inside `A` | Resolves |
|---|---|
| `t{Initial;B;}` | yes — bare/relative |
| `t{Initial;A.B;}` | yes — root-qualified |
| `t{Initial;B.C;}` | yes — relative, two levels |
| `t{Initial;A.B.C;}` | yes — root-qualified, two levels |
| `t{Initial;A.A.B;}` | no — `Error:A` |

So line 114 is not *wrong*; it is incomplete. It presents the qualified form as the only
form, while the bare form is both valid and the more natural one from within the parent.
Line 116 then describes a scoping rule strict enough to contradict line 114's example.

### Why it matters

The bare form is the one that matters for `c{}`: a recursive call names its own enclosing
machine, and the relative path for that is the bare name. Documentation that implies
otherwise sends a reader towards the qualified form and, if resolution is read strictly,
towards the conclusion that the construct is unsupported.

### Fix

State the rule once, in one place: *a state or machine id is a dot-separated path,
resolved first relative to the enclosing `sm` block and then from the root; a submachine
`B` of `A` may therefore be written `B` from inside `A`, or `A.B` from anywhere. Traces
and coverage reports always display the fully qualified form.*

---

<a name="d11"></a>
## D11. State-index intervals are not contiguous across cover classes

**Severity:** Low as things stand; a trap for anything that assumes otherwise.
**Area:** Internals.
**File:** `core/src/state_machine_simulation_core_buildsms.cpp:250-270`.

`traverse_sm` (`:126`) is a DFS pre-order walk, so each machine's subtree is normally a
half-open index interval `[idx_, idx_ + width)`. That property is what makes
"is state *i* inside machine *m*" an integer comparison.

It does not hold across cover classes. Index assignment runs in **two passes** gated on
`non_cover_sm`, with `start_of_covering_states = old_state_ctr` at `:270`. A covering
machine nested inside a non-covering one is numbered in the second pass, at the tail, so
the enclosing machine's subtree is split in two.

Observable in the trace ordering: names are emitted in index order, so

```
sm{A; concept; states{Initial;}; sm{B; states{Initial;};}; t{Initial;B;}; };
```

prints `A.B+ A.B.Initial+ A.Initial-` while the all-covering variant prints
`A.Initial- A.B+ A.B.Initial+` — `B`'s indices sit below `A`'s in the first case.

Nothing in the current engine depends on the interval property, so this is latent. It
stops being latent for `c{}`, whose frame mechanism restricts stepping to the callee's
index interval; that is why the specification carries static check E4
(`RECURSIVE-STATE-MACHINES.md` §5, §6.1).

Worth noting separately that trace lines list names in index order rather than causal
order, so within a single line an entry can precede the exit that caused it.

---

## Reproducing

All models above are small enough to paste into a file and run directly:

```
ceps <file>.ceps ; echo $?
```

Exit codes are `0` on success, `1` on a fatal error, `139` for [D2](#d2). When checking
the exit code after a pipeline, remember that `$?` reports the *last* command's status —
capture it before piping.

## Relationship to `RECURSIVE-STATE-MACHINES.md`

[D3](#d3) and [D4](#d4) are documented in §9.6 of that specification because they
constrain acceptance criterion A12. [D6](#d6) is §10.3 and needs a decision before `c{}`
is implemented. [D11](#d11) is the motivation for static check E4. The remainder are
independent of the `c{}` work.
