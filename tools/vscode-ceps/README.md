# ceps syntax highlighting for VSCode

Basic TextMate highlighting for `.ceps` files, covering the language proper and
the Oblectamenta assembler.

## Install

Copy or symlink this directory into your extensions folder and restart VSCode:

    ln -s "$PWD/tools/vscode-ceps" ~/.vscode/extensions/ceps-0.1.0

For a remote/SSH or dev-container window the folder is `~/.vscode-server/extensions`
instead. To build a `.vsix` for sharing, `npx @vscode/vsce package`.

## What it highlights

| Scope | What lands there |
|---|---|
| `keyword.control` | `if` `else` `for` `static_for` `return` |
| `keyword.declaration` | `struct` `val` `let` `kind` `fun` `template` `macro` `label` `raw_map` `opr` |
| `entity.name.type` | the name introduced by `kind X;` |
| `storage.type` | the kind in a declaration — the `Event` of `Event E,F;` |
| `entity.name.variable` | the names that declaration introduces |
| `support.constant.si-unit` | **names that are really SI units — see below** |
| `support.function.opcode` | the 150 Oblectamenta opcodes |
| `variable.language.register` | `SP` `FP` `ARG0`–`ARG5` `RES` `R0`–`R15`, `addr`, `reg` |
| `support.class.sm` | `sm` `states` `Actions` `t` `Simulation` `oblectamenta` `data` `text` `asm` … |
| `entity.name.tag` | any other `name{` heading a struct |
| `comment` / `string` / `constant.numeric` | the usual |

### The unit scope

Fourteen names silently resolve to SI units rather than to whatever you meant —
`m metre meter s second kg kilogram celsius kelvin ampere cd candela mol mole`.
This is [D17](../../DEFECTS.md#d17), it is rated Critical, and it produces a wrong
answer with exit code 0 and no diagnostic. A loop variable called `m` binds to
metre; a struct called `s` is unreachable even by an explicit path.

The grammar gives those names a scope of their own, so in an editor they do not
look like names you introduced:

    Sensor m, s, kg, mol, cd;    // coloured as units -- all five are traps
    Sensor A, K, g;              // coloured as names -- these three are safe

That is the main reason to install this. `A`, `K` and `g` look like unit symbols
but are not in the list, so guessing which names are dangerous does not work.

## Regenerating the grammar

The word lists are **read out of the sources**, not retyped, because a retyped
list drifts:

| List | Read from |
|---|---|
| opcodes, registers, modifiers | `core/include/oblectamenta_decls.ceps` |
| SI unit names | `../ceps/core/src/ceps_interpreter_eval_id.cpp` |
| keywords | `../ceps/core/src/cepslexer.cpp` |

So this expects the `ceps` checkout beside `machines4ceps`, as the `Makefile`
already does. After changing any of those, regenerate and commit the result:

    python3 tools/vscode-ceps/generate-grammar.py

The only hand-maintained list is `sm_vocabulary` in the generator — those names
are matched as string literals in the C++ and are not declared anywhere
machine-readable.

## Checking it

`check-grammar.js` tokenizes with the real TextMate engine and asserts that the
scopes which matter actually land. It exits non-zero when they do not.

    npm install vscode-textmate vscode-oniguruma
    node tools/vscode-ceps/check-grammar.js          # checks sample.ceps
    node tools/vscode-ceps/check-grammar.js -v       # print every token's scopes

`sample.ceps` is the fixture it runs against. It exercises every rule and it is
a working program — `bin/ceps tools/vscode-ceps/sample.ceps` counts to 100,
prints it, reaches `Done` and exits 0 — so it cannot rot into something that
merely looks right.

## Limits

Highlighting is lexical. It does not know which identifiers are declared, so a
data label is only recognised inside an opcode's argument list, and state names
inside `states{…}` are not distinguished from anything else. Structure names
beyond the hardcoded `sm_vocabulary` list are highlighted generically as struct
headings, which is usually what you want anyway.
