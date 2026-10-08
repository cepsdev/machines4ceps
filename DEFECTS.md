# Known defects

Defects found while auditing `core/` and `vm/`, while specifying `c{}`
(see `RECURSIVE-STATE-MACHINES.md`), during the repository-wide sweep recorded in
`INVENTORY.md`, and — for [D17](#d17) — while reading production transformations written
against ceps outside this repository. Every entry below was reproduced against `bin/ceps`,
version 0.8.1.3.3 (built Aug 27 2026), on Linux.

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
| [D12](#d12) | High | Docs | The model-traversal layer is absent from `SKILL.md` |
| [D13](#d13) | High | Evaluator | Named arguments in a call (`f(a = 1)`) are rejected as an unsupported assignment |
| [D14](#d14) | Critical | Simulation | The current-state set is empty in assertions, so `ASSERT_CURRENT_STATES_CONTAINS` never holds and its negation always does |
| [D15](#d15) | High | VM | The Oblectamenta assembler rejects guard expressions the language admits |
| [D16](#d16) | High | Transport | Crash (SIGSEGV) on a `receiver` with a `canbus` transport and no frame definitions |
| [D17](#d17) | Critical | Language | A loop variable named after an SI unit silently binds to the unit, producing wrong output with exit code 0 |

D13–D16 were found by the sweep and are regressions against material that is still in the
tree as tests and examples. D14 is rated Critical because it disables the mechanism ceps
uses to check itself, and because one half of it fails silently green.

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

<a name="d12"></a>
## D12. The model-traversal layer is absent from `SKILL.md`

**Severity:** High — the capability that most distinguishes ceps is undiscoverable from
its own documentation.
**Area:** Documentation.
**Files:** `SKILL.md`, `QUICK-START-UML-WITH-CEPS.md:84-86`.

A ceps model is traversable from inside ceps. `root` is an ordinary value, `root.sm`
yields the machines, `a_machine.t` the transitions, and a small query algebra operates on
the result. This is what makes a model transformation a short ceps program instead of a
C++ plugin: the statechart-to-mermaid converter in
[cepsdev/mermaid](https://github.com/cepsdev/mermaid) is 35 lines, the reverse direction
is 10, and event-signature extraction across nested machines is 81.

None of it appears in `SKILL.md`. Every one of the following is used in working programs
in that repository, and none is mentioned:

- `root` as a traversable value, and `arglist` as the parameter of a `macro`
- `.content()`, `.at(n)`
- `.symbol()`, `.symbol("Event")`
- `.sort()`, `.unique()`, `.is_struct()`
- `.fetch_recursively_symbols()`
- `predecessor()`, which refers to the preceding sibling node and is how a staged
  computation is expressed
- `for (x : nodeset) { … }` over model nodes, and `if(!last)` inside it
- that the execution report is itself queryable, via
  `root.summary.coverage.state_coverage.covered_states`

`QUICK-START-UML-WITH-CEPS.md:84-86` links the repository exactly once, under the heading
*"Visualization using mermaid.js"*, with the text *"More information on that is found
here"*. Nothing suggests a metaprogramming reference sits behind that link.

### Why it matters

`SKILL.md` is the LLM-facing document, so it defines what an agent believes ceps can do.
An agent working from it will emit state machines and nothing else — it will not write a
model transformation, derive an interface, or query a report, because it has no reason to
think those are possible. The feature that separates ceps from every other statechart
tool is invisible precisely where visibility pays.

### Fix

Document the traversal layer in `SKILL.md`: the combinator list above, plus two worked
examples — one structural transformation and one query over a report. Reclassify the
`cepsdev/mermaid` link from *visualization* to what it actually is, the de facto reference
for model transformation, and consider moving the material into this repository.

---

<a name="d13"></a>
## D13. Named arguments in a call are rejected as an unsupported assignment

**Severity:** High — two complete application areas in `test/` cannot run.
**Area:** Evaluator.
**Affects:** 18 files, among them all of `test/jenkins/` (6) and `test/mms_devops/` (6),
plus `test/fibex/sim1.ceps`, `sim_can3xml_1.ceps`, `sim_can3xml_2.ceps`.

An `=` inside a call-parameter list is treated as an assignment statement and refused.
The same symbol assigned inside `globals{}` works, so the defect is specific to the
call-argument position, not to assignment in general.

### Reproducer

```
kind Parameter;
Parameter command, hostname;
foo(command = "build", hostname = "h");
```

### Observed

```
***Fatal Error:Unsupported assignment:(OPERATOR = "=" (SYMBOL "command" "Parameter" )"build" )
```

### Control

Identical declaration, assignment moved into `globals{}` — exits 0:

```
kind Parameter;
Parameter command;
globals{ command = "build"; };
```

### Why it matters

This is the calling convention the Jenkins and MMS DevOps models are written in:

```
 jenkins(
         command                = "build",
         hostname               = ...,
```

Both areas are otherwise intact. One evaluator fix restores twelve models and the two
largest non-automotive application examples in the repository.

---

<a name="d14"></a>
## D14. The current-state set is empty in assertions

**Severity:** Critical — the mechanism by which a ceps specification checks itself does
not work, and the failure is silent in one direction.
**Area:** Simulation / assertions.
**Affects:** every `Simulation{}` block using state assertions, including
`test/systemparameter.ceps`, which is labelled *"Regression test"* in its own header, and
`examples/guard_usage_example.ceps`.

`ASSERT_CURRENT_STATES_CONTAINS` reports the current-state set as empty regardless of the
actual configuration. The trace, written by the same run, shows the state was entered.

### Reproducer

```
kind Event; Event e;
Statemachine{ id{S;}; States{Initial;a;}; Transition{Initial;a;e;}; };
Simulation{ Start{S;}; e; ASSERT_CURRENT_STATES_CONTAINS{S.a;}; };
```

### Observed

```
S.Initial- S.a+

***Fatal Error:
ASSERTION not satisfied (ASSERT_CURRENT_STATES_CONTAINS):
Expected to be in state S.a, current states are:
.
```

The trace line `S.a+` records entry into `S.a`; the assertion, evaluated afterwards, sees
nothing. The set is empty even with no events at all — asserting `S.Initial` immediately
after `Start{S;}` fails the same way.

### The silent half

Because the set is empty, the complementary assertion is vacuously true. The following
exits 0 and reports nothing:

```
kind Event; Event e;
Statemachine{ id{S;}; States{Initial;a;}; Transition{Initial;a;e;}; };
Simulation{ Start{S;}; e; ASSERT_CURRENT_STATES_CONTAINS_NOT{S.Initial;}; };
```

So `..._CONTAINS` never holds and `..._CONTAINS_NOT` always holds. A specification using
the negative form passes while checking nothing.

### Where the fault is not

The simulator's own notion of the current state is **correct** — only the assertion path
sees an empty set. Two independent witnesses:

1. The trace prints enter and exit events in the right order (`S.Initial- S.a+` above).
2. Coverage is computed from the same information and comes out right. Running the
   partition example in [INVENTORY.md](INVENTORY.md) §4.2 reports `State Coverage: 0.75`
   with three of four states entered, which is only derivable from a populated state set.

So this is not a state-tracking bug. It is a lookup performed against the wrong object, or
performed at a point in the cycle where the set has already been cleared — which makes it
a much smaller fix than the severity suggests, and is the reason it belongs in phase 0.

### Why it matters

`POSITIONING.md` argues that ceps's distinguishing property is making described behaviour
executable, and therefore checkable, before agreement exists. State assertions are the
primary way a ceps model states what it expects of itself. While D14 stands, the positive
form blocks any model that uses it and the negative form gives false assurance — which is
worse than having no assertions at all.

This defect is also the likely reason several areas in the sweep appear to fail: the model
runs correctly and the assertion that follows it does not.

---

<a name="d15"></a>
## D15. The Oblectamenta assembler rejects guard expressions the language admits

**Severity:** High — guard forms that are documented and used in the tree do not compile,
and one of the two model-based-testing constructs is disabled by it.
**Area:** Oblectamenta VM / expression compilation.
**Affects:** 6 files, among them `test/guard/guards.ceps`, `test/markdown/run_of_sm.ceps`,
`test/shadow_states/d.ceps`, `examples/math_functions.ceps`,
`examples/multiple_starts.ceps`, `examples/first_steps/simple_guard_example.ceps`.
**Also disables `cover_path{}` entirely** — see below.

Since guards and actions are compiled to Oblectamenta, expressions the front end parses
can still fail at assembly. Two distinct classes were observed.

### Reproducer A — `in_state` with a qualified id

From `test/guard/guards.ceps`:

```
***Fatal Error:***Error oblectamenta_assembler: Expression failed to compile.
Offending expression is >>>(FUNC_CALL (ID "in_state" )(CALL_PARAMETERS (OPERATOR . "" (ID "S1" )(ID "A" ))))<<<
```

### Reproducer B — floating-point comparison through `abs`

```
kind Event; Event e;
kind Systemstate; Systemstate s;
globals{ s = 1; };
Statemachine{ id{S;}; States{Initial;a;}; Transition{Initial;a;e;abs(s) == 1.0;}; };
Simulation{ Start{S;}; e; };
```

Observed:

```
***Fatal Error:***Error oblectamenta_assembler: Expression failed to compile.
Offending expression is >>>(INT 0  )<<<
```

The reported offending expression is an integer literal that does not appear in the
source, so the diagnostic points at an artefact of lowering rather than at the guard the
user wrote. Fixing the message is worth doing independently of the compilation gap; see
roadmap phase 1B.

### Why it matters

`in_state(Machine.State)` is the canonical way to write a guard that depends on another
machine, which makes it load-bearing for exactly the composition story `ROADMAP.md`
phase 7 depends on.

It is also worse than a six-file problem. `cover_path{}`
(`core/src/modelling/cover_path.cpp`, registered at
`core/src/state_machine_simulation_core.cpp:1332`) **generates** `in_state()` guards — that
is its entire output. So D15 disables the construct, not just the files that use it:

```sh
$ cd examples/doing_specs/vehicle_navigation
$ ceps .ceps/prelude.ceps examples_of_operations/stored_data_invalid_gps_alignment.ceps
***Fatal Error:***Error oblectamenta_assembler: Expression failed to compile.
Offending expression is >>>(FUNC_CALL (ID "in_state" )(CALL_PARAMETERS
  (OPERATOR . "" (ID "Vehicle" )(ID "Standstill" ))))<<<
```

Its sibling construct `partition{}` works, so the modelling layer is half alive. See
[INVENTORY.md](INVENTORY.md) §4.2.

There is a third consequence, in the document renderer. `--ppe --format markdown` annotates
the generated specification with a `Visited` column taken from the run
([INVENTORY.md](INVENTORY.md) §4.3) — which requires the run to finish. For any model whose
transitions carry guards, it does not: `test/markdown/run_of_sm.ceps` renders its state and
transition tables and then dies in the assembler, so the annotation is absent.
`test/markdown/README.md`, a saved specimen from an earlier build, shows the coverage bars
that can no longer be produced.

So D15 blocks three separate capabilities — guarded models, `cover_path{}`, and
coverage-annotated documentation. This raises it from "some guards do not compile" to "a
feature of the tool cannot be used at all", and it is the strongest argument for putting
D15 in phase 0.

---

<a name="d16"></a>
## D16. SIGSEGV on a `receiver` with a `canbus` transport and no frame definitions

**Severity:** High — a crash on a four-line model, reached by omission rather than by
error.
**Area:** Transport / CAN.
**Affects:** `doc/ceps/can_comm/example_1`, `example_4`, `example_5`,
`doc/vcan_api/two_nodes_no_hub` — in each case the `receiver.ceps` fragment when it is not
composed with the file defining the frames.

### Reproducer

```
receiver{
 id{can_in;};
 transport{canbus{bus_id{"vcan0";};};};
};
```

```
$ ceps d16.ceps ; echo $?
Segmentation fault
139
```

### Scope

- The bus need not exist. `bus_id{"doesnotexist99";}` crashes identically, so this is not
  a failure to open the interface.
- A `sender` with the identical transport block exits 0.
- A `receiver` with no `transport` block at all exits 1 with a diagnostic.

So the crash is specific to the receiver path when a transport is present and no frame
mapping has been declared. Composed with its frame definitions the model runs, which is
why the examples work when started through their shell scripts.

### Why it matters

The failure mode is omission — a user writing a receiver before declaring frames gets a
segfault instead of the message that the `receiver` case without a transport already
produces. This is the same shape as [D2](#d2): a missing declaration reaching a
dereference instead of a check.

---

<a name="d17"></a>
## D17. A loop variable named after an SI unit silently binds to the unit

**Severity:** Critical — not a crash, a *wrong answer*. Exit code 0, plausible-looking
output, no diagnostic of any kind.
**Area:** Language / name resolution.
**Affects:** every `static_for` and `for` comprehension, and therefore every transformation
in the `rollAut` style. In principle any construct that introduces a binding.

### Reproducer

Two files differing only in the name of the inner loop variable.

```
rollout{ markets{ market{id{"M1";};}; market{id{"M2";};}; }; };
static_for(e : root.rollout){
  static_for(m : e.markets.market){
    seen{ m.content().id.content(); };
  }
}
```

```
$ ceps d17.ceps --pe ; echo $?
...
(STRUCT "seen"
  (OPERATOR . ""
    (OPERATOR . ""
      (OPERATOR . ""
        (INT 1 m^1 )
        (FUNC_CALL
...
0
```

`m` has bound to the SI unit **metre** — visible as `m^1` in the dump — and the path
expression is left unevaluated as an operator tree. Rename the variable and nothing else:

```
    static_for(market : e.markets.market){
      seen{ market.content().id.content(); };
    }
```

```
$ ceps d17b.ceps --pe ; echo $?
(STRUCT "seen"
  "M1"
)
(STRUCT "seen"
  "M2"
)
0
```

### Scope

The unit system is a genuine and useful feature — `timeout{2.0*s;}` in
`examples/doing_specs/lueftersteuerung/` depends on it. The defect is not that units
exist; it is that **a unit name wins against a binding introduced in the same expression,
silently**. The candidate collision set is large and consists entirely of names a
programmer would reach for: `m`, `s`, `A`, `K`, `g`, `cd`, `mol`, and whatever derived and
prefixed forms the unit table carries.

The existing corpus escapes this by accident. Every comprehension in `planck-service`,
`free-mdf/analyze-mdf` and `rollAut` happens to use `e`, `market`, `step` or `i`. No
convention was ever written down, and nothing warns.

### Why it matters

Every other defect in this file announces itself: a crash, a non-zero exit, a diagnostic,
a transition that fails to fire. This one produces a well-formed document containing the
wrong data, and the only way to notice is to already know the answer.

That is tolerable in a tool with one user who has internalised the collision set. It is
disqualifying for two of the directions the project is now pointed at. Generated code —
see [LANG-ROADMAP.md](LANG-ROADMAP.md) — will use `m` for a market and `s` for a step,
because that is what reads naturally and no corpus teaches otherwise. And an analysis
pipeline over contracts or measurements has no oracle: a silently wrong aggregate is
indistinguishable from a right one.

### What a fix looks like

In order of increasing ambition:

1. **Diagnose.** Reject a binding whose name is in the unit table, at the point of
   binding, with a message naming the collision. Cheap, immediate, and it converts a
   wrong answer into a build failure.
2. **Shadow, and say so.** Let the binding win — which is what a reader expects — and emit
   a warning. Requires deciding what `2.0*s` means inside a `static_for(s : …)` body; the
   answer is probably "the binding", with the unit reachable under an explicit
   qualification.
3. **Separate the namespaces.** Units are not ordinary identifiers and arguably should not
   share a lookup with them.

(1) is a defensible release in itself and should not wait for (2) or (3).

---

## Reproducing

All models above are small enough to paste into a file and run directly:

```
ceps <file>.ceps ; echo $?
```

Exit codes are `0` on success, `1` on a fatal error, `139` for [D2](#d2) and
[D16](#d16). When checking the exit code after a pipeline, remember that `$?` reports the
*last* command's status — capture it before piping.

Several models in the tree are **fragments** that are composed on the command line rather
than run alone:

```
ceps common.ceps frames.ceps receiver.ceps receiver_node.ceps
```

Running such a fragment by itself produces a parse error that says nothing about the
model. The sweep in `INVENTORY.md` initially mis-classified 29 files this way.

## Relationship to `RECURSIVE-STATE-MACHINES.md`

[D3](#d3) and [D4](#d4) are documented in §9.6 of that specification because they
constrain acceptance criterion A12. [D6](#d6) is §10.3 and needs a decision before `c{}`
is implemented. [D11](#d11) is the motivation for static check E4. [D14](#d14) blocks the
acceptance criteria themselves: every criterion in §15 that is phrased as an expected
configuration is unverifiable while state assertions do not see the configuration. The
remainder are independent of the `c{}` work.
