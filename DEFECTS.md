# Known defects

Defects found while auditing `core/` and `vm/`, while specifying `c{}`
(see `RECURSIVE-STATE-MACHINES.md`), during the repository-wide sweep recorded in
`INVENTORY.md`, for [D17](#d17) while reading production transformations written
against ceps outside this repository, for [D18](#d18)–[D20](#d20) while checking
whether model stacking satisfies an incrementality law, and for [D22](#d22)–[D23](#d23)
while writing the worked example in `doc/scribble-concept/README.md`, and for
[D24](#d24) while reading the Oblectamenta serializer generator. Every entry below
was reproduced against `bin/ceps`, version 0.8.1.3.3, on Linux — D1–D21 against the
Aug 27 2026 build, D22–D24 against the Oct 9 2026 build of the same version string.

Line numbers refer to the working tree at the time of writing.

| # | Severity | Area | Summary |
|---|---|---|---|
| [D1](#d1) | High | VM | `bgt`, `bneq`, `callx` are implemented but not declared, so they cannot be assembled by hand — though the C++ generator emits `bneq` regardless ([note](#d1-note)) |
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
| [D17](#d17) | Critical | Language | Any name colliding with an SI unit silently resolves to the unit — loop variables bind to it, and a node so named is unreachable even by explicit path; exit 0 |
| [D18](#d18) | High | Evaluator | `.content()` collapses singletons, so appending one element silently turns a working expression into a half-evaluated residue; exit 0 |
| [D19](#d19) | High | Evaluator | `.children()` yields the empty sequence instead of reporting an unknown accessor, so loops over it silently do not run |
| [D20](#d20) | High | Printer | The evaluated document is not a ceps document — `--pe`/`--ppe` output does not re-parse |
| [D21](#d21) | High | CLI / Docs | Forty-six of the sixty-three command-line flags are undocumented, including the working C++ generator `--cppgen` |
| [D22](#d22) | High | Printer | `--pe`/`--ppe` print a macro as a raw heap address and omit its body, so two runs of one input produce two different documents |
| [D23](#d23) | Medium | Evaluator | A macro used as `name;` instead of `name();` is silently not expanded and reaches the output as a bare identifier; exit 0 |
| [D24](#d24) | Medium | VM | The serializer generator names opcodes by string literal, bypassing the compile-time checking that `emit<Opcode::X>` provides to every other emitter |

D13–D16 were found by the sweep and are regressions against material that is still in the
tree as tests and examples. D14 is rated Critical because it disables the mechanism ceps
uses to check itself, and because one half of it fails silently green.

## Triage

Severity says how bad a defect is. It does not say what it holds up. At twenty-four
entries that distinction is the one that matters, so it is drawn here.

### One diagnosis, repeated

**Fourteen of the twenty-four are the same failure mode**: a plausible answer with exit
code 0 and nothing said about what was skipped — D1, D3, D6, D7, D8, D12, D14, D15, D17,
D18, D19, D21, D22, D23. **Both remaining Critical defects — D14 and D17 — are in that
class.** The two SIGSEGVs are merely High; a crash is the easy kind. (D18 was a third
Critical until 2026-10-09, when it turned out to be narrower than filed; it stays in the
silent class. D24 is deliberately outside it: it throws, and names the offending
mnemonic. The class is a finding, not a framing — entries are admitted to it on evidence.)

This is worth stating plainly because it changes what the list is. It is not an
assortment of unrelated faults, it is one property of the system observed from fourteen
angles: *ceps conceals what it did not do.* Which is the exact behaviour
`POSITIONING.md` argues the language exists to prevent. Fixing them one at a time treats
the symptoms; the shared fix is a representable third outcome.

### Group 1 — blocks the thesis or its evidence

These make a claim untrue, or corrupt the measurement that would support it. They come
first regardless of severity.

| # | why it is here |
|---|---|
| [D18](#d18) | **Re-scoped 2026-10-09**, twice. The canonical example *does* reproduce (with `.at(0)`), so it no longer blocks the examples. But the real defect it uncovered — appending silently breaks readers — is the paper's own thesis reproduced inside the evaluator, and fixing it buys a sentence the paper wants. |
| [D17](#d17) | Silent wrong answers in exactly the transformations the language is being proposed for. **Broadened 2026-10-09**: it is not confined to bindings — a *node* named after a unit is unreachable even by an explicit `root.` path, and returns empty rather than erroring. It also corrupted a probe in this file, which is the sharpest evidence of its reach. |
| [D14](#d14) | A model cannot check itself; every acceptance criterion phrased as a configuration is unverifiable. |
| [D20](#d20) | Evaluated output is not re-parsable, so "the document stays well-formed" is false, and incrementality cannot be tested. |
| [D22](#d22) | Two runs of one input produce two different documents, and a macro's body is absent from both. Trace comparison is the proposed experiment; this breaks diffing at the source. Same fix surface as D20 — do them together. |
| [D19](#d19) | Silent empty traversal — and the best first site for a `nonhitter{}` demonstration. |
| [D5](#d5), [D6](#d6) | Coverage is the metric the argument rests on; one prints `-nan`, the other drops edges without saying so. |
| [D3](#d3), [D4](#d4) | **Reclassified.** Trace defects were cosmetic while the trace was a debugging aid. They are not cosmetic now that trace comparison is the proposed experiment — a missing initial-entry line and a spurious trailing line both corrupt an alignment. |

### Group 2 — blocks adoption

Capability present, surface absent or broken. Nothing here is wrong about the language;
all of it is wrong about meeting it.

| # | why it is here |
|---|---|
| [D21](#d21) | 46 of 63 flags invisible, including a working C++ generator. An evaluator reads `--help` and sees a small simulator. |
| [D12](#d12), [D10](#d10) | The traversal layer is undocumented and the submachine documentation contradicts itself — the two things a newcomer needs first. |
| [D2](#d2), [D16](#d16) | Crashes on ordinary mistakes: a machine without `Initial`, a receiver without frames. |
| [D13](#d13), [D15](#d15) | The tool rejects input the language admits. Regressions against material still in the tree. |
| [D1](#d1) | Three working VM instructions unreachable because they are undeclared. |
| [D18](#d18) | A scribble that works stops working when you add to it, silently. That is the one operation the language promises is safe. |
| [D23](#d23) | A macro invoked with the wrong one of two near-identical syntaxes vanishes without a word. `name;` is how paths splice, so the wrong guess is the natural one. |
| [D8](#d8) | Three report formats accepted and ignored; the fix is a pretty-print away. |
| [D9](#d9) | A verbosity control that exists but cannot be reached. |

### Group 3 — internal

Real, worth fixing, holding up nothing.

| # | |
|---|---|
| [D7](#d7) | A feature switched off by a stray `return;`. |
| [D11](#d11) | State-index intervals non-contiguous across cover classes. |

### Order of work

1. **Group 1, and D20 with [D22](#d22) first within it** — both are printer changes rather
   than semantics changes, they touch the same dispatch, and they are visible to anyone
   who tries the examples. D22 additionally buys a three-line regression test (`--pe`
   twice, `diff`, require identical) that guards every future printer change.
2. **D21 next**, out of order, because generating the flag list from the parser is an
   afternoon and it changes what the project looks like from outside.
3. The rest of group 2 as the implementation month allows; group 3 when touched for other
   reasons.

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

<a name="d1-note"></a>

### Note added 2026-10-10 — `bneq` is not unreachable, and that changes the fix

The entry above says three working instructions are unreachable. That is right for `bgt`
and `callx`. It is **wrong for `bneq`**, and the correction matters more than the original
finding.

`bneq` is emitted by the serializer generator, four times, at
`core/src/vm/oblectamenta-assembler.cpp:858`, `:894`, `:930` and `:966` — all inside
`oblectamenta_assembler_preproccess_match` (`:640`), the generator for the *read* path.
Each site is the same shape: load a child node's `what` field, compare it against the
expected `msg_node` tag, and branch away on mismatch:

```
em(r,"ldsi32");
em(r,"ldi32",msg_node::INT32);
emwsa(r,"bneq",lbl_wrong_node_type_i32_expected,"OblectamentaCodeLabel");
```

So the one undeclared opcode that is actually used is the instruction that **type-checks
incoming messages**. Pruning it, which is the other obvious reading of "implemented but
not declared", would delete wire-format validation from every generated deserializer. The
fix is to declare the three, as the entry says — not to remove them.

**Why it works at all.** `gen_mnemonic` builds the node directly in C++:

```
return mk_symbol(name, "OblectamentaOpcode");
```

That confers the `OblectamentaOpcode` kind without consulting
`core/include/oblectamenta_decls.ceps`. The declaration file gates the *parser*, so it
gates hand-written assembly only. Generated assembly never passes through it.

**The structural statement** is therefore sharper than a missing line in a list: the
assembler has **two front doors with different vocabularies**. Hand-written Oblectamenta
can use what `oblectamenta_decls.ceps` declares; the C++ generator can use anything in the
`mnemonics` table in `core/include/vm/vm_base.hpp`. Nothing requires the two to agree, and
as of this writing they do not. This is also the likely reason the test suite survived the
change that stopped predeclaring opcodes by default: the generated path does not read the
declarations, and it is the path the tests exercise.

**The guard in the note above is accordingly too weak.** Two lists are not the invariant.
Four artefacts must agree, and all four are maintained by hand:

| Artefact | Location | Count |
|---|---|---|
| `enum class Opcode` | `core/include/vm/vm_base.hpp` | 171 |
| `op_dispatch.push_back` sequence | `core/src/vm/vm_base.cpp` | 171 |
| `mnemonics` table | `core/include/vm/vm_base.hpp` | name → opcode → emitter |
| `OblectamentaOpcode` declarations | `core/include/oblectamenta_decls.ceps` | 150 |

The enum and the dispatch sequence agree only because they are positional — a
`push_back` inserted out of order would misroute an opcode to the wrong handler with no
diagnostic at all. A CI diff catches the declaration gap; it does not catch that. The
construction that removes all four problems at once is to derive every artefact from a
single X-macro list, so that adding an opcode is one edit and desynchronisation becomes
unrepresentable. See also [D24](#d24), which is the same root cause seen from the
generator's side.

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
sees an empty set. Three independent witnesses:

1. The trace prints enter and exit events in the right order (`S.Initial- S.a+` above).
2. Coverage is computed from the same information and comes out right. Running the
   partition example in [INVENTORY.md](INVENTORY.md) §4.2 reports `State Coverage: 0.75`
   with three of four states entered, which is only derivable from a populated state set.
3. The `[ACTIVE_STATES]` log line prints the complete configuration — all 36 top-level
   machines plus the hierarchical paths — correctly and on every step. Verified against
   production logs from 2016 (`TRGS/statemachines/log_pure_runtime_with_log.txt`,
   1671 configurations; `log_native_with_log.txt`, 1988). The capability is not merely
   present, it has been in industrial use.

So this is not a state-tracking bug. It is a lookup performed against the wrong object, or
performed at a point in the cycle where the set has already been cleared — which makes it
a much smaller fix than the severity suggests, and is the reason it belongs in phase 0.
Witness 3 narrows it further: the logger and the assertion have different views of the
same run, so the fix is to point the assertion at whatever the logger reads.

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
## D17. A name that collides with an SI unit silently resolves to the unit

**Severity:** Critical — not a crash, a *wrong answer*. Exit code 0, plausible-looking
output, no diagnostic of any kind.
**Area:** Language / name resolution.
**Affects:** every `static_for` and `for` comprehension, and therefore every transformation
in the `rollAut` style — and, as of the 2026-10-09 broadening below, **any name in the
document at all**, not only names introduced by a binding.

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

The mechanism is that **a unit name wins against every other name, silently**. Whether
built-in units should exist at all is answered further down, by the person who put them
in; what comes first is the shape of the collision.

The candidate set is large and consists entirely of names a programmer would reach for:
`m`, `s`, `A`, `K`, `g`, `cd`, `mol`, and whatever derived and prefixed forms the unit
table carries.

The existing corpus escapes this by accident. Every comprehension in `planck-service`,
`free-mdf/analyze-mdf` and `rollAut` happens to use `e`, `market`, `step` or `i`. No
convention was ever written down, and nothing warns.

### Wider than bindings — added 2026-10-09

This entry was first filed as a defect about *bindings* — loop variables and the like.
That was too narrow. The collision reaches **document structure**, and an explicit path
does not escape it.

A plain struct named `m` defines and prints correctly, and is then unreachable:

```
$ cat t3.ceps
m{ 42; };
holder{ m; };

$ ceps --pe t3.ceps ; echo $?
(STRUCT "m"
  (INT 42  )
)
(STRUCT "holder"
  (INT 1 m^1 )
)
0
```

`holder` contains *one metre*. The 42 is gone. Nothing was bound anywhere — `m` is a
node, not a loop variable.

Worse, a fully qualified path does not help. It returns the empty result, silently:

```
$ cat t4.ceps
m{ 42; };
holder{ root.m.content(); };

$ ceps --pe t4.ceps ; echo $?
(STRUCT "m"
  (INT 42  )
)
(STRUCT "holder"
)
0
```

Control, identical but for the name:

```
$ cat t5.ceps
zz{ 42; };
holder{ root.zz.content(); };

$ ceps --pe t5.ceps
(STRUCT "zz"
  (INT 42  )
)
(STRUCT "holder"
  (INT 42  )
)
```

So a node whose name is in the unit table can be written, can be printed, and cannot be
read back by any spelling tried — bare or fully qualified. `root.m` is as unambiguous as
the language gets, and it still yields nothing, with exit code 0.

This was found the hard way: it corrupted a probe in this very catalogue. A macro named
`m` in a draft of [D23](#d23) was shadowed at its use site, and the resulting
`(INT 1 m^1 )` was briefly misread as evidence that macro expansion dropped statements.
D23 was rewritten once the cause was known. The defect is good enough at hiding to fool
someone who had already written its entry.

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

### The decided direction — 2026-10-09

Built-in SI units are not to be patched. They are to be **removed**. This is the author's
decision, recorded here in his terms: units in the core were a yamdl-era choice made for
the BMW HAF project around 2013, and regretted ever since. Units should be written as
ordinary function calls — `m(1)` — with arithmetic such as `m(1) + m(2)` falling out of an
operator-overloading mechanism, a separate planned language feature deliberately **not**
filed in this catalogue.

That dissolves D17 rather than patching it. With no unit table in the name resolver there
is no collision set, `m` is an ordinary identifier, and every variant in this entry —
the loop variable, the struct, the `root.m` path — stops being special.

Two observations in support, both measured rather than asserted.

**The syntax the fix wants is currently occupied by the defect.** `m(1)` parses today, but
as a call whose *callee* is one metre:

```
$ printf 'a{ m(1); };\n' > u.ceps ; ceps --pe u.ceps
(STRUCT "a"
  (FUNC_CALL
    (INT 1 m^1 )
    (CALL_PARAMETERS
      (INT 1  )
    )
  )
)
```

The node shape the fix needs is already there; only the callee resolves wrongly. Removing
built-in units vacates the syntax rather than requiring new grammar.

**The migration cost is smaller than it looks.** Sixty-nine `.ceps` files in `examples/`
and `test/` use the `N*unit` form, which sounds like a large corpus to rewrite. By unit:

```
$ grep -rhoE '\*\s*(s|ms|m|kg|A|K|g|cd|mol|Hz|us|ns)\s*[;,)}]' --include=*.ceps examples/ test/ \
  | grep -oE '\*\s*[a-zA-Z]+' | tr -d '* ' | sort | uniq -c | sort -rn
    132 s
      3 m
      3 kg
```

132 of 138 uses are seconds, nearly all of them timer arguments such as
`start_timer(3.0*s,E)`. The three `m` and three `kg` uses are one demo
(`weight = 10*kg; height = 100*m;`) copied into three files under
`examples/distributed/`. So the feature's entire real footprint is *seconds in timer
calls* — one unit in one role — and the price for it is a Critical defect over the whole
name space. That is a lopsided trade, and it is the empirical form of the author's regret.

**How old the commitment is — added 2026-10-10.** The yamdl repository was recovered
(`~/dev/yamdl-orig`, first commit 2013-09-25). Its **first test file**,
`core/test/test_1.yamdl`, is *entirely* SI units:

```
 kg;
 kg*ampere;
 1/(3.0*kg*ampere);
 3.0 * m/(s*s);
 2.0 * s^1*m^2*kg^3;
```

Its header reads `@version 060820131625` — 6 August 2013, earlier than the repository
itself. Units are not a later convenience bolted onto the language; they are the oldest
surviving design commitment in the project, and `test_1` is the first thing it ever
checked.

That cuts both ways and both should be stated. It explains why the unit table sits so
deep in name resolution that it beats a `root.` path. And it raises the bar for removal:
the file still runs correctly under `bin/ceps` 0.8.1.3.3 today, thirteen years on, so
what is being removed is working, tested, load-bearing-for-nothing-much code of
unusually long standing. The decision stands — one unit in one role does not pay for
D17 — but it is a retraction of a founding choice, not a cleanup, and the changelog
should say so.

### The interim fix

Removal is a language change and will not land tomorrow. Until it does:

**Diagnose.** Reject any declaration whose name is in the unit table, with a message
naming the collision, and make a path whose final segment resolves to a unit rather than
a node a diagnostic instead of the empty result. After the broadening above this must
cover three sites, not one — bindings (`static_for(m : …)`), node names (`m{42;}`) and
path segments (`root.m`). It converts a wrong answer into a build failure, it is cheap,
and it is a defensible release on its own.

---

<a name="d18"></a>
## D18. Appending to a node silently breaks every reader that assumed a singleton

**Severity:** High — silent, exit code 0. *Arguably Critical*, and in the same family as
[D17](#d17); held at High because a reliable discipline exists today (always `.at(n)`) and
the residue is visible in the output if anyone reads it. Was filed as Critical for the
wrong reason, corrected on 2026-10-09, then found to be a larger defect than either
version — see below. **Reframed 2026-10-10** by `doc/model-revision/README.md`, which
proposes append-only model revision — that is, this defect's trigger adopted deliberately
as the architecture. Under that reading the fault is not that appending breaks singleton
readers, but that *a reader can assume a singleton silently*; the fix shrinks accordingly,
from "make appending safe" to "make the assumption impossible to state without saying so".
**Area:** Evaluator / model traversal / the nodeset data model.
**Affects:** every expression that reads a node's content. The hazard is latent until the
node grows, and growing nodes is the language's designed workflow.

### Correction, 2026-10-09

This entry originally claimed that indexed access into a node's children was broken and
that the canonical trail-property example could not be reproduced. **That was wrong, and
the error was mine.** Indexing works; it is spelled `.at(n)`, applied to the result of
`.content()`. Discovered by running
`doc/scribble-concept/you_dont_erase_a_scribble.ceps`, which uses `.content().at(0)`
throughout and runs clean.

```
a{10;20;30;};
with_at_0{root.a.content().at(0);};
with_at_1{root.a.content().at(1);};
with_at_2{root.a.content().at(2);};
```

```
$ ceps --pe at.ceps
(STRUCT "with_at_0" (INT 10 ))
(STRUCT "with_at_1" (INT 20 ))
(STRUCT "with_at_2" (INT 30 ))
```

Correct, in order, no diagnostics. And the canonical example reproduces exactly:

```
a{1;2;3;};
b{6;};
fixed{root.a.content().at(0) + root.b.content().at(0);};
```

```
(STRUCT "fixed" (INT 7 ))
```

`c{7;}`, as documented. The earlier `(+6)` came from using the wrong accessor, not from
broken arithmetic.

### The real defect: `.content()` is data-dependently typed

Found 2026-10-09 while reviewing `you_dont_erase_a_scribble.ceps`, and it is a bigger
finding than the accessor spelling.

`.content()` **collapses a one-element result to that element**. This is deliberate and it
is good: it is what lets a scribble be written without ceremony.

```
a{1;};
b{2;};
collapse{root.a.content();};
sum{root.a.content() + root.b.content();};
at_on_singleton{root.a.content().at(0);};
```

```
(STRUCT "collapse"        (INT 1 ))
(STRUCT "sum"             (INT 3 ))
(STRUCT "at_on_singleton" (INT 1 ))
```

Note that `.at(0)` on a collapsed singleton is the identity, so both spellings coexist.

**Now append one element to `a` and change nothing else.**

```
a{1;5;};                                    // was a{1;}
sum_unchanged{root.a.content() + root.b.content();};   // line untouched
sum_with_at{root.a.content().at(0) + root.b.content().at(0);};
```

```
$ ceps --pe grown.ceps ; echo $?
(STRUCT "sum_unchanged"
  (OPERATOR + ""
    (NODESET ""
      (INT 1 )
      (INT 5 ))
    (INT 2 )))
(STRUCT "sum_with_at" (INT 3 ))
0
```

The untouched line went from `3` to a half-evaluated residue. Exit 0, no diagnostic.

So the result **type** of `.content()` is decided by how many children the node happens to
have, not by anything visible in the expression. The collapse saves typing precisely in the
cases where it will later bite: where a node genuinely cannot grow the collapse is free,
where it might grow it is a trap, and both get the same spelling.

**Why this matters more here than it would elsewhere.** The one operation this language
declares safe is appending — *you don't erase a scribble*. But appending silently breaks
every reader that assumed a singleton. The safe operation is not safe. This is not a
corner case; growing a node is the designed workflow.

**`.at(0)` is therefore not ceremony — it is a prediction.** Writing it asserts "this node
may grow"; omitting it asserts "it never will". Nothing checks either. In
`you_dont_erase_a_scribble.ceps` the two are used inconsistently: no `.at(0)` on `a` and
`b` (leaves), `.at(0)` on `idea_refinement` (a singleton *today*).

### The lineage: a correction that never travelled

XPath is an acknowledged inspiration for the nodeset feature (author, 2026-10-09) — but
**no XPath implementation was ever used in ceps**. The influence is experiential: the
author built XSLT/XPath reporting in 2005 and carried the data-model intuition forward.
Nothing was ported; nothing conforms. That matters for the fix, so the chronology is worth
stating:

- **1999** XPath 1.0 — node-sets; >1 item where 1 is expected silently yields the first.
- **2005** the author uses 1.0 in production. 2.0 is still a draft. The absorbed intuition
  is 1.0's.
- **2007** XPath 2.0 reaches Recommendation: sequences replace node-sets and silent
  atomization of a >1 sequence becomes a **type error**. He has left XML work by then.
- **2015→** ceps rebuilds the model from that decade-old intuition.

| | XPath 1.0 (absorbed 2005) | XPath 2.0 (2007, never encountered) | ceps today |
|---|---|---|---|
| container | node-set: nodes only, document order, deduplicated | sequence: any items, ordered, duplicates kept | **ordered, duplicates kept, holds atomic items** (confirmed by author) |
| name | node-set | sequence | `NODESET` |
| >1 item where 1 expected | silently takes the first node | **error (`XPTY0004`)** | silently produces a residue, exit 0 |

So ceps arrived **independently** at something close to 2.0's data model — the printer
output above is the evidence, `(NODESET "" (INT 1) (INT 5))` holds atomic values, which a
1.0 node-set could not — while carrying 1.0's **error discipline**, because that is the
part that was in memory. The name is a 1999 label on a 2007-shaped object.

**The consequence for the fix.** This is not a half-finished migration; no migration was
begun. It is the same end state reached independently, missing the one correction that made
that end state safe. The remedy below is therefore still a path a standards body already
walked with a decade of field reports behind it — which is the whole reason to prefer it
over inventing something.

**And ceps's divergence makes the case stronger, not weaker.** XPath queries a document it
does not own and cannot change, so cardinality holds still during evaluation. ceps queries
a document that is also the program, also being rewritten, and whose growth is the point.
The hazard is strictly worse here than in the ancestor.

*(XPath claims above are asserted from knowledge and are listed in the paper's
`## Citations to verify`. Check `XPTY0004` and the 1.0 `string(node-set)` first-node rule
against the specs before either is cited.)*

### The narrow half: `content(n)` discards its argument

```
a{10;20;30;};
p0{root.a.content(0);};  p1{root.a.content(1);};  pall{root.a.content();};
```

```
$ ceps --pe d18.ceps ; echo $?
(STRUCT "p0"   (INT 10 ) (INT 20 ) (INT 30 ))
(STRUCT "p1"   (INT 10 ) (INT 20 ) (INT 30 ))
(STRUCT "pall" (INT 10 ) (INT 20 ) (INT 30 ))
0
```

`content(0)`, `content(1)` and `content()` are indistinguishable. It matters because
`content(n)` is the spelling a newcomer reaches for — not least because `.at(n)` is
undocumented — and the consequence surfaces as a wrong number rather than an error.

### What a fix looks like

**Keep the collapse.** Removing it would put `.at(0)` in front of every value access in the
first five minutes of the language, and the frictionless scribble is the language's
identity.

1. **`demand_scalar`.** Any operation requiring a single value, handed a sequence of length
   ≠ 1, is a diagnostic naming the node and the length. One predicate, applied at every
   scalar-demanding site: arithmetic and comparison operators, `text()`, `as_identifier()`,
   and anything else that atomises. This is XPath 2.0's rule. With it, the collapse becomes
   safe — its assumption is checked at the point of use instead of assumed forever, and the
   failure arrives at the moment of the append, when it can still be acted on.
2. **Keep the residue; add the escalation.** Do not abort. `(+ (NODESET 1 5) 2)` shows
   exactly where evaluation stopped and is genuinely informative — it is how this was found.
   Emit it *and* a diagnostic *and* a nonzero exit. The trail property survives; only the
   silence dies. Partial evaluation is a result; saying nothing about it is the bug.
3. **Reject `content(n)`** at evaluation, with a message naming the arity and pointing at
   `.at(n)`. Aliasing it to `content().at(n)` is the other option and is probably worse —
   two spellings for one operation. Discarding the argument is the only option that should
   be off the table. And **document `.at(n)`**, which is the actual root cause of anyone
   reaching for `content(n)`.
4. **Consider renaming `NODESET` to `SEQUENCE`** in the printer. Cosmetic, but the name is
   currently a 1.0 label on a 2.0 object and it mis-sets expectations about ordering and
   duplicates. Cheap to do alongside the above.

Item 1 is the one that matters. 2 is what keeps the fix in character. 3 and 4 are tidying.

**Why this is worth doing before the paper.** Today, appending to a scribble produces a
silent wrong answer: the evaluator conceals what it did not do. That is the paper's thesis
reproduced inside the tool that argues it. One evaluator change and ceps practises what it
preaches — and that sentence is worth more in a paper than the defect costs to fix.

---

<a name="d19"></a>
## D19. `.children()` yields the empty sequence instead of reporting an unknown accessor

**Severity:** High — silent, exit code 0. A loop over it simply does not run.
**Area:** Evaluator / model traversal.
**Affects:** any traversal written against a plausible-but-wrong accessor name. The
traversal vocabulary is undocumented ([D12](#d12)), so guessing is the normal case.

### Reproducer

```
a{1;2;3;};
b{ for (elem: root.a.children()){ hit{elem;}; } };
```

```
$ ceps d19.ceps --ppe --format raw ; echo $?
a{
1 2 3}
b{
}
0
```

The body never executes and `b{}` renders as empty — indistinguishable from a correct
traversal over an empty node. With `root.a.content()` the same model yields three `hit{}`
nodes.

A related, milder case: `for (elem: root.a)` binds `elem` to the whole node and runs
exactly once, which may be intended but is silently different from `.content()`.

### What a fix looks like

An unknown member on a node is a diagnosable condition and should be diagnosed. Where the
project would rather not fail, this is the first and best candidate for the `nonhitter{}`
treatment: leave the unresolved accessor in place in the output instead of yielding
nothing, so the artifact records that the traversal was not understood.

---

<a name="d20"></a>
## D20. The evaluated document is not a ceps document — `--ppe`/`--pe` output does not re-parse

**Severity:** High — it blocks the language's own round trip, and with it any incremental
or staged use of the evaluator.
**Area:** Printer / output format.
**Affects:** `--pe`, `--ppe` with `--format raw`; everything downstream that would consume
an evaluated model as a model.

### Reproducer

```
$ printf 'a{1;2;3;};\n' > d20.ceps
$ ceps d20.ceps --ppe --format raw > d20.out ; cat d20.out
a{
1 2 3}
$ ceps d20.out --ppe --format raw ; echo $?
***Error near line 1, column 1:
syntax error
***Fatal Error:A parser exception occured in 'd20.out'.
1
```

The printer drops the inner `;` separators and the trailing `};`:

| | text |
|---|---|
| accepted as input | `a{1;2;3;};` |
| printed after evaluation | `a{1 2 3}` |

Confirmed against the parser directly — `a{1 2 3};`, `a{1 2 3}` and the two-line printed
form are all rejected at column 2; only `a{1;2;3;};` is accepted.

### Why it matters

Models are routinely composed on the command line — `ceps A.ceps B.ceps C.ceps` — and
later files read earlier files' *evaluated* results. That works. What cannot be done is
the staged form:

```
ceps A.ceps B.ceps --ppe > AB.ceps     # fine
ceps AB.ceps C.ceps                    # parse error
```

So evaluation cannot be checkpointed, results cannot be cached, and the natural law
`eval(A · B · C) = eval(eval(A · B) · C)` cannot be tested, let alone relied on.

### What a fix looks like

Emit separators. This is a printer change, not a semantics change — the evaluator already
holds the structure. A `--format ceps` that is guaranteed to re-parse, with a round-trip
test `eval(print(eval(x))) == eval(x)` in the suite, would close it.

---

<a name="d21"></a>
## D21. Forty-six of the sixty-three command-line flags are undocumented

**Severity:** High as documentation, higher as an opportunity — `--cppgen` alone is a
working C++ code generator that `--help` does not mention.
**Area:** CLI / docs.
**Affects:** discoverability of most of the tool. The same pattern as [D12](#d12) (the
model-traversal layer missing from `SKILL.md`) and [D8](#d8) (flags accepted but inert),
here measured across the whole interface.

### Reproducer

```
$ grep -o '"--[a-z_0-9]*"' core/src/cmdline_utils.cpp | tr -d '"' | sort -u > parsed
$ ceps --help | grep -o '\-\-[a-z_0-9]*' | sort -u > documented
$ wc -l < parsed ; wc -l < documented
63
19
$ comm -23 parsed documented | wc -l
46
```

`--help` itself is among the forty-six.

### The ones that matter

| flag | what it is |
|---|---|
| `--cppgen`, `--cppgen_statemachines`, `--cppgen_ignore_print` | the sm4ceps C++ generator |
| `--monitor`, `--run_as_monitor`, `--live_log` | runtime monitoring |
| `--ws_api`, `--port`, `--sleep_before_ws_api` | a WebSocket API |
| `--print_transition_tables` / `--ptt`, `--print_statemachines`, `--print_event_signatures`, `--print_signal_generators` | model introspection |
| `--print_raw_input_tree`, `--print_evaluated_input_tree`, `--print_evaluated_postprocessing_tree` | the pipeline stages, individually dumpable |
| `--dump_asciidoc_can_layer`, `--dump_stddoc_canlayer` | the docgen subsystem |
| `--plugin`, `--package_file`, `--push_dir`, `--pre`, `--post_processing` | extension points |
| `--enforce_native`, `--vcan_api`, `--rmip`, `--rmport` | target and transport control |

`--cppgen` is not vestigial. It works:

```
$ ceps test/native_main_loop/timer_b.ceps --cppgen ; echo $?
S1.a1();
S1.Initial- S1.A+
…
0
$ head -8 out.hpp
/* out.hpp
   CREATED Fri Oct  9 00:55:41 2026
   GENERATED BY THE sm4ceps C++ GENERATOR VERSION 0.90.
   BASED ON cepS … VERSION 1.1 (Jan 13 2026) …
   Input files:
      …/test/native_main_loop/timer_b.ceps
```

There are thirteen fixtures for it under `test/native_main_loop/`, with checked-in
`out.hpp`/`out.cpp`. A whole compilation path, exercised by tests, invisible from the
command line.

### Why it matters

The project's stated difficulty is adoption, and the three documented symptoms —
[D8](#d8), [D12](#d12) and this one — are the same shape: capability present, surface
absent. Someone evaluating ceps reads `--help`, sees nineteen flags, and concludes it is a
small simulator. The generator that produced production ARM code in 2016 is two lines of
`--help` away from being visible.

Generating the flag list from the parser rather than maintaining it by hand would close
this permanently and is a smaller change than writing the missing nineteen lines.

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

<a name="d22"></a>
## D22. `--pe`/`--ppe` print a macro as a raw heap address and omit its body

**Severity:** High.
**Area:** Printer.
**Affects:** every workflow that compares two runs — documentation checking, regression
fixtures, the trace-comparison demo. Found 2026-10-09 while writing
`doc/scribble-concept/README.md`, which needed a macro example whose output was stable
enough to commit.

### Reproducer

```
$ cat m.ceps
macro greet{ hello{1;}; };

$ ceps --pe m.ceps
(MACRO "greet" 0x637924972ae8
)
$ ceps --pe m.ceps
(MACRO "greet" 0x55b3408d0ae8
)
```

Same input, same binary, two different documents. The address is a heap pointer, so it
moves with ASLR on every process start. `--ppe` behaves identically; both reach the same
printer.

Two faults, not one. The second is the worse of the two:

```
$ cat m2.ceps
macro big{ alpha{1;}; beta{2;}; gamma{"xyzzy";}; };
plain{ alpha{1;}; beta{2;}; };

$ ceps --pe m2.ceps
(MACRO "big" 0x5d77ef415ae8
)
(STRUCT "plain"
  (STRUCT "alpha"
    (INT 1  )
  )
  (STRUCT "beta"
    (INT 2  )
  )
)
```

`plain` prints its whole body. `big` prints a pointer *instead of* its body. Nothing of
`alpha`, `beta` or `gamma` appears. The printed tree does not contain the macro.

### Mechanism

`ceps_ast.hh:892` (in `cepsdev/ceps`):

```cpp
typedef ast_node<Ast_node_kind::macro_definition,
                 std::string /*name*/,
                 Nodebase_ptr /*body*/,
                 std::vector<Nodebase_ptr> /*attributes*/> Macrodef;
```

The body is a **typed member**, not a child. The generic member printer at
`ceps_ast.hh:415-418` is:

```cpp
void print_content(std::ostream& out,bool pretty_print,int indent) const override
{
    out << x << " "; Base::print_content(out,pretty_print,indent);
}
```

`x` here is a `Nodebase*`. `operator<<` on a non-`char` pointer prints the address, and
there is no specialisation that recurses into a `Nodebase_ptr` member. So the body is
rendered as its own location in memory. Meanwhile `print_content_helper`
(`ceps_ast.hh:361-383`) walks `children()` — and the body is not among them, so the
recursive walk never reaches it either.

This is not a macro-specific slip. It is a hole in the printer's type dispatch, and it has
two more mouths:

| member type | typedef | printed as |
|---|---|---|
| `Nodebase_ptr` | `Macrodef` body (`:892`) | heap address; subtree lost |
| `void*` | `Label` symbol entry (`:893`) | address; `0` while unbound — latent, same class |
| `std::vector<Nodebase_ptr>` | `Macrodef` attributes (`:892`) | **nothing at all** (`:443-446` drops `x` silently) |

The vector case is the quietest of the three: its `print_content` forwards to the base and
never touches `x`, so macro attributes leave no trace in the output whatsoever — no
address, no placeholder, no body.

```
$ cat lab.ceps
label l;

$ ceps --pe lab.ceps
(LABEL "l" 0
)
```

### Why High and not Critical

It does not corrupt a computation, only the record of one. [D20](#d20) already establishes
that `--pe` output does not re-parse, so no downstream stage consumes it; the blast radius
stops at humans and at diffs. That is the boundary, and it is the only thing holding this
below Critical.

Within those bounds it is the printer family's strongest instance of the house pattern
(see Triage, *One diagnosis, repeated*). Both halves fail silently green: the address
prints without comment, the dropped body prints without comment, exit code 0. Someone
comparing two runs sees a difference that is not there; someone reading one run sees a
macro that appears to be empty. A third outcome — `(MACRO "greet" <body-not-printed>)`, or
simply printing the body — would cost one specialisation.

It also compounds [D20](#d20) rather than merely resembling it. D20 says the output is not
a ceps document. D22 says that for macros it is not even a lossless *record* of one, so
no amount of work on the grammar at the D20 end would recover the body.

### What a fix looks like

1. Specialise the printer for a `Nodebase_ptr` member so it recurses, exactly as the
   `children()` walk does. This fixes the address and the omission in one move, and is
   the whole of the defect as filed.
2. Specialise for `std::vector<Nodebase_ptr>` so attributes are printed instead of
   dropped.
3. Never print a raw pointer. If a member cannot be rendered, emit a marker that says so.
   An address in output is an ASLR oracle as well as noise.
4. Add a round-trip check to the test harness: `ceps --pe f.ceps` twice, `diff` the two,
   require them identical. That is a three-line test that would have caught this at the
   moment it was introduced, and it generalises to every future printer change.

Until then, anything diffing ceps output must mask `0x[0-9a-f]+`.
`tools/check-doc-examples.py` does.

<a name="d23"></a>
## D23. A macro used without parentheses is silently not expanded

**Severity:** Medium.
**Area:** Evaluator / name resolution.
**Affects:** any macro invoked as `name;` rather than `name();`. Found 2026-10-09 while
probing [D22](#d22).

**This entry replaces an earlier one** titled *"Macro expansion keeps only the first
statement of the body, and strips its wrapper"*. That filing was wrong. Its probes named
the macro `m`, which is the SI unit **metre**, so the use site was shadowed by
[D17](#d17) and the resulting `(INT 1 m^1 )` was misread as a truncated body. Re-run with
a name that is not a unit, multi-statement expansion is correct. The correction is kept
visible rather than deleted, and the incident is recorded in D17 as evidence of how well
that defect hides.

### Reproducer

Two files differing only in `()`.

```
$ cat t1.ceps
macro two_things{ alpha{1;}; beta{2;}; };
holder{ two_things; };

$ ceps --pe t1.ceps ; echo $?
(MACRO "two_things" 0x5f1736e2aae8
)
(STRUCT "holder"
  (ID "two_things"
  )
)
0
```

```
$ cat t2.ceps
macro two_things{ alpha{1;}; beta{2;}; };
holder{ two_things(); };

$ ceps --pe t2.ceps ; echo $?
(MACRO "two_things" 0x6176947e3ae8
)
(STRUCT "holder"
  (STRUCT "alpha"
    (INT 1  )
  )
  (STRUCT "beta"
    (INT 2  )
  )
)
0
```

With parentheses the expansion is correct and complete — both statements, wrappers
intact. Without them the macro is not expanded; `two_things` survives into the output as
a bare `(ID "two_things")`, and nothing is said about it.

(The `0x…` addresses are [D22](#d22).)

### What is actually wrong

Not the expansion. The expansion is fine. What is wrong is the **non-answer**: an
identifier that names a macro in scope is left unresolved and emitted as an `(ID ...)`
node, with exit code 0 and no diagnostic.

Whether `name;` *ought* to expand is a language-design question and this entry does not
prejudge it. Either answer is defensible:

- if bare reference should expand, this is a missing case in resolution;
- if a macro must be called, then `two_things;` is a reference to an undefined identifier
  and belongs in the same class as any other unbound name.

What is not defensible is the third thing it currently does, which is to quietly produce a
document containing the identifier itself. That is the catalogue's house pattern again:
the tool did not do the thing, and did not say so.

It is rated Medium rather than High because the correct spelling is available, is the one
the documentation uses, and works completely. The cost is a silent wrong document for
anyone who guesses the other spelling — which, since `name;` is exactly how a *path* is
spliced in ceps (see `doc/scribble-concept/README.md`, the `root.lets_try_the_idea.sm;`
line), is an easy guess to make. The two syntaxes look alike and behave differently.

### What a fix looks like

Decide whether bare macro reference expands. Then make the other case a diagnostic. In
either case an identifier that reaches output unresolved should be an error, not a node —
which is the same representable-third-outcome fix the Triage section argues for across
the whole catalogue.

---

<a name="d24"></a>
## D24. The serializer generator addresses opcodes by string, discarding the type machinery

**Severity:** Medium — loud when it fires, but deferred to assembly time and conditional
on the generated path being reached. It is explicitly **not** in the silent class; the
assembler throws and names the mnemonic.
**Area:** Oblectamenta VM / serialization.
**Files:** `core/src/vm/oblectamenta-assembler.cpp`, `core/include/vm/vm_base.hpp`.

Hand-written emission is checked at compile time. `emit` takes the opcode as a **template
parameter** and overloads on operand shape (`core/include/vm/vm_base.hpp:584-617`):

```cpp
template<Opcode opcode> size_t emit(VMEnv&, size_t pos);
template<Opcode opcode> size_t emit(VMEnv&, size_t pos, size_t v);
template<Opcode opcode> size_t emit(VMEnv&, size_t pos, double v);
template<Opcode opcode> size_t emit(VMEnv&, size_t pos, VMEnv::reg_t, VMEnv::reg_offs_t);
```

A misspelled opcode is not a name at all, and an operand list of the wrong shape fails
overload resolution. Neither error can survive compilation. This is the best thing in the
VM and it is worth saying so before describing where it stops.

It stops at the generator. `oblectamenta-assembler.cpp:217-230` reintroduces the opcode as
a `std::string`:

```cpp
static ceps::ast::node_t gen_mnemonic(std::string name){
    return mk_symbol(name, "OblectamentaOpcode");
}
```

and roughly a thousand lines of generator — `oblectamenta_assembler_preproccess` (`:270`),
`oblectamenta_assembler_preproccess_match` (`:640`), `gen_expr` (`:1080`) — are written
against string literals: `em(r,"ldsi32")`, `em(r,"ldi32",msg_node::INT32)`,
`emwsa(r,"bneq",lbl,"OblectamentaCodeLabel")`. These are the densest emission sites in the
project, and they are the only ones with no compile-time check.

### Observed

A typo reaches the lookup at `oblectamenta-assembler.cpp:1434`, which is at least honest
about it:

```cpp
auto it{mnemonics.find(mnemonic)};
if (it == mnemonics.end())
 throw std::string{"oblectamenta_assembler: unknown opcode: '"+ mnemonic+"'" };
```

So the failure is loud. The costs are the two that remain after that:

1. **It is deferred and conditional.** The throw fires when that generated instruction is
   assembled, which happens only for message shapes a run actually builds. A mistyped
   mnemonic on a rarely generated branch — an error path, say, which is precisely where
   the four `bneq` sites live — sits dormant until the day it is needed.
2. **The operand-shape check is lost entirely.** The string path selects an emitter from
   the `mnemonics` tuple by argument form at run time (`MNEM_NOARG` … `MNEM_IMM_IMM`,
   `vm_base.hpp:623-627`), and a slot holding `nullptr` is simply not called. The
   compile-time guarantee that `buc` cannot take a `double` does not exist here.

### What a fix looks like

Make the generator use the same mechanism as every other emitter. The minimum is a
template overload set mirroring `emit`:

```cpp
template<Opcode op> ceps::ast::node_t gen_mnemonic();
template<Opcode op> ceps::ast::node_t gen_mnemonic(int v);
```

with the mnemonic string derived from the opcode rather than written beside it. That
requires one thing the project does not yet have: a single list from which the enumerator
*and* its spelling are generated — the X-macro construction proposed in the
[note on D1](#d1-note). The same list would close D1's declaration gap and the positional
coupling between the enum and `op_dispatch`.

This entry is rated Medium rather than Low because of where the unchecked code is. The
generator is not peripheral; it writes the serializers and deserializers for every
message the VM handles, and it is the one part of the emission layer that cannot be
checked by reading it, because the correctness of a thousand string literals is not
something reading establishes.
