# Inventory

A repository-wide sweep, run on 8 October 2026 against `bin/ceps` version 0.8.1.3.3
(built Aug 27 2026). Every `.ceps` file outside `neural_networks/` was executed with a
ten-second timeout and its exit code recorded.

The purpose is a **baseline**: a statement of what exists, what still runs, and what is
implemented but undocumented, specific enough that the same sweep can be repeated later
and diffed.

| | |
|---|---|
| Files executed | 370 |
| Areas under `test/` | 37 |
| Run scripts found | 41 |
| Run scripts whose binary path resolves | **2** |

---

## 1. Results

| Outcome | Files | |
|---|---:|---|
| Ran to completion | 221 | 59% |
| Fatal or parse error | 123 | 33% |
| Timed out at 10s | 20 | 5% |
| Crashed (SIGSEGV / SIGABRT) | 6 | 2% |

**That 59% understates the tree, and the error is in the method.** Three corrections
apply, and they are recorded here rather than quietly fixed because anyone repeating the
sweep will hit them again:

1. **Fragments are not entry points.** Many models are composed on the command line:

   ```
   ceps common.ceps frames.ceps receiver.ceps receiver_node.ceps
   ```

   Run alone, `receiver.ceps` fails at line 1 because the kinds it uses are declared in
   `common.ceps`. Of the 29 parse errors, most are this. Re-run composed,
   `doc/ceps/can_comm/example_1`, `example_4`, `doc/vcan_api/two_nodes_no_hub` and
   `examples/distributed/can_frames/led` all behave correctly.

2. **A timeout is often the correct behaviour.** A receiver waiting on a bus, a periodic
   timer and a websocket server are all supposed to keep running. Of the 20 timeouts,
   most are servers doing their job.

3. **Some failures are missing plugins, not broken models.** The 14 `… not defined`
   errors are the Gherkin feature suite, whose runner still reads
   `--pluginlibINSERT_PLUGIN_NAME_HERE.so`.

After these corrections the genuine regressions are a small number of defects with a wide
blast radius, recorded as [D13–D16](DEFECTS.md) and summarised in §3.

---

## 2. The harness is the largest single problem

**39 of 41 run scripts invoke a binary that does not exist.**

| Path referenced | Scripts |
|---|---|
| `../../x86/ceps` and deeper variants | 14 |
| `../eclipse/Debug/statemachines` | 24 |
| `../../x86/sm` | 1 |

Meanwhile `ceps` is on `PATH` and is byte-identical to `bin/ceps`. Nothing rotted here:
the binary moved and the scripts were never updated. The newer scripts already use the
right form — `vm/features/serialization/run` calls `ceps` directly,
`examples/from_spec_to_test/run` calls `../../bin/ceps`.

This is the highest-leverage repair in the repository. It is mechanical, it is one
pattern, and until it is done no amount of working code can be *demonstrated* to work.

### Golden files

Seven `expected*` files exist tree-wide, all under `test/native_main_loop/` and
`test/livelog/`. `test/native_main_loop/run.sh` is the pattern worth propagating — run,
capture, `diff` against the expected log, print a coloured verdict — but the better model
is newer: **`vm/features/` is one file per behaviour, named after the behaviour, with a
`run` script**, and all ten of its serialization cases pass. Promote that layout rather
than inventing one.

---

## 3. Regressions found

Detailed in `DEFECTS.md` with reproducers.

| Defect | Blast radius |
|---|---|
| [D13](DEFECTS.md#d13) — named arguments in a call rejected | 18 files; all of `test/jenkins/` and `test/mms_devops/`, plus the FIBEX simulations |
| [D14](DEFECTS.md#d14) — current-state set empty in assertions | every `Simulation{}` using state assertions; the negative form passes vacuously |
| [D15](DEFECTS.md#d15) — assembler rejects admissible guards | 6 files; `in_state(M.S)` and `abs()`-based float comparisons |
| [D16](DEFECTS.md#d16) — SIGSEGV on `receiver` + `canbus` without frames | 4 documented examples; reproducible in four lines |

**Retracted.** An earlier reading of the sweep counted 14 files failing with
`Receiver definition: on_msg : frame_id unknown` as a fifth regression. They are not:
composed with the file that defines the frames, they run. That was correction (1) above.

---

## 4. Implemented and working, documented nowhere

This is the part of the sweep that matters most. Each item below was verified to run, or
read in the source, during the audit.

### 4.1 `.ceps.lex` — partial parsers for foreign notations

Six lexer specifications exist:

| File | Lines | Ingests |
|---|---:|---|
| `vm/features/common/gherkin.ceps.lex` | 64 | Gherkin (Given/When/Then) |
| `import-scripts/dbc/dbc.ceps.lex` | 113 | Vector DBC (CAN databases) |
| `doc/cloud/ctrlpanel/dbc.ceps.lex` | 111 | DBC |
| `test/cepslexer/dbc.ceps.lex` | 105 | DBC |
| `vm/statistical-learning/data.ceps.lex` | 22 | training data |
| `examples/analyze_logs/log.ceps.lex` | 20 | log files |

The format is a transducer, not a scanner. Named sub-lexers invoked with `call`,
`rewind` for backtracking, `exit` to return, positional captures `$0`, `$2`, … and a
right-hand side that emits **ceps AST directly** rather than tokens:

```
lexer read_then{
 ident is equal to a VM formed of ident,ident,ident,ident =>  verdict{equality_test{
            $0;
            vm{ $8; $10; $12; $14; };
        };};.
 \.  =>   exit .
 any  =>  .
}
```

Concrete-syntax-to-AST transduction is not new — TXL (Cordy, *Science of Computer
Programming*, 2006, doi:10.1016/j.scico.2006.04.002), ASF+SDF, Stratego/XT and Racket's
`#lang` all do it. What is unusual here is the last line of that block. **`any => .`
makes it a partial parser**: no grammar for Gherkin was ever written, only the five
phrases that mattered, and everything else falls on the floor. Transformation systems
cannot do this because transformation demands a total grammar; extraction does not.

That property is what lets ceps meet another party in *their* notation — the `.dbc`
they already have, the `.feature` they already wrote — which is the condition
`POSITIONING.md` is built around.

**Documentation:** none. The only file in the repository that mentions the format is
`RECURSIVE-STATE-MACHINES.md`, which was written during this audit.

**Known gap:** `msg{read}` has `onerror`; `.ceps.lex` has `else => rewind exit`, but that
is control flow, not a diagnostic. A sentence that nearly matches is indistinguishable
from one deliberately ignored, so a typo in a `.feature` file silently removes a scenario.
Partiality needs a way to be loud; this layer does not have one.

### 4.2 `core/src/modelling/` — model-based test generation

**This is the largest single omission in the audit, and it was missed twice**: the sweep
ran `.ceps` files, and this layer is C++; and the two example models that exercise it both
fail to parse when run on their own, so they looked like ordinary casualties.

Three files, ~7 KB, dated March 2024, registered as ceps constructs at
`core/src/state_machine_simulation_core.cpp:1330` and `:1332`:

| Construct | Source | What it does |
|---|---|---|
| `partition{}` | `partitions.cpp` | declares equivalence classes over a system state; **generates** a state machine whose states are the classes and whose edges are the class-to-class transitions, carrying a `cover{}` obligation |
| `cover_path{}` | `cover_path.cpp` | declares a sequence of states; **generates** an observer that reaches `Final` iff that sequence occurs as a subsequence of the run |
| `sm(...)` | `gensm.cpp` | the small builder both use to emit `Statemachine{}` AST |

Together with `signal{}` / `start_signal()` (`core/src/signalgenerator.cpp`, registered at
`state_machine_simulation_core_buildsms.cpp:954`) this is a complete model-based testing
loop: **partition the input domain, generate the coverage automaton, generate the
stimulus, measure.**

#### Worked example — it runs today

`examples/doing_specs/lueftersteuerung/` (fan control). The specification is five lines:

```
Systemstate motor_temperatur;

partition{
 id = vp_motor_temperatur;
 {motor_temperatur <= 1.0;                           niedrig;};
 {motor_temperatur > 2.0 && motor_temperatur <= 3.0; mittel; };
 {motor_temperatur > 3.0;                            hoch;   };
};
```

The test is seven:

```
signal{
 id = signal_1;
 delta_t = 0.1*s;
 values{ for(i : 1 .. 3 j : 1 .. 10 ) { i+(j-1)/10.0; } };
};

Simulation{
 start_signal(signal_1,motor_temperatur);
 timeout{2.0*s;};
 motor_temperatur = 1.0;
};
```

```
$ ceps .ceps/prelude.ceps spec/main.ceps tests/monotonically_increasing_temperature_1.ceps
partition_sm_vp_motor_temperatur+ partition_sm_vp_motor_temperatur.Initial+
partition_sm_vp_motor_temperatur.Initial- partition_sm_vp_motor_temperatur.niedrig+
partition_sm_vp_motor_temperatur.mittel+ partition_sm_vp_motor_temperatur.niedrig-
State Coverage: 0.75 ( 75% )
Transition Coverage: 0.166667 ( 16.6667% )
```

No machine was written. The four states and twelve edges are derived from the three range
predicates; the ramp is derived from a comprehension; the coverage figure is the answer to
*"did this stimulus exercise the classification?"* — and the honest answer is no. The
`timeout{2.0*s;}` cuts the 3-second ramp short, so `hoch` is never entered. **The example
demonstrates a gap being detected, which is the right thing for an example to do.**

#### Partial partitions

The partition above has a hole: nothing classifies `(1.0, 2.0]`. Nine of the thirty
samples fall in it. The run shows no transition there — the machine stays in `niedrig`
until the value reaches `mittel`. **An unclassified region is not an error.** You name the
classes you care about and the rest is simply not a boundary. That is the same rule as
partial programs, shadow states, `onerror`, bit patterns and `any => .`, now at a sixth
layer — test design. See [POSITIONING.md](POSITIONING.md), *The invariant*.

#### Status

- `partition{}` — **works.** Verified 8 October 2026.
- `cover_path{}` — **broken.** It generates `in_state(Vehicle.Standstill)` guards, which
  is exactly [D15](DEFECTS.md#d15). The one example in the repository,
  `examples/doing_specs/vehicle_navigation/examples_of_operations/stored_data_invalid_gps_alignment.ceps`,
  dies in the Oblectamenta assembler. D15 does not merely affect six files; **it disables
  one of the two modelling constructs outright.**
- Documented in neither `SKILL.md`, `QUICK-START-UML-WITH-CEPS.md` nor `README.md` — the
  words `partition`, `cover_path`, `signal`, `start_signal` and `macro` do not occur in
  any of the three.

#### Two further conventions found here

**`macro`.** `macro timeout { start_timer(hd(arglist),EXIT);};` — macros with an
`arglist` and list primitives. Verified to parse and run. Undocumented.

**`.ceps/prelude.ceps`.** Both `doing_specs` projects carry a hidden `.ceps/` directory
holding the declarations every file in the project assumes (`kind Systemstate; kind Event;
kind Guard;` plus shared macros). This is the convention that answers "where did the
built-in kinds go". **It is not auto-loaded** — nothing in the binary looks for it; it has
to be passed as the first input file, as `test/fibex/*.sh` does. Making the lookup
automatic is a small change with a large effect on first-run experience, since a model
that omits it fails with a bare `syntax error` at the first declaration.

### 4.3 `--cppgen` — the state-machine to C++ compiler

Still works. Verified during the sweep:

```
ceps substatemachines_basic_a.ceps --cppgen --ignore_simulations
→ out.cpp (7810 bytes), out.hpp (3600 bytes)
```

The generated header identifies itself as *"sm4ceps C++ GENERATOR VERSION 0.90"*. This is
the compiler written for the ARMv7 target when the simulator proved too slow — the same
move available to any ceps model that needs to leave the interpreter.

### 4.4 Message definition and serialization

`vm/features/serialization/` — ten cases, Apache-2.0, 2025, **all ten pass**. A
message-definition DSL embedded in Oblectamenta assembly: nested sub-messages, typed field
tags (`i32`, `i64`, `f64`, `sz`), arbitrary computation inside a field, and an `onerror`
handler for fields that are not present.

The case names are the schema-evolution problems — read a non-existent field, read fields
in the wrong order, read and write empty messages, substructs. Fields are keyed **by
name**, length-prefixed and type-tagged (`msg_node` in `core/include/fast-json.hpp`);
there are no field numbers, no varints and no `.proto`.

**Documentation:** `doc/tutorial/serialization/README.md`, 5.7 KB, and good. It is linked
from nothing. `ideas/README.md` is the design note that preceded the implementation.

Also: `make_byte_sequence` / `breakup_byte_sequence` with bit-field patterns and `any`
wildcards — 146 lines of assertions in `test/make_and_break_byte_sequences/test.ceps`.

### 4.5 Shadow states — conformance checking

`core/src/sm_sim_core_shadow_states.cpp`, 114 lines, 2017. Nine models in
`test/shadow_states/`, of which nine of ten run. `compute_shadow_transitions()` is a
static check that the shadow map is a **simulation relation**: an implementation
transition between two shadowed states must have a compatible concept transition, or the
run aborts naming both states, the event and the requirement. The diagnostic is the best
in the codebase.

`test/shadow_states/description.txt` sketches a richer surface — `extend{}`,
`implement{}`, `where{}`, `path{}` — that was never built.

### 4.6 Declarative checking

- `test/alloy/` — a classic Alloy problem solved in ceps using `rule{}` and a
  `symbolic_equality` primitive that returns a structured difference.
- `test/agda/insertion_sort.ceps` — execution traces compared symbolically.

### 4.7 Model traversal

`root.sm`, `.content()`, `.at(n)`, `.symbol()`, `.sort()`, `.unique()`, `.is_struct()`,
`.fetch_recursively_symbols()`, `predecessor()`, `for (x : nodeset)`. Recorded as
[D12](DEFECTS.md#d12); the reference is [cepsdev/mermaid](https://github.com/cepsdev/mermaid).

### 4.8 Automatic differentiation

`vm/test/plugin-entrypoint.cpp` implements forward-mode (`tangent_forward_diff`) and
reverse-mode (`backpropagation`) differentiation of computation graphs, emitting
Oblectamenta assembly. Documented only by a generated summary in `vm/test/README.md`.

### 4.9 Gherkin feature suite

`vm/features/machinelanguage/` — a BDD suite whose scenarios are written in Gherkin,
parsed by `.ceps.lex`, and whose results are emitted as markdown
(`arithmetic.result.md`, `control.result.md`, …). The runner needs its plugin name filled
in before it will execute.

### 4.10 The command line is three quarters undocumented

`core/src/cmdline_utils.cpp` parses **54** flags. `--help` lists **19**. The 40 that are
accepted but unlisted include the whole of the generator and inspection surface:

```
--cppgen  --cppgen_statemachines  --print_event_signatures  --print_transition_tables
--print_statemachines  --print_signal_generators  --print_raw_input_tree
--print_evaluated_input_tree  --run_as_monitor  --live_log  --logtrace  --vcan_api
--ws_api  --dump_asciidoc_can_layer  --dump_stddoc_canlayer  --report_format_*
--package_file  --post_processing  --start_paused  --no_file_output  --enforce_native …
```

`--cppgen` is the sharpest case: it is the compiler, it still works, and a user reading
`--help` has no way to learn it exists. Note that three of the listed flags are inert
rather than merely undocumented — [D8](DEFECTS.md#d8).

Reproduce:

```sh
grep -oE 'arg == "--[a-z_0-9]+"' core/src/cmdline_utils.cpp | sed 's/.*"\(--[a-z_0-9]*\)"/\1/' | sort -u > /tmp/parsed
bin/ceps --help | grep -oE '^\s+--[a-z_0-9-]+' | tr -d ' ' | sort -u > /tmp/helped
comm -23 /tmp/parsed /tmp/helped | wc -l
```

### 4.11 Other, unexamined

`utils/fibex_import.cpp` (467 lines), `utils/can_layer_docgen.cpp` (492),
`utils/stddoc.cpp` (423), `utils/concept_dependency_graph.cpp` (76),
`test/save_all_and_load_all`, `test/restart_state_machine`, `test/trees`,
`test/structures`, `test/mini-slac`, `algorithms/heapsort`, `vm/statistical-learning`,
`vm/sm-actions`, `examples/basic-micro-service`.

---

## 5. Per-area status

Counts are files that exited 0 when run individually. Areas marked ✱ contain fragments and
should be judged composed, not alone.

### `test/`

| Area | | Area | |
|---|---|---|---|
| `native_main_loop` | 21/25 | `can` ✱ | 4/9 |
| `shadow_states` | 9/10 | `vcan_api` ✱ | 2/4 |
| `logging` | 8/8 | `fibex` ✱ | 2/7 |
| `reporting` | 8/9 | `sha1` | 2/2 |
| `funny_transitions` | 6/6 | `save_all_and_load_all` | 2/2 |
| `alloy`, `agda`, `trees`, `labels` | 1/1 each | `structures` | 1/2 |
| `cepslexer`, `protobufish`, `env`, `mod` | 1/1 each | `livelog` | 1/2 |
| `phase0_plugin`, `make_and_break_byte_sequences` | 1/1 each | `jenkins` | 0/6 — [D13](DEFECTS.md#d13) |
| `cover_states_with_enter_and_exit_times` | 1/1 | `mms_devops` | 0/6 — [D13](DEFECTS.md#d13) |
| `events_and_simulation_directives` | 1/1 | `guard` | 0/1 — [D15](DEFECTS.md#d15) |
| | | `markdown` | 0/1 — [D15](DEFECTS.md#d15) |

### `vm/`, `examples/`, `doc/`

| Area | | Note |
|---|---|---|
| `vm/features/serialization` | 10/10 | all pass |
| `vm/features/machinelanguage` | — | needs the plugin name |
| `vm/benchmarks`, `vm/sm-actions`, `vm/test` | 1/1 each | |
| `examples/distributed` ✱ | 13/28 | composed, most run |
| `examples/doing_specs` | 9/15 | |
| `examples/first_steps` | 8/14 | includes [D15](DEFECTS.md#d15) |
| `examples/complete_projects` | 4/9 | |
| `doc/ceps` ✱ | 29/40 | includes [D16](DEFECTS.md#d16) |
| `doc/vcan_api` ✱ | 13/16 | includes [D16](DEFECTS.md#d16) |
| `doc/workflow_engine_howto`, `doc/tutorial*`, `doc/import` | all | |

---

## 6. What this says

Four things, in order of how actionable they are.

**The harness, not the code, is what is broken.** 39 dead script paths and seven golden
files across 370 models. The code is in much better condition than the ability to show
that it is.

**The regressions are few and concentrated.** Four defects account for the great majority
of real failures, and two of them — [D13](DEFECTS.md#d13) and [D14](DEFECTS.md#d14) — are
single fixes that between them restore twelve models and the entire assertion mechanism.

**The documentation deficit is not uniform; it is adverse.** What is documented is what
was borrowed — statecharts, an event loop, a bytecode VM. What is undocumented is what was
invented: partial parsing of foreign notations, conformance checking against a simulation
relation, name-keyed message definitions with an error path, generated test obligations, a
model that includes its own execution report. The selection function filtered out
precisely the differentiators.

**A sweep that looks only at one file extension will miss a layer.** §4.2 was found after
this document was first written, by looking at a directory rather than at models. The
pattern it illustrates is worth stating: the capability was invisible from three
directions at once — not a `.ceps` file, so the sweep skipped it; absent from `--help`, so
the CLI did not announce it; and its two examples failed to parse standalone because they
depend on a `.ceps/prelude.ceps` that is not auto-loaded. **Each of the three obstacles is
individually trivial and together they hid a working feature for two years.** Nine other
`core/src/` subdirectories — among them `transform/`, `docgen/` and `cppgenerator/` — and
five `utils/` files have not had the same treatment (§4.11).

## Reproducing this sweep

```sh
find . -name '*.ceps' -not -path "*/neural_networks/*" -not -path "*/bin/*" | sort |
while read f; do
  (cd "$(dirname "$f")" && timeout 10 ceps "$(basename "$f")" >/dev/null 2>&1 </dev/null)
  printf '%s\t%s\n' "$?" "$f"
done
```

Fragments will report failures; see §1. Redirecting `stdin` from `/dev/null` matters,
since several models read from it.
