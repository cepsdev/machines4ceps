# Schedule — 2 Nov to 31 Dec 2026

Execution plan for **L1 + L2 + L3** of [LANG-ROADMAP.md](LANG-ROADMAP.md): grammar and
semantics, loud failure, the flagship. L4 (arena), L5 (conformance beyond the basics) and
L6 (ecosystem) are **out of scope** and are not in this schedule.

Preceded by **Phase 0** (§3), a two-week LR parser generator written in ceps and
Oblectamenta. It is not part of L1–L3 and it is not on the critical path. It is in the
schedule because it is the best available preparation for the semantics chapter, and
because it carries its own hard stop.

**Capacity:** full days, Mon–Fri. **Off:** 24–26 Dec, 31 Dec.
**41 working days — 10 in Phase 0, 31 in the main block.** Weekends are not scheduled —
they are the release valve, and the plan needs about three of them. If you are spending
every weekend by mid-December, the plan has already failed and the cut list in §6 applies.

---

## 1. The one thing that reshaped this plan

`ceps/core/src/grammar/ceps.y` is **790 lines of bison, 26 rules**. The grammar is already
formal and already authoritative — the parser is generated from it.

So L1's grammar half is **transcription and annotation from a source that cannot be wrong**,
not reverse-engineering. Four days, not two weeks.

That moves the weight of L1 onto the half that genuinely does not exist anywhere:
**evaluation semantics — what runs when.** That is the hard, valuable, undocumented thing,
and this schedule gives it five days and the freshest hours of the week.

Second useful find: `static_for` is recognised in `ceps/core/src/cepslexer.cpp:274`, as a
lexer-level keyword. That is also where the [D17](DEFECTS.md#d17) collision lives, so the
fix and the name-resolution section of the reference are the same investigation.

---

## 2. Shape of the day

Thirty-one consecutive full days is a long push. The shape that survives it:

- **Mornings — semantics.** Anything requiring a fresh head: evaluation order, name
  resolution, the parts of the reference a reader will rely on.
- **Afternoons — mechanical.** Transcription, `--help`, diagnostics plumbing, extracting
  the flagship, conformance cases.
- **End of each day — one commit, one line in the changelog.** Non-negotiable. The
  evidence that this phase happened is the thing that attracts the second person.

---

## 3. Phase 0 · 2–13 Nov — an LR parser generator in ceps

**Ten days. Hard stop Friday 13 November — stop, not finish.**

### What this is, named honestly

This is §7 of this document arriving two weeks early: the genuinely interesting problem
that feels like the right thing to work on. Scheduling it deliberately, with a stop date,
is the only version of it that does not cost December.

### Why it earns the fortnight

1. **It is a forcing function for L1.** You cannot write a parser generator in ceps
   without finding every corner where the semantics are undefined. Two weeks of hard use
   immediately before writing the semantics chapter is the best preparation available, and
   **the list of things you did not know is the raw material for Week 49.**
2. **Self-hosting is the credential for the audience in
   [LANG-ROADMAP.md](LANG-ROADMAP.md) §4.** `rollout2simulation.ceps` is the better
   demonstration for industry; a parser generator written in the language is the better
   one for language implementers. These are different audiences and you can have both.
3. **It completes the `.ceps.lex` story** — a regular layer and a context-free layer, both
   expressed as data rather than as hand-written C++. That feeds yadb directly.

### Conditions — non-negotiable

| # | Condition | Why |
|---|---|---|
| 1 | **Hard stop Fri 13 Nov.** | Parser generators are a famous rabbit hole. The stop is what makes this affordable. |
| 2 | **Do not touch `ceps.y`.** | It must stay authoritative and frozen through December — that is what makes L1 four days instead of two weeks. The new generator is a *validator* that reads the same grammar, never a replacement. |
| 3 | **SLR(1) and a driver.** | No error recovery, no conflict diagnostics beyond *"conflict here"*, no bison replacement. Upgrade to LALR(1) only if days 1–6 ran fast. |
| 4 | **Fix [D17](DEFECTS.md#d17) on day one.** | You will write `static_for(s : symbols)` within the first hour. This moves the fix out of 27 Nov and frees a day downstream. |
| 5 | **The semantics log is a deliverable.** | Not a side effect. One file, appended to daily: every construct whose behaviour you had to discover by experiment. |

### The days

| Day | Work | Note |
|---|---|---|
| **Mon 2** | Fix [D17](DEFECTS.md#d17). Grammar representation as ceps structs. Open the semantics log. | The bootstrap problem is free: a grammar written as ceps is already a tree, so the generator needs no parser to read its own input. |
| **Tue 3** | `nullable`, FIRST. | |
| **Wed 4** | FOLLOW. **Checkpoint.** | Set-heavy fixpoint computation over a pointer-chasing AST with no native set type. **This is where ceps is weakest and where this project dies if it dies.** If it is going badly, stop here and bank the log. |
| **Thu 5** | LR(0) items, closure. | |
| **Fri 6** | `goto`, the canonical collection. | |
| **Mon 9** | SLR(1) table construction, conflict detection. | |
| **Tue 10** | Driver in Oblectamenta — shift, reduce, goto over the compute stack. | The VM is nearly an LR driver already. The satisfying part. |
| **Wed 11** | Finish the driver; reduce actions building a ceps tree. | |
| **Thu 12** | Run it: arithmetic expressions, then a subset of ceps itself. | |
| **Fri 13** | **Stop.** Write up the semantics log. Tag whatever exists. | Whatever state it is in. |

### What counts as success

Not a finished parser generator. **The semantics log**, plus whatever runs. Half a
generator and a list of undefined behaviour still pays for the fortnight, which is why
condition 5 exists and why Wednesday 4 November is a real stop point rather than a
milestone.

**14–15 Nov off.** Start Monday 16 rested; the main block is 31 consecutive days.

---

## 4. The schedule

### Week 47 · 16–20 Nov — unblock, then grammar

| Day | Work | Done when |
|---|---|---|
| **Mon 16** | Repair the test harness ([ROADMAP.md](ROADMAP.md) 0.1). 39 of 41 run scripts invoke a binary path that no longer exists. | `test/` runs end to end and reports. You cannot verify a reference against an implementation you cannot exercise — this is why it is day one. |
| **Tue 17** | Reference skeleton. Keep the four principles in `doc/ceps-lang.md` as the preamble. Grammar: structs, `kind` and kind-instances, literals, units. | Lexical and structural core transcribed from `ceps.y`, with prose. |
| **Wed 18** | Grammar: path expressions, `.content()`, `.at()`, `is_id()` — the traversal layer ([D12](DEFECTS.md#d12)). | The layer `SKILL.md` never mentions is written down. |
| **Thu 19** | Grammar: `val`, `static_for`, `for`, `as_identifier`, macros. | The metaprogramming surface — the part `rollAut` rests on and no document names. |
| **Fri 20** | Grammar: `sm{}`, `states{}`, `t{}`, `Actions{}`, `Simulation{}`. | State machine surface. Cross-check against `SKILL.md`; record disagreements rather than silently picking one. |

### Week 48 · 23–27 Nov — finish grammar, then names

| Day | Work | Done when |
|---|---|---|
| **Mon 23** | Grammar: `oblectamenta{}` / `asm{}` / `msg{}`. Fold in the existing `doc/ceps-lang.md` assembler section and `ASM.md`. | Grammar complete. |
| **Tue 24** | **LLM checkpoint #1.** Reference so far into context; ask for a model with two state machines and a transition table. Record every failure verbatim. | A list of what the reference does not yet say. This is the §1.3 acceptance test, run early and often rather than once at the end. |
| **Wed 25** | Fix what checkpoint #1 exposed. Expect this to be about ambiguity, not omission. | Checkpoint #1 failures addressed. |
| **Thu 26** | **Name resolution.** Scopes, shadowing, the unit namespace. Write the rule first, then read `cepslexer.cpp` to find out what actually happens. | The rule is written and the divergence from the implementation is listed. |
| **Fri 27** | **Triage the Phase 0 semantics log.** Sort every entry into: belongs in the reference, is a defect, or was a misunderstanding. | The log becomes the outline for Week 49. This day exists because [D17](DEFECTS.md#d17) was fixed on 2 Nov. |

### Week 49 · 30 Nov – 4 Dec — the hard part

| Day | Work | Done when |
|---|---|---|
| **Mon 30** | **Evaluation semantics, pass structure.** `--pr` → `--pe` → `--ppe` as the spine: parse, expand, execute. | Each pass has a statement of what it may observe and what it may produce. |
| **Tue 1** | Semantics: expansion. When `val` evaluates, when `static_for` unrolls, what a macro sees, in what order. | The questions a transformation author actually hits are answered. |
| **Wed 2** | Semantics: execution. How the expanded tree becomes a running simulation; where results are written back ( the `--ppe` tree). | The projection property is explained mechanically, not asserted. |
| **Thu 3** | **The prelude decision.** Either predeclare the standard kinds or make `.ceps/prelude.ceps` auto-load. Today it is a convention two projects observe and the implementation ignores. | `Event E;` works from a clean install, or the reference says plainly why it does not. |
| **Fri 4** | **LLM checkpoint #2 — the L1 gate.** Three state machines, a `static_for` transformation, a message definition, from the reference alone. | **Decision point. See §4.** |

### Week 50 · 7–11 Dec — loud failure

| Day | Work | Done when |
|---|---|---|
| **Mon 7** | Diagnostics: carry expected/found out of the parser. Today an undeclared kind and an unterminated struct produce the identical `syntax error`. | Two different mistakes produce two different messages. |
| **Tue 8** | Name the top five newcomer errors. First among them: *"`Event` is not a known kind; declare it with `kind Event;`"*. | The five failures that end a first session each name their cause. |
| **Wed 9** | Finish diagnostics; re-run both LLM checkpoints against the new messages. | A model given a compile error can act on it. |
| **Thu 10** | Silent-path audit. [D7](DEFECTS.md#d7) (feature disabled by an undocumented `return;`), [D8](DEFECTS.md#d8) (flags parsed and never read), and anything else that accepts an instruction and does nothing. | The list exists. Fix what is cheap; file the rest. |
| **Fri 11** | **Buffer. Scheduled, not earned.** | Weeks three and four are where slip accumulates. If nothing has slipped, start Week 51 early. |

### Week 51 · 14–18 Dec — the flagship

| Day | Work | Done when |
|---|---|---|
| **Mon 14** | Extract the `rollAut` rollout example into this repository. Lex file, plan, five transformations, customer specifics removed. | It is here and it is yours to publish. |
| **Tue 15** | Make it run from a clean clone — no `LD_LIBRARY_PATH` improvisation, no absolute paths. | `git clone && build && ./run` produces five artifacts from one plan. |
| **Wed 16** | **`--help` tells the truth.** 54 flags parsed, 19 listed. Low-energy day, high satisfaction, visible result. | Every flag that does something is listed; every inert one is removed or marked. |
| **Thu 17** | Write up the flagship: one plan, five projections, **eight lines** for the dry-run everybody else pays for separately. | The argument lands before the explanation starts. |
| **Fri 18** | The sentence (LANG-ROADMAP §4.2) into `README.md` and `POSITIONING.md`. The comptime anchor into the introduction. | A stranger reading `README.md` knows within a paragraph what this is. |

### Week 52 · 21–23 Dec — conformance and freeze

| Day | Work | Done when |
|---|---|---|
| **Mon 21** | Conformance cases for every construct in the reference. Seed these throughout the preceding weeks; today is consolidation, not a standing start. | The suite runs and is the authority when reference and implementation disagree. |
| **Tue 22** | Changelog, version, a statement of what may change. | There is a release, not a `git clone`. |
| **Wed 23** | **Freeze.** Clean clone on a machine you have not been working on. Build, install, run the flagship, run the suite, read the reference as a stranger. Write down everything that bites; fix nothing today. | A list. The discipline is in not fixing on freeze day. |

**24–26 Dec off.**

### Week 53 · 28–30 Dec — land it

| Day | Work | Done when |
|---|---|---|
| **Mon 28** | Fix what the freeze found, in severity order. | The list is empty or consciously deferred. |
| **Tue 29** | **First cut item.** If you are on schedule: the MDF pipeline as second demonstration — lift 1.2 GB, run the analysis, swap in test data by commenting out four lines. If you are behind: skip it without regret. | Second demonstration exists, or is explicitly dropped. |
| **Wed 30** | **LLM checkpoint #3**, final. Tag the release. Write the announcement — one idea, one sentence, one demo, per LANG-ROADMAP §4. | Tagged, announced, done. |

**31 Dec off.**

---

## 5. The decision point: Friday 4 December

Checkpoint #2 is the gate, because everything after it is optional and L1 is not.

- **Reference holds up** → continue as scheduled.
- **Reference is close** → spend Mon 7 and Tue 8 on it, push diagnostics into Week 51, and
  take the MDF demo off the board now rather than in three weeks.
- **Reference is not close** → **cut L3 entirely** and spend the rest of December on L1 and
  L2.

That last branch is a success, not a failure. A reference with no flagship is still entry
into the league. A flagship with no reference is a demo that nobody can build on, and you
already have two of those.

---

## 6. Cut list, in order

When it slips — and it will — cut from the top. Decide at the Friday checkpoints, never
mid-week.

1. MDF second demonstration (Tue 29 Dec).
2. Silent-path audit (Thu 10 Dec) shrinks to *enumerate, fix nothing*.
3. Conformance breadth (Mon 21 Dec) shrinks to the core constructs.
4. `--help` (Wed 16 Dec) shrinks to adding the missing entries without the inert-flag
   cleanup.
5. The `doc/ceps-lang.md` assembler sections — point at `ASM.md` instead of merging.

**Never cut:** the grammar, the evaluation semantics, the D17 fix, the three LLM
checkpoints, the 23 Dec freeze. Those five are the phase. Everything else is decoration.

Phase 0 is not on this list in either direction. It is cut in its entirety by not starting
it, or it is stopped on 13 Nov. There is no middle setting.

---

## 7. Two standing risks

**The 90/10 reflex.** Every item in the main block is in the last ten percent — the part
with no unsolved structure in it. Somewhere around week three a genuinely interesting
problem will present itself: the arena, `c{}`, a better `msg{}`, finishing the parser
generator. It will feel like the right thing to work on and it will be the thing that
costs December. Write it down and keep going. `arena_allocator/` and `compressed_ast/`
are already sitting in `~/dev` — they are L4, they are January at the earliest, and they
are the specific temptation.

Phase 0 is the controlled version of this risk, not an exception to it. The thing that
makes it affordable is the 13 November stop, and the stop only works if the parser
generator stays stopped. **An unfinished generator in `~/dev` on 16 November is the single
most likely cause of a failed December.** If that pull is strong on the 13th, the honest
move is to write the log, tag it, and not open the directory again until January.

**Silent scope growth in the reference.** The grammar will tempt you into documenting
everything the parser accepts. The acceptance test is not completeness, it is checkpoint
#2: can a reader who has never seen ceps write a working model from this document alone.
Anything not serving that is week five, next year.

---

## 8. What this phase is worth

Forty-one days against fifteen years. At the end: a reference that did not exist, a
language that fails loudly instead of silently, one command that turns one file into five
artifacts — and, if Phase 0 lands, a parser generator written in the language it parses.

That is enough to be argued with on the merits, which is the whole point of
[LANG-ROADMAP.md](LANG-ROADMAP.md). And it is the thing that makes a second person
possible — which §8 of that document argues is the actual constraint, and has been for
fifteen years.
