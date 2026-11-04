# Audit: the `.ceps.lex` engine

A close reading of `core/include/transform/streamtransform.hpp` — 1649 lines, header-only,
the whole implementation of the `.ceps.lex` language: scanner, rewrite-rule parser, layered
matcher, and an embedded side-effect interpreter.

The reading was prompted by writing a `.ceps.lex` grammar for INCOSE requirements
(`examples/incose/hello_world`), which exercised the engine end to end for the first time
outside the import scripts it was built for.

Every entry below was reproduced against `bin/ceps`, version 0.8.1.3.3, Nov 4 2026 build,
on Linux. Line numbers refer to the working tree at the time of writing. Findings are
numbered `L1`…`L18` to keep them separate from `DEFECTS.md`, which records defects of the
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
| [L14](#l14) | High | Silence | `\|` does not reliably escape the guard separator: a rule containing it can be accepted without complaint and never fire |
| [L7](#l7) | High | Documentation | The only documentation of the format describes an engine that does not exist: two of its functions are unimplemented, its `$N` indexing is off by one, and the algorithm it specifies re-feeds its own output, which the code does not do |
| [L8](#l8) | Medium | Silence | An unrecognised `%pragma` is accepted and ignored without a word |
| [L9](#l9) | Medium | Performance | The matcher heap-allocates once per token scanned |
| [L10](#l10) | Medium | Silence | Input the grammar does not cover has no reportable outcome — three exits, no diagnostic in any of them |
| [L16](#l16) | Medium | Termination | `rewind` without `exit` cannot progress; the engine loops silently, accumulating memory at ~60 MB/s, and never reports anything |
| [L17](#l17) | Medium | Correctness | An action may both emit and `rewind`, and **both take effect** — one input token leaves two nodes behind. Latent today, and the precondition [G11](GOALS.md#g11--channels) depends on |
| [L18](#l18) | Medium | Memory safety | `call` pushes a real stack, and nothing bounds it: past `max_env_stacking_` = 4096 the engine indexes a vector out of range and segfaults. Latent — no grammar in the tree recurses |
| [L11](#l11) | Low | Correctness | `strtol` for a 64-bit field, an unsigned underflow, and a signed/unsigned comparison — all correct only by accident |
| [L12](#l12) | Low | Hygiene | Dead members, dead debug scaffolding, and POSIX headers in an otherwise portable header |
| [L15](#l15) | Low | Documentation | A right-hand side may splice a capture into the *name* of the node it emits; this is in production use and written down nowhere |
| [L13](#l13) | — | Open question | The embedded side-effect interpreter is a second language inside the first, undocumented — and, on a closer reading of `dbc.ceps.lex`, load-bearing |

## Triage

**The same diagnosis as `DEFECTS.md`.** Eight of the eighteen produce a plausible answer and
exit 0 — L2, L7, L8, L10, L11, L14, L17, and D25. The one that matters most is **L2**: it does not
fail, it answers *confidently and wrongly*, and it does so on the exact document a European
OEM would hand you. By the yardstick `DEFECTS.md` already applies — a silent wrong answer is
worse than a crash — L2 is the most serious finding in this file, ahead of L1.

**The memory-safety cluster (L1, L3, L4, L5, L18) has one shared cause**: a size is assumed
rather than checked. Five different fixed bounds, five different ways past them. L1 is rated
Critical rather than High because its failure is *not* guaranteed to be a crash — glibc's
fortification caught it here; the same write without fortification is silent corruption —
and because it is reachable from input data rather than from a grammar the user wrote.
L18 is the mildest of the five only because nothing in the tree recurses yet.

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

## <a name="l7"></a>L7 — The documented engine is not the one that exists

**Severity: High. Kind: documentation.**

Lines 70–95 of the header are the **only** description of the `.ceps.lex` format anywhere in
the repository. The worked example they contain is broken in three independent ways, and the
algorithm sketch above it describes different behaviour from the code.

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

**The algorithm sketch above it is wrong as well.** Lines 40–56 describe `preprocess` as a
fixed point:

```
 *      check for matching rewrite rules
 *      take the rewrite rule with maximal index, if there is none => return true
 *      buffer = [t_1,t_2,...,t_n] where t_i is the ith token of the right hand side
 *      goto start
```

The `goto start` says that emitted right-hand-side tokens are fed back through the rules. The
engine does not do this. It is single-pass:

```
$ cat c2.ceps.lex
...
BEGIN{
 a => wrapper{b;}; .
 b => deeper{1;}; .
 any => .
}

$ echo 'a' > in2.ch && ceps c2.ceps.lex in2.ch --pe
(STRUCT "wrapper"
  (ID "b"
  )
)

$ echo 'b' > in3.ch && ceps c2.ceps.lex in3.ch --pe   # control: the rule works
(STRUCT "deeper"
  (INT 1  )
)
```

The emitted `b` is not re-matched. So the comment documents a rewriting engine with a
different computational character from the one that exists — and the documented one has no
termination guarantee, since `a => a a .` would diverge under it. See `GOALS.md`, G11: the
explicit channel stack is the same idea in a form that terminates by construction.

The honest repair is to delete both the invented example and the sketch, and point at a
grammar in the tree that is executed — see `GOALS.md`, G7.

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
  grammar does use it, and **not decoratively** — `read_id_list`
  (`import-scripts/dbc/dbc.ceps.lex:33-37`) is built on it:

  ```
  lexer read_id_list{
   ident , => $0; .
   ident | !read_id_list_end => $0; /read_id_list_end=1;/ .
   any | read_id_list_end => rewind exit .
  }
  ```

  Worth reading closely, because of *what* it is buying: the guard and the assignment exist
  to terminate a **list**. The pattern language cannot say "repeat until there is no comma",
  so the grammar builds a one-bit state machine by hand. The sublanguage is load-bearing, but
  what it is standing in for is **iteration**, which is a far smaller feature than sixteen
  node kinds and a symbol table.
- It is the one part of the engine that does not fit the layering: layers stack and hand
  control back with `rewind exit`; the side-effect store is global mutable state that cuts
  across them.

The question for `GOALS.md` is whether this earns its quarter of the file, given that ceps
itself is the evaluator and the engine's job is to recognise. The `read_id_list` reading
sharpens it: *cut* is not a free option, because something real would be lost. *Replace with
iteration* may be.

---

## <a name="l14"></a>L14 — `|` is a guard separator, and `\|` does not reliably escape it

**Severity: High. Kind: silent wrong answer.**

An unescaped `|` in a pattern does not match a pipe. It **ends the pattern and begins a guard
expression** (`:1136`):

```cpp
else if (t.kind() == Token::TOK_TOK && t.sval_ == "|")
{   
  expr_parser_gettoken();
  guard_condition = parse_sideeffect_expr();
  break;
}
```

This is not documented anywhere — not in the header comment, not in `INVENTORY.md`. The only
way to learn it is to read this line or to read `dbc.ceps.lex`, where both meanings appear
**within ten lines of each other**:

```
 [ any \| any ] => min{$1;}; max{$3;};.      // line 21 — pipe meant literally
 ident | !read_id_list_end => $0; …          // line 35 — pipe meant as a guard separator
```

So far this is a readability complaint, and a direct instance of the rule in `GOALS.md`
*The yardstick*: a metacharacter is a character that does not mean itself, and it is paid for
in escapes.

### The escape does not work, and fails in silence

```
$ cat t.ceps.lex
%ext_tst
%ignore_ws
%%
BEGIN{
 a \| b => hit{1;}; .
 any => .
}
%%
$ echo 'a | b' > in.tst
$ ceps t.ceps.lex in.tst --pe
$ echo $?
0
```

No output. No diagnostic. Exit 0. The rule never fires.

The same grammar with `\.` instead of `\|`, against `a . b`, emits `(STRUCT "hit" (INT 1))` —
so escaping works in general, and `\|` specifically does not.

### It is position-dependent, which is the part to be careful about

Three results that do not sit together comfortably, all reproduced:

| pattern | position in block | result |
|---|---|---|
| `a \| b` | first rule of `BEGIN` | **no match**, silent |
| `any \| any ]` | first rule of `BEGIN` | **no match**, silent |
| `any \| any ]` | second rule of `BEGIN` | matches |
| `\|` alone | first rule of `BEGIN` | matches |
| `[ any \| any ]` | first rule of a **named sub-lexer** (`read_signal_min_max`) | matches — the shipped DBC importer depends on this |

So `\|` works when it is the first token of a pattern, and works inside the DBC sub-lexers,
and does not work when it follows another token at the top of `BEGIN`. **The behaviour is
reproduced; the mechanism is not established here** — the rule reader, the escape handling in
`gettoken_local`, and the `|` branch above interact, and the author should be the one to say
how.

What can be said without reservation is the consequence: **a grammar can contain a rule that
looks correct, is accepted without complaint, and never fires.** That is the same class as
[L2](#l2), [L8](#l8) and [L10](#l10) — the engine answering confidently with nothing.

### Related

`read_sender_list` (`dbc.ceps.lex:80-83`) has neither an `else` nor a catch-all. Input that
matches neither `any ;` nor `any ,` has no path out of the sub-lexer. See [L10](#l10).

---

## <a name="l15"></a>L15 — Identifier splicing is undocumented

**Severity: Low. Kind: documentation.**

A right-hand side may concatenate a capture onto an identifier to **construct the name of the
node it emits**:

```
 VAL_ any ident => endl dbc_value_description_mapping$1{ sig{…}; }; endl.
```

`$1` is spliced into the identifier, not passed as a child. Confirmed firing — the shipped
DBC import of `frames.dbc` produces a node literally named
`dbc_value_description_mapping601`:

```
$ ceps ../common.ceps dbc.ceps.lex frames.dbc config.ceps param_import_dbc.ceps \
    --print_evaluated_input_tree | grep -oE 'dbc_[a-z_]*[0-9]+'
dbc_value_description_mapping601
```

`dbc_signal_comment$2` uses the same construction.

This is a real feature — it is how the DBC importer keys per-message data into distinct
nodes — and it is written down nowhere. It also complicates
[G4](GOALS.md#g4--references-are-names-not-positions): a named hole must have a reading in
*name position* as well as in value position, and `dbc_value_description_mapping<canid>`
needs to be legible as splicing rather than as a child.

---

## <a name="l16"></a>L16 — `rewind` without `exit` cannot progress, and loops in silence

**Severity: Medium. Kind: termination.**

`rewind` returns the matched input to the stream; `exit` leaves the layer. Every `rewind` in
every grammar in the tree is written `rewind exit`, and nothing in the documentation says
why. The reason is that the other spelling does not terminate.

### Reproduction

```
%ext_probe
%ignore_ws
%%
BEGIN{
 a => marker{1;}; rewind .
 any => .
}
%%
```

on an input file containing `a`. The rule matches, emits, hands `a` back to the stream, and
the same layer matches `a` again — forever. Nothing is printed, no diagnostic is issued, and
the process does not stop.

It does not merely spin, either. Sampling `VmRSS` of the live process:

```
t=1s  VmRSS:    94712 kB
t=2s  VmRSS:   154076 kB
t=3s  VmRSS:   216148 kB
t=4s  VmRSS:   515616 kB
```

Roughly 60 MB/s of accumulated output that is never flushed, because the run never reaches
the point where it would be. A grammar of six lines will exhaust the machine.

### Why Medium and not High

It is reachable from a **grammar**, not from an input document, so the blast radius stops at
the person writing the grammar — unlike [L1](#l1) and [L2](#l2), which travel with the data.
A hang also announces itself, which puts it outside the silent-wrong-answer class that
`DEFECTS.md` rates worst.

It is still in the silent class in the sense that matters here: the engine has everything it
needs to know this rule cannot make progress, and says nothing — not at load time, when the
rule is parsed, and not at run time, when the same offset comes round a second time.

### Suggested direction

**Static.** A rule whose action contains `rewind` but neither `exit` nor a side-effect
assignment cannot change anything about the next round, and is therefore a non-terminating
rule by construction. Reject it when the rule is read, with its position
([L6](#l6) is the precondition).

The side-effect carve-out is necessary rather than cautious: `read_id_list` in
`dbc.ceps.lex:33-37` rewinds under a guard that a sibling rule sets, so "rewind without exit"
is only *certainly* a loop when nothing else can change.

**Dynamic backstop.** If the matcher arrives at the same machine state twice with the input
at the same offset, it cannot escape, and it can say so and name the rule.

**"Machine state" must include the call depth.** An earlier draft of this entry proposed the
triple (offset, layer, guard state), which is **wrong**: `call` pushes a real stack
([L18](#l18), and *What this component actually is* below), so the same triple legitimately
recurs at different depths in a correct recursive grammar. A backstop that ignored depth
would abort valid work. The state is (offset, layer, guard bindings, **call depth**).

Even then the check is a backstop and not a decision procedure — it catches a loop once the
machine repeats itself, which is sound, but it is not a static guarantee of termination. With
a stack in the machine, there is no cheap complete one.

---

## <a name="l17"></a>L17 — An action may both emit and rewind, and both take effect

**Severity: Medium. Kind: correctness, latent.**

There are 25 occurrences of `rewind` across the four grammars that use it — nine distinct
rules in `dbc.ceps.lex`, replicated in three near-copies, plus one in `gherkin.ceps.lex`.
**Every one of them is `rewind exit` with nothing emitted.** The uniformity invites the
reading that the engine enforces it.

It does not. The engine accepts an action that emits *and* rewinds, and performs both.

### Reproduction

```
%ext_probe
%ignore_ws
%%
lexer sub{
 b => inner{1;}; rewind exit .
}
BEGIN{
 a => call sub .
 b => outer{2;}; .
 any => .
}
%%
```

on the input `a b`:

```
(STRUCT "inner" (INT 1))
(STRUCT "outer" (INT 2))
```

One input token, two output nodes. `sub` matched `b`, emitted `inner`, then returned `b` to
the stream; `BEGIN` re-read the same token and emitted `outer`. Exit 0, no diagnostic.

Ordering within the action is irrelevant — `rewind inner{1;}; exit` behaves identically — so
this is not a sequencing subtlety. Emission and rewind are simply independent effects that
the engine never checks against each other.

### Why it is listed although nothing in the tree does it

Two reasons.

First, it is the silent class in its purest form: the output is *plausible*. A duplicated
node in a large import does not look like a fault, it looks like the input had two of
something.

Second, and the reason it is worth a number: **this is the invariant the pipelined channel
stack in [G11](GOALS.md#g11--channels) rests on.** A rule that emits and then rewinds has
already sent something downstream that it is about to re-derive. In a batch pass that is a
duplicate; in a pipeline where the next channel is already consuming, it is a branch
misprediction, and recovering from it requires every channel boundary to carry a flush and
rollback protocol.

Because `rewind exit` with no emission holds everywhere today, that protocol is not needed —
a rule firing is a commit. But it holds by habit, and G11 would be building on it. It should
be made a checked invariant *before* anything depends on it, not after.

### Suggested direction

Reject at load time any rule whose action both emits and rewinds. This costs nothing: no
grammar in the tree would change, and the error message writes itself.

---

## <a name="l18"></a>L18 — Recursion depth is unchecked, and overruns a fixed vector

**Severity: Medium. Kind: memory safety. Latent — no grammar in the tree recurses.**

`call` pushes a real stack (see *What this component actually is*). The stack is
`std::vector<lex_env> lex_envs_`, sized **once** to `max_env_stacking_ = 4096`
(`:1364-1371`) and thereafter indexed with `operator[]` by a counter that nothing bounds:

```cpp
 size_t max_env_stacking_ = 4096;
 ...
   lex_envs_.resize(max_env_stacking_);
 ...
   auto tt = cur_env_++;              // :1452 — no check against max_env_stacking_
   bool r = gettoken(t,l_id);         // :1455 — and gettoken recurses in C++ too
   ...
   lex_envs_[cur_env_+1] = lex_env(); // :1463 — heap write past the end
```

This is the fifth member of the memory-safety cluster, with the same shared cause as
[L1](#l1), [L3](#l3), [L4](#l4) and [L5](#l5): *a size assumed rather than checked*.

### Reproduction

A grammar whose sub-lexer recurses once per input token:

```
%ext_probe
%ignore_ws
%%
lexer rec{
 a => A{1;}; call rec .
 else => rewind exit .
}
BEGIN{
 start => call rec .
 any => .
}
%%
```

The depth is then set by **the input document**, and the boundary falls exactly where
`max_env_stacking_` says it should:

| input | result |
|---|---|
| `start` + 4000 × `a` | exit 0, 4000 `A` nodes |
| `start` + 4090 × `a` | exit 0, 4090 `A` nodes |
| `start` + 4200 × `a` | **SIGSEGV**, no output, no diagnostic |
| `start` + 6000 × `a` | **SIGSEGV** |

A grammar that recurses *without* consuming input faults immediately, and does so whatever
`ulimit -s` is set to — the fault is the vector, not the thread stack:

```
lexer rec{ else => call rec . }
```

### Why Medium, and the circumstance that raises it

**No grammar in the tree recurses.** Both call graphs are acyclic and shallow:

```
gherkin:  BEGIN -> read_descr
          BEGIN -> read_scenario -> {read_given, read_given_and, read_when, read_then}
dbc:      BEGIN -> {skip_ns_enumeration, read_value_description_mapping, read_sender_list}
          BEGIN -> read_signals -> read_signal -> {signedness, factor_offset, min_max,
                                                   unit, read_id_list}
```

Maximum depth four, against a ceiling of 4096. Nothing shipped is near it, which is why this
is Medium rather than High.

It becomes High the moment anyone uses recursion for the thing recursion is for — nesting.
A grammar over brackets, JSON, XML or nested blocks recurses once per level, and at that
point **the depth is a property of the input document** rather than of the grammar. 4096 is
not a comfortable distance from a generated file, and the failure mode is a segfault with
nothing printed.

### Suggested direction

Test `cur_env_` against `max_env_stacking_` before the increment, and report *grammar
recursed deeper than N, in lexer X* — which needs [L6](#l6)'s positions to be worth reading.
Making the limit configurable is secondary; reporting it at all is the fix.

`gettoken` also recurses in C++ at `:1455`, so the host stack is a second and softer ceiling.
Bounding `cur_env_` bounds both.

---

## What this component actually is

The name is wrong, and the name has cost something.

**It is not a lexer.** A lexer is a finite-state machine — that is what the word means, and
it is the property everything else assumes. This engine has a stack. `call` does not jump,
it *pushes*, and the action resumes where it left off when the callee returns:

```
lexer rec{
 a => down{1;}; call rec up{1;}; .
 else => rewind exit .
}
```

On `start a a a` this emits

```
down down down up up up
```

and not `down up down up down up`. The `up` nodes unwind in LIFO order, which is a stack and
nothing else. No finite-state machine produces that bracketing for arbitrary *n*, so the
engine is **strictly beyond regular** — demonstrated rather than argued.

**It is not Turing complete either**, and it is worth being exact about why, because guards
and recursion together look as though they ought to be enough. Three things block it:

1. **No arithmetic.** The side-effect sublanguage ([L13](#l13)) has exactly `ASSIGN`, `OR`,
   `AND`, `NOT`, `EQ`, `NEQ`, `GT`, `LT`, `IF`, `ELSE`, `RETURN`, `MATCH_REF` and
   `CLEAR_SYMBOLS` — comparison and boolean logic, no `+`, no increment. A Minsky counter
   machine needs two counters with increment, decrement and test-for-zero, and there is no
   construction for a counter here: `symbols` is a `std::map<string,Token>`, so finitely many
   names each holding **one** token, and no operation grows a value.
2. **No feedback.** Output never re-enters the input — the engine is single-pass, proved
   under [L7](#l7). A rewriting system that cannot read what it wrote cannot iterate on its
   own output, and it is exactly that loop which makes semi-Thue systems universal.
3. **The stack is bounded regardless** — 4096, per [L18](#l18). A pushdown automaton with a
   fixed stack bound is formally a finite automaton again.

The honest classification is therefore **a pushdown machine with finitely many registers over
an unbounded token domain, running in a single pass**: above regular, below Turing complete.
Exactly where it sits in between is not something this audit pins down.

### The part that is genuinely interesting

The engine that is *documented* **would** be Turing complete. The header comment
([L7](#l7)) specifies a fixed point — rewrite, feed the output back, start again. Add that
one edge and the machine becomes a general rewriting system, with everything that implies,
including undecidable termination.

So the component sits between two wrong descriptions, and they are wrong in opposite
directions. **The name says regular. The documentation says universal. The code is neither.**

### Why this is more than a label

Three findings in this file are plausibly *downstream of the name*:

- Lexers always terminate, so nothing checks termination — [L16](#l16).
- Lexers do not recurse, so nothing bounds the recursion — [L18](#l18).
- Lexers are regular, so the stack is written down nowhere — and nor is [L15](#l15)'s
  splicing, nor [L14](#l14)'s guard separator.

Calling a pushdown rewriting machine a lexer sets the expectation that none of those
questions need asking. The implementation already knows better: the file is
`transform/streamtransform.hpp`, in a directory called `transform`, and *stream transform* is
very nearly the right name. Only the surface — `.ceps.lex`, `lexer NAME{…}` — still says
otherwise.

Renaming is neither urgent nor free: `.ceps.lex` appears in seven grammars and their run
scripts. It is recorded here as a decision to take deliberately rather than a wart to live
with, and is filed as an open decision in [`GOALS.md`](GOALS.md).

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

That "untouched" is doing more work than it looks. Across all 25 uses of `rewind` in the
tree, a rule that gives input back emits nothing — *a rule firing is a commit*. It is the
property that makes backtracking cheap here, and the one that would make a channel pipeline
flush-free. It is also, per [L17](#l17), unenforced: the engine will happily do both. Worth
promoting from habit to invariant before [G11](GOALS.md#g11--channels) leans on it.

**Backtracking is a value copy.** `state_t` is `std::pair<intmax_t,intmax_t>`;
`get_state()`/`read_state()` are assignment. No machinery, no ownership question, no cost.

**The ratio.** 1649 header-only lines with no dependencies, driving seven grammars in the
tree including a working DBC importer. The faults above are real, but they are faults of a
small component that does a large job.
