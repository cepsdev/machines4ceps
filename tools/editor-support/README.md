# ceps syntax highlighting

Basic highlighting for `.ceps` files — the language proper and the Oblectamenta
assembler — for **VSCode, Emacs and Vim**.

All three are produced by one generator, `generate.py`, which reads its word
lists out of the ceps sources. Three hand-copied lists would drift apart, and
drifting word lists are the defect this tooling exists to expose.

## Install

**VSCode** — symlink into your extensions folder and restart:

    ln -s "$PWD/tools/editor-support/vscode" ~/.vscode/extensions/ceps-0.1.0

For a remote/SSH or dev-container window the folder is `~/.vscode-server/extensions`.
To build a `.vsix` for sharing, `npx @vscode/vsce package` inside `vscode/`.

**Emacs** — add the directory to `load-path` and require the mode:

    (add-to-list 'load-path "~/dev/machines4ceps/tools/editor-support/emacs")
    (require 'ceps-mode)

`.ceps` files then open in `ceps-mode` automatically.

**Vim / Neovim** — install as a package:

    mkdir -p ~/.vim/pack/ceps/start
    ln -s "$PWD/tools/editor-support/vim" ~/.vim/pack/ceps/start/ceps

For Neovim use `~/.local/share/nvim/site/pack/ceps/start` instead. Without
packages, copy `vim/syntax/ceps.vim` and `vim/ftdetect/ceps.vim` into the
matching subdirectories of `~/.vim`.

## What it highlights

| What lands there | VSCode scope | Emacs face | Vim group |
|---|---|---|---|
| `if` `else` `for` `static_for` `return` | `keyword.control` | `keyword` | `Keyword` |
| `struct` `val` `let` `kind` `fun` `template` `macro` `label` `raw_map` `opr` | `keyword.declaration` | `keyword` | `Keyword` |
| the name introduced by `kind X;` | `entity.name.type` | `type` | `Type` |
| the kind in a declaration — the `Event` of `Event E,F;` | `storage.type` | `type` | `Type` |
| the names that declaration introduces | `entity.name.variable` | `variable-name` | `Identifier` |
| **names that are really SI units — see below** | `support.constant.si-unit` | `ceps-si-unit-face` | `Todo` |
| the 150 Oblectamenta opcodes | `support.function.opcode` | `function-name` | `Function` |
| `SP` `FP` `ARG0`–`ARG5` `RES` `R0`–`R15`, `addr`, `reg` | `variable.language.register` | `constant` | `Constant` |
| `sm` `states` `Actions` `t` `Simulation` `oblectamenta` `data` `text` `asm` … | `support.class.sm` | `builtin` | `Structure` |
| any other `name{` heading a struct | `entity.name.tag` | `function-name` | `Function` |
| comments, strings, numbers | the usual | the usual | the usual |

The unit face stands out on purpose: `ceps-si-unit-face` inherits
`font-lock-warning-face`, and Vim links to `Todo`. These are not decoration.

### The unit scope

Fourteen names silently resolve to SI units rather than to whatever you meant —
`m metre meter s second kg kilogram celsius kelvin ampere cd candela mol mole`.
This is [D17](../../DEFECTS.md#d17), it is rated Critical, and it produces a wrong
answer with exit code 0 and no diagnostic. A loop variable called `m` binds to
metre; a struct called `s` is unreachable even by an explicit path.

Those names get a face of their own, so in an editor they do not look like names
you introduced:

    Sensor m, s, kg, mol, cd;    // coloured as units -- all five are traps
    Sensor A, K, g;              // coloured as names -- these three are safe

That is the main reason to install this. `A`, `K` and `g` look like unit symbols
but are not in the list, so guessing which names are dangerous does not work.

## Regenerating

The word lists are **read out of the sources**, not retyped:

| List | Read from |
|---|---|
| opcodes, registers, modifiers | `core/include/oblectamenta_decls.ceps` |
| SI unit names | `../ceps/core/src/ceps_interpreter_eval_id.cpp` |
| keywords | `../ceps/core/src/cepslexer.cpp` |

So this expects the `ceps` checkout beside `machines4ceps`, as the `Makefile`
already does. After changing any of those, regenerate and commit the result:

    python3 tools/editor-support/generate.py

That writes all four generated files — `vscode/syntaxes/ceps.tmLanguage.json`,
`emacs/ceps-mode.el`, `vim/syntax/ceps.vim`, `vim/ftdetect/ceps.vim`. Do not
edit them by hand.

The only hand-maintained list is `sm_vocabulary` in the generator — those names
are matched as string literals in the C++ and are not declared anywhere
machine-readable.

## Checking it

Each editor has a checker that drives that editor's **real** highlighting engine
over `sample.ceps` and asserts the same 19 expectations. They exit non-zero when
a scope does not land, so a regression in the generator is caught rather than
noticed later by eye.

    npm install --prefix tools/editor-support/vscode vscode-textmate vscode-oniguruma
    node tools/editor-support/vscode/check-grammar.js

    emacs --batch -l tools/editor-support/emacs/check.el

    sh tools/editor-support/vim/check.sh

`check-grammar.js -v` prints every token with its scopes. `sample.ceps` is the
shared fixture; it exercises every rule and it is a working program —
`bin/ceps tools/editor-support/sample.ceps` counts to 100, prints it, reaches
`Done` and exits 0 — so it cannot rot into something that merely looks right.

## Limits

Highlighting is lexical. It does not know which identifiers are declared, so a
data label is only recognised inside an opcode's argument list, and state names
inside `states{…}` are not distinguished from anything else. Structure names
beyond the hardcoded `sm_vocabulary` list are highlighted generically as struct
headings, which is usually what you want anyway.

`msg` is both an opcode and part of the sm vocabulary; the opcode rule wins in
all three editors. Consistent, if not always what you meant.
