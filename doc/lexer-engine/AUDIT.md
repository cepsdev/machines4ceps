# Audit: the `.ceps.lex` engine

A close reading of `core/include/transform/streamtransform.hpp` — 1649 lines, header-only,
the whole implementation of the `.ceps.lex` language: scanner, rewrite-rule parser, layered
matcher, and an embedded side-effect interpreter.

The reading was prompted by writing a `.ceps.lex` grammar for INCOSE requirements
(`examples/incose/hello_world`), which exercised the engine end to end for the first time
outside the import scripts it was built for.

Every entry below was reproduced against `bin/ceps`, version 0.8.1.3.3, Nov 4 2026 build,
on Linux. Line numbers refer to the working tree at the time of writing. Findings are
numbered `L1`…`L13` to keep them separate from `DEFECTS.md`, which records defects of the
ceps language and runtime; this document records defects of one component, and is kept
apart because the component is about to be reworked rather than patched.

`D25` in `DEFECTS.md` — positional `$N` in rewrite actions — belongs to this component and
is not restated here.

| # | Severity | Kind | Summary |
|---|---|---|---|
| [L1](#l1) | Critical | Memory safety | A numeric literal of 128 characters or more in an **input document** overflows a fixed stack buffer |
| [L2](#l2) | Critical | Correctness | Non-ASCII text is silently mistokenised, so a well-formed requirement is reported as non-conforming; exit 0 |
| [L3](#l3) | High | Memory safety | The operator matcher reads an uninitialised stack array, and its bound is fixed while its index is caller-supplied |
| [L4](#l4) | High | Memory safety | The reader dereferences before it checks bounds, over a buffer allocated with no sentinel byte |
| [L5](#l5) | High | Memory safety | The rule program buffer is a fixed 16384 entries, written through `operator[]` in 36 places with no check |
| [L6](#l6) | High | Diagnostics | Grammar errors are reported as `?`, `???`, `????`, `?????`, `??????`, or as a bare token-kind integer; no error anywhere carries a line or column |
| [L7](#l7) | High | Documentation | The only documentation of the format is a worked example that cannot run: two of its functions do not exist and its `$N` indexing is off by one |
| [L8](#l8) | Medium | Silence | An unrecognised `%pragma` is accepted and ignored without a word |
| [L9](#l9) | Medium | Performance | The matcher heap-allocates once per token scanned |
| [L10](#l10) | Medium | Silence | Input the grammar does not cover has no reportable outcome — three exits, no diagnostic in any of them |
| [L11](#l11) | Low | Correctness | `strtol` for a 64-bit field, an unsigned underflow, and a signed/unsigned comparison — all correct only by accident |
| [L12](#l12) | Low | Hygiene | Dead members, dead debug scaffolding, and POSIX headers in an otherwise portable header |
| [L13](#l13) | — | Open question | The embedded side-effect interpreter is a second language inside the first, undocumented and unexercised |

## Triage

**The same diagnosis as `DEFECTS.md`.** Six of the thirteen produce a plausible answer and
exit 0 — L2, L7, L8, L10, L11, and D25. The one that matters most is **L2**: it does not
fail, it answers *confidently and wrongly*, and it does so on the exact document a European
OEM would hand you. By the yardstick `DEFECTS.md` already applies — a silent wrong answer is
worse than a crash — L2 is the most serious finding in this file, ahead of L1.

**The memory-safety cluster (L1, L3, L4, L5) has one shared cause**: a size is assumed
rather than checked. Four different fixed bounds, four different ways past them. L1 is rated
Critical rather than High because its failure is *not* guaranteed to be a crash — glibc's
fortification caught it here; the same write without fortification is silent corruption —
and because it is reachable from input data rather than from a grammar the user wrote.

**L6 and L7 are the two that cost a newcomer the most**, and neither is hard. The position
information L6 wants already exists and is simply not used.

---

## <a name="l1"></a>L1 — A long numeric literal overflows a fixed stack buffer

**Severity: Critical. Kind: memory safety.**

`gettoken_local` copies a scanned numeric literal into a fixed stack buffer so it can be
handed to `strtod`/`strtol`, which need a NUL-terminated string. The copy is unbounded.

`core/include/transform/streamtransform.hpp:439`

```cpp
char buffer[128] = {0};

memcpy(buffer,p_.ptr_+start,p_.pos_-start);
```

`p_.pos_-start` is the length of the literal as it appeared in the file. Nothing constrains
it.

### Reproduction

```
$ cat probe.ceps.lex
%ext_prb
%ignore_ws
%%
BEGIN{
 ident integer \. => got{ name{$0;}; num{$1;}; }; .
 any => .
}
%%

$ python3 -c "print('value ' + '9'*300 + '.')" > input.prb
$ ceps probe.ceps.lex input.prb --pe
*** buffer overflow detected ***: terminated
Aborted (core dumped)
```

The threshold is exactly the buffer size:

| digits | exit |
|---|---|
| 126 | 0 |
| 127 | 0 |
| 128 | 134 (SIGABRT) |
| 200 | 134 (SIGABRT) |

### Why this is the Critical one of the four

The overflowing value is not in the grammar — it is in the **document being read**. Every
`.ceps.lex`-driven import is this surface: a `.dbc` file from a supplier, a log, a
requirements document. The write is `memcpy` into a stack frame with a fully attacker-chosen
length and content.

### Suggested direction

The buffer exists only because `strtod` and `strtol` need termination. `std::from_chars`
(C++17, `<charconv>`) takes a pointer range, needs no copy, and reports failure instead of
guessing — so the correct fix deletes the buffer rather than bounding it. See also
[L4](#l4), whose fix (allocate one sentinel byte) would make the old approach safe as well.

---

## <a name="l2"></a>L2 — Non-ASCII text is silently mistokenised

**Severity: Critical. Kind: correctness, silent.**

Identifier scanning is driven by `std::isalpha` on a plain `char`:

`core/include/transform/streamtransform.hpp:396`

```cpp
if (std::isalpha(ch))
```

In the default C locale no byte of a multi-byte UTF-8 sequence is alphabetic, so an
identifier containing one is split at the first non-ASCII byte. The fragments fall through to
the operator table, fail to match, and emerge as `TOK_UNKNOWN`.

The token kind that would have carried this case, `TOK_UTF8 = 4` (line 330), is **declared
and never produced anywhere in the tree** — the slot was reserved and never filled.

### Reproduction

Using the known-good INCOSE grammar, with one identifier changed from `DoorLock` to
`Türschloss` and nothing else:

```
$ ceps examples/incose/hello_world/ears.ceps.lex ascii.req --pe
(STRUCT "incose_init"
  (STRUCT "req"
    "REQ-001"
  )
  ...
exit=0

$ ceps examples/incose/hello_world/ears.ceps.lex umlaut.req --pe
(STRUCT "incose_nonconforming"
  (STRUCT "req"
    "REQ-001"
  )
  (STRUCT "report"
  ...
exit=0
```

The requirement is well-formed EARS. The engineer is told it is **non-conforming**, at exit
0, in these words:

```
[REQ-001] NON-CONFORMING: matches no accepted pattern, no model element generated
```

Every clause of that sentence is false. The conformance machinery built on top of the grammar
is not merely bypassed — it is turned into a source of confident false accusations, and the
state machine the requirement describes is silently absent from the model.

### A second problem in the same line

`std::isalpha` has undefined behaviour when its argument is a negative `int`. `char` is
signed on x86-64 Linux, so every byte ≥ 0x80 passes a negative value. Correct usage requires
a cast to `unsigned char`. This applies at line 396 and at line 469 (`std::isalpha(p_.next())`
in the pragma scanner), and to the `isalnum`/`isdigit`/`isspace` calls around them.

### Why this is Critical

It is the silent class, it is confident, and it lands exactly where this component was
pointed: requirements documents. A German, French or Swedish requirements corpus is not an
edge case for an INCOSE tool; it is the normal case.

---

## <a name="l3"></a>L3 — The operator matcher reads an uninitialised array

**Severity: High. Kind: memory safety.**

`core/include/transform/streamtransform.hpp:266-285`

```cpp
int match(P& in, char ch)
{
  if (toks_.size() == 0) return -1;
  bool state_table[1024];
  int states_active = 0;
  int max_state = -1;
  auto nn = toks_.size();
  for(size_t h = 0; h < nn;++h){
    auto & e = toks_[h];
    if (e.length()>0 && e[0] == ch) { state_table[h] = true; max_state = h ;++states_active; }
  }

  if(max_state == -1) return -1;
  ...
  for(int i = 0; i <= n;++i)
  {
    if (!state_table[i]) continue;
```

Two faults in six lines.

**Uninitialised read.** The seeding loop writes `true` only where the first character
matches. It never writes `false`. The matching loop then reads every index from `0` to
`max_state`, including those never written. This is undefined behaviour on every call that
does not match the lowest-indexed operator. A garbage `true` on a multi-character operator
whose second character happens to match will take over `max_state` and return the wrong
token.

**Fixed bound, caller-supplied index.** `state_table` is 1024 entries; `h` runs to
`toks_.size()`, set by the public `clear_and_read_table(const char* tbl[], size_t size)`.
The default table (`core/src/transform/streamtransform.cpp:79`) has 31 entries, so this is
latent today — but the bound and the index come from different places and nothing connects
them.

The uninitialised read is live **now**, at 31 entries, on most calls.

---

## <a name="l4"></a>L4 — Dereference before bounds check, over a buffer with no sentinel

**Severity: High. Kind: memory safety.**

`core/include/transform/streamtransform.hpp:188` and `:212`

```cpp
bool get(T& ch)
 {
  if (pos_ >= size_) return false;
  auto pos = pos_;
  for(;ptr_[pos] == '\n' || ptr_[pos] == '\r';++pos) if (pos >= size_) break;
```

```cpp
std::intmax_t number_of_chars_left_in_line()
{
    if (pos_ >= size_) return 0;
    auto pos = pos_;
    decltype(pos) n = 0;
    for(;ptr_[pos] != '\n' && ptr_[pos] != '\r';++pos,++n) if (pos >= size_) break;
```

In both loops `ptr_[pos]` is read *before* `pos >= size_` is tested. The test is in the loop
body; the dereference is in the condition that precedes it.

The buffer has no spare byte to absorb it. `core/src/transform/streamtransform.cpp:67`

```cpp
mem.ptr_ = new char[statbuf.st_size];
```

Exactly the file size. So any file whose final character is not a newline produces a
one-byte heap over-read in `get()`, which is the most frequently called function in the
engine.

Allocating `st_size + 1` and writing a NUL costs one byte, fixes both loops, and would also
make [L1](#l1)'s `strtod` path safe.

### Also in that function

`readfile_to_memory` never closes `inputfd` — one descriptor leaked per file read.

---

## <a name="l5"></a>L5 — The rule program buffer is fixed and unchecked

**Severity: High. Kind: memory safety.**

```cpp
enum {DEFAULT_RW_PROG_BUFFER = 16384};      // :558
...
prog_buffer_.resize(DEFAULT_RW_PROG_BUFFER);   // :596, :604
matching_rules_.resize(DEFAULT_RW_PROG_BUFFER); // :599, :607
```

`rw_prog_buffer_free_` appears 36 times, always as `prog_buffer_[rw_prog_buffer_free_++] =
…`. `std::vector::operator[]` does not check. There is no test against
`DEFAULT_RW_PROG_BUFFER` anywhere in the file, and no growth path: the two `resize` calls are
in the constructors and are the only ones.

One opcode is emitted per token of every pattern and every action, across every rule of every
layer. A grammar that crosses 16384 opcodes does not report anything — it writes past the
vector and corrupts the heap.

`matching_rules_` is sized by the same constant although it is indexed by **rule** number,
not opcode number. Two unrelated quantities share one bound, which is how a bound stops
meaning anything.

A single guarded `push` helper replaces all 36 sites.

---

## <a name="l6"></a>L6 — Grammar errors do not say what, and never say where

**Severity: High. Kind: diagnostics.**

The messages a user of `.ceps.lex` can actually receive:

| line | thrown |
|---|---|
| 876 | `match_ex("?")` |
| 1198 | `match_ex("???")` |
| 1224 | `match_ex("????")` |
| 1228 | `match_ex("?????")` |
| 1236 | `match_ex("??????")` |
| 1155 | `match_ex("read_rewrite_rule: Unknown Token " + <token kind as an integer>)` |
| 1275 | `match_ex{"read_rewrite_rule(): rhs faulty."}` |
| 613 | `struct eval_ex{};` — carries nothing at all |

`expr_parser_match` produces the only helpful family, `"Expected " + content`, and even that
has no position:

```
$ ceps doc.ceps.lex input.doc
***Lexer script faulty:Expected =>
***Fatal Error:A parser exception occured in 'doc.ceps.lex'.
```

The grammar in question is six lines long. In a 113-line grammar such as
`import-scripts/dbc/dbc.ceps.lex`, this message locates nothing.

### The position information already exists

`Memory` carries `pos_`, a byte offset into a buffer the scanner still holds. Line and column
are a scan of that buffer up to `pos_` — work done **only on the error path**, costing
nothing in the matching loop. Adding `pos_` to `match_ex` and formatting at the catch site
is a small change with a large effect, and it is the prerequisite for every other
diagnostic improvement in this file.

---

## <a name="l7"></a>L7 — The documented example does not run

**Severity: High. Kind: documentation.**

Lines 70–95 of the header are the **only** description of the `.ceps.lex` format anywhere in
the repository. The worked example they contain is broken in three independent ways.

**Two functions that do not exist.** `suffix()` appears at lines 80, 81, 87, 88 and 95;
`error()` at line 75. Neither is implemented. The side-effect expression parser accepts only
`print_symbols`, `return`, `$N`, `(`, `!`, `==`, `!=`, `&&`, `||`, `=`, `if`, `else` — there
is no call syntax at all.

```
$ cat doc.ceps.lex          # the documented guard form, verbatim in shape
...
 ident |suffix($0)!="_" => kept{$0;}; .
...
$ ceps doc.ceps.lex input.doc
***Lexer script faulty:Expected =>
***Fatal Error:A parser exception occured in 'doc.ceps.lex'.
exit=1
```

**The indexing is off by one.** The comment writes

```
VERSION string => version { $2; };
```

for a two-token pattern. The engine is **zero-based over all matched tokens including
punctuation**, so the string is `$1`. The comment is yacc-style 1-based throughout; this is
the same inherited convention that `D25` is about, and here it has already produced wrong
documentation.

**It documents a dialect nobody uses.** The real grammars in the tree
(`import-scripts/dbc/dbc.ceps.lex` and the six others) share an idiom the comment never
shows: `else => rewind exit .` to close a layer.

The honest repair is to delete the invented example and point at a grammar in the tree that
is executed — see `GOALS.md`, G7.

---

## <a name="l8"></a>L8 — An unknown pragma is accepted in silence

**Severity: Medium. Kind: silent.**

`core/include/transform/streamtransform.hpp:479-486`

```cpp
if (keyword == "ignore_ws") ignore_space_ = true;
else if (keyword == "encode_line_feed_as_endl") encode_line_feed_as_endl = true;
else if (keyword == "skip_unmatched_tokens") skip_unmatched_tokens_ = true;
else if (keyword == "dont_ignore_ws") ignore_space_ = false;
else if (keyword == "dont_encode_line_feed_as_endl") encode_line_feed_as_endl = false;
else if (keyword == "dont_skip_unmatched_tokens") skip_unmatched_tokens_ = false;
else pragmas.push_back(keyword.as_str());
```

The `else` is load-bearing: `%ext_req` and friends are read back out of `pragmas` by
`core/src/state_machine_simulation_core.cpp`. But it also swallows every typo. `%ignore_w`,
`%ignore_whitespace`, `%dont_ignore_whitespace` are all stored, applied to nothing, and
reported nowhere — and the symptom is a grammar that silently sees whitespace it was written
to ignore.

A `%ext_`-prefixed or otherwise namespaced form for the open-ended pragmas would let the
closed set be checked.

---

## <a name="l9"></a>L9 — The matcher allocates once per token

**Severity: Medium. Kind: performance.**

`core/include/transform/streamtransform.hpp:1506`

```cpp
std::vector<bool> guard_is_true(n);
```

This is inside `gettoken`, in the loop that runs for **every token of every input file**.
`n` is the rule count, so it is a heap allocation and release per token scanned. It is a
member variable's job.

In the same region, `matching_rules_` is a `std::vector<bool>` — the bit-packed
specialisation — accessed in the innermost loop, where a `std::vector<char>` or a reused
`std::uint64_t` word array would do the same work without the shift-and-mask on every
element.

This is the inner loop of every import the product performs.

---

## <a name="l10"></a>L10 — Input the grammar does not cover has no reportable outcome

**Severity: Medium. Kind: silent.**

`core/include/transform/streamtransform.hpp:1615-1626`

```cpp
if (min_matching_idx >= n && last_min_matching_idx < n) min_matching_idx = last_min_matching_idx;
if (min_matching_idx >= n && else_state == -1) {
 if (skip_unmatched_tokens_)
 {
     if(!r){t = orig_t; return r;}
     else continue;
 }
 else {t = orig_t; return r;}
} else if (min_matching_idx >= n && else_state >= 0)
{
 min_matching_idx = else_state;
 p_.read_state(state_raw_lexer_before_gettoken_local);
}
```

This is the branch that means *no rule in this layer understood the input*. It has three
exits — discard and continue, pass the raw token through, or divert to the `else` rule — and
**none of them records anything**. `else => rewind exit` is control flow, not a report.

The consequence is visible one level up: the INCOSE example had to carry its own catch-all
rule and emit its own non-conformance node, because the engine offers no way to ask "what did
you not understand, and where". Every grammar must re-invent that, per grammar, by hand —
and `%skip_unmatched_tokens` quietly removes the ability entirely.

This is the partiality gap recorded in `INVENTORY.md` §4.1 and `ROADMAP.md` 0.5. These twelve
lines are where the hook belongs.

### Related

`else_state = i;` (`:1564`) is assigned without the `min_matching_idx > i` guard that every
other branch applies, so the **last** `else` rule in a layer wins rather than the first. It
is also assigned at any `current_pos`, not only at position 0.

---

## <a name="l11"></a>L11 — Three things that are correct by accident

**Severity: Low. Kind: correctness.**

**`strtol` into a 64-bit field** (`:455`)

```cpp
t.v.ival64_ = std::strtol(buffer,nullptr,10);
```

`TOK_INT64` deserves `strtoll`. On LP64 Linux `long` is 64 bits so this is right; it is
wrong on any LLP64 target, and it is wrong to depend on.

**Unsigned underflow in `matched_tokens()`** (`:586`)

```cpp
if ((ssize_t)current_layer()>= matched_tokens_.size())
 for(int i =  matched_tokens_.size()-1 ; i < current_layer();++i)
  matched_tokens_.push_back(std::vector<Token>());
```

On an empty vector `matched_tokens_.size()-1` is `SIZE_MAX`, narrowed to `int i` to land on
`-1`, which happens to be the value that makes the loop push the right number of elements.
The comparison casts the `int` operand to `ssize_t` and then compares it against a `size_t`,
so the cast achieves nothing and the comparison is still signed-to-unsigned. `resize(current_layer()+1)`
says the whole thing.

**Newline runs collapse** (`:188`). `get()` folds any run of `\n`/`\r` into a single `'\n'`.
Blank lines are therefore invisible to every grammar: you cannot match an empty line, and
`%encode_line_feed_as_endl` yields one `endl` for a paragraph break. For a requirements
format where a blank line *is* structure, that is a real limit. It may well be the behaviour
you want — but it is currently undiscoverable, and it should be a named pragma rather than a
property of the reader.

---

## <a name="l12"></a>L12 — Dead weight

**Severity: Low. Kind: hygiene.**

- `Token::a` (`:314`) and `Token::b` (`:316`) — two `int` members, never read or written
  anywhere, copied with every token in the hot path.
- `constexpr bool print_debug_info = false;` (`:34`) at namespace scope in a header, with
  roughly a dozen `if (print_debug_info) std::cout << …` sites threaded through the matcher.
- `//if(orig_t.sval_=="BO_") std::cout << …` — DBC-specific scaffolding left inside the
  matching loop, and the commented-out `rw_rule_in_execution_` members at `:581`.
- `#include <unistd.h>`, `<fcntl.h>`, `<sys/stat.h>` — POSIX, used by `readfile_to_memory`
  in the `.cpp`, not by this header. Without them the header is portable.

---

## <a name="l13"></a>L13 — Open question: the embedded side-effect interpreter

**Not a defect. A design question that should be answered before the rework.**

`Sideeffects_node` (`:645`) declares sixteen node kinds — `STMT, ASSIGN, LEAF,
DEBUG_PRINT_SYMBOLS, OR, AND, NOT, EQ, NEQ, GT, LT, CLEAR_SYMBOLS, IF, ELSE, RETURN,
MATCH_REF` — served by a recursive-descent expression parser (`:617`–`:1040`), a `std::map`
symbol table, and a tree-walking `evaluate`. Together with the guard machinery it is roughly
a quarter of the file.

Observations, offered without a verdict:

- It is a **second language with its own evaluation semantics living inside the first**. The
  rest of the engine recognises and rewrites; this part computes.
- Its only documentation is the header example, which does not parse ([L7](#l7)).
- `CLEAR_SYMBOLS` and `GT`/`LT` are declared; `clear_symbols` appears nowhere else in the
  tree, and the expression parser's comparison chain handles only `==` and `!=`.
- The INCOSE grammar reached a complete working state without using any of it. The DBC
  grammar uses guards sparingly.
- It is the one part of the engine that does not fit the layering: layers stack and hand
  control back with `rewind exit`; the side-effect store is global mutable state that cuts
  across them.

The question for `GOALS.md` is whether this earns its quarter of the file, given that ceps
itself is the evaluator and the engine's job is to recognise.

---

## What is right

Worth recording, because a rework can lose it by accident.

**The inverted default.** In this pattern language a bare word is a **literal**; only
`ident`, `integer`, `string`, `double`, `any`, `else`, `endl`, `enter`, `exit` and the
escapes are syntax. It is one branch in the parser (`:1129`):

```cpp
else if (t.kind() == Token::TOK_ID)
{
   prog_buffer_[rw_prog_buffer_free_++] = rw_opcode(true,true,-1,t);
   ++pat_len;
}
```

Regex, BNF and ANTLR default the other way — *this is syntax, quote your text* — and the
cost is that the grammar stops being readable to the person who owns the requirements. Here
the EARS rules read almost like the sentences they match. That property is the reason the
INCOSE prototype landed, and it costs four lines.

**Layers that hand control back.** `lexer NAME{…}` with `else => rewind exit .`, and
`rewind_state_` carried on the rule, is one mechanism applied uniformly: a sub-language runs
until it stops recognising, then returns the input to its caller untouched. `dbc.ceps.lex`
is built almost entirely out of that single move.

**Backtracking is a value copy.** `state_t` is `std::pair<intmax_t,intmax_t>`;
`get_state()`/`read_state()` are assignment. No machinery, no ownership question, no cost.

**The ratio.** 1649 header-only lines with no dependencies, driving seven grammars in the
tree including a working DBC importer. The faults above are real, but they are faults of a
small component that does a large job.
