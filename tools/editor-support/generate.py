#!/usr/bin/env python3
"""Generate editor syntax definitions for ceps.

One set of word lists, three editors:

    vscode/syntaxes/ceps.tmLanguage.json
    emacs/ceps-mode.el
    vim/syntax/ceps.vim

The word lists this needs already exist in the sources, so they are read from
there rather than retyped:

  opcodes/registers/modifiers  core/include/oblectamenta_decls.ceps
  SI unit names                ../ceps/core/src/ceps_interpreter_eval_id.cpp
  keywords                     ../ceps/core/src/cepslexer.cpp

A retyped list is a list that drifts, and three of them would drift three ways.
Run this after changing any of the above:

    python3 tools/editor-support/generate.py
"""

import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
CEPS = os.path.abspath(os.path.join(REPO, "..", "ceps", "core"))

DECLS = os.path.join(REPO, "core", "include", "oblectamenta_decls.ceps")
EVAL_ID = os.path.join(CEPS, "src", "ceps_interpreter_eval_id.cpp")
LEXER = os.path.join(CEPS, "src", "cepslexer.cpp")


def read(path):
    if not os.path.exists(path):
        sys.exit("generate.py: missing source: %s" % path)
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.read()


def decls_of(text, kind):
    """Names declared as `<kind> a, b, c;` in a .ceps declaration file."""
    names = []
    for m in re.finditer(r"\b%s\b\s*(.*?);" % re.escape(kind), text, re.S):
        body = re.sub(r"//[^\n]*", "", m.group(1))
        names += re.findall(r"[A-Za-z_]\w*", body)
    return names


def si_units(text):
    """The names eval_id turns into a unit-carrying Int."""
    units = []
    for m in re.finditer(r'name\(id\)\s*==\s*"([^"]+)"', text):
        units.append(m.group(1))
    # eval_id's chain ends with names that are not units at all.
    cut = units.index("undef") if "undef" in units else len(units)
    return units[:cut]


def lexer_keywords(text):
    """Words cepslexer.cpp turns into a token before any symbol-table lookup."""
    body = text[text.find('if (s == "struct")'):]
    body = body[: body.find("driver.symboltable().lookup")]
    return re.findall(r's\s*==\s*"([^"]+)"', body)


def alt(words):
    """An Oniguruma alternation; longest first, since alternation is first-match."""
    uniq = sorted(set(words), key=lambda w: (-len(w), w))
    return "|".join(re.escape(w) for w in uniq)


decls_src = read(DECLS)
opcodes = decls_of(decls_src, "OblectamentaOpcode")
registers = decls_of(decls_src, "OblectamentaReg")
modifiers = decls_of(decls_src, "OblectamentaModifier")
kinds = re.findall(r"\bkind\s+([A-Za-z_]\w*)\s*;", decls_src)

units = si_units(read(EVAL_ID))
keywords = lexer_keywords(read(LEXER))

control = [k for k in keywords if k in ("if", "else", "for", "static_for", "return")]
declaration = [k for k in keywords if k not in control]

# Structure names the C++ side looks for by name. Unlike the above these are not
# declared anywhere machine-readable, so they are listed here; they are ordinary
# structs, hence `support`, not `keyword`.
sm_vocabulary = [
    "sm", "states", "Actions", "Guards", "Simulation", "Start", "Thread",
    "on_enter", "on_exit", "transition", "t", "Events",
    "oblectamenta", "global", "data", "text", "asm", "x64", "msg",
]

grammar = {
    "$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json",
    "name": "ceps",
    "scopeName": "source.ceps",
    "fileTypes": ["ceps"],
    "patterns": [
        {"include": "#comments"},
        {"include": "#strings"},
        {"include": "#kind-declaration"},
        {"include": "#keywords"},
        {"include": "#si-units"},
        {"include": "#registers"},
        {"include": "#opcodes"},
        {"include": "#sm-vocabulary"},
        {"include": "#symbol-declaration"},
        {"include": "#struct-head"},
        {"include": "#numbers"},
        {"include": "#operators"},
    ],
    "repository": {
        "comments": {
            "patterns": [
                {
                    "name": "comment.line.double-slash.ceps",
                    "begin": "//",
                    "end": "$",
                },
                {
                    "name": "comment.block.ceps",
                    "begin": "/\\*",
                    "end": "\\*/",
                },
            ]
        },
        "strings": {
            "patterns": [
                {
                    "name": "string.quoted.double.ceps",
                    "begin": "\"",
                    "end": "\"",
                    "patterns": [
                        {
                            "name": "constant.character.escape.ceps",
                            "match": "\\\\.",
                        }
                    ],
                }
            ]
        },
        "kind-declaration": {
            "match": "\\b(kind)\\s+([A-Za-z_]\\w*)",
            "captures": {
                "1": {"name": "keyword.declaration.kind.ceps"},
                "2": {"name": "entity.name.type.ceps"},
            },
        },
        "keywords": {
            "patterns": [
                {
                    "name": "keyword.control.ceps",
                    "match": "\\b(%s)\\b" % alt(control),
                },
                {
                    "name": "keyword.declaration.ceps",
                    "match": "\\b(%s)\\b" % alt(declaration),
                },
            ]
        },
        # Any identifier in this list silently becomes a unit-carrying value --
        # see DEFECTS.md D17. Scoped so that it cannot be mistaken for a name
        # the author introduced.
        "si-units": {
            "name": "support.constant.si-unit.ceps",
            "match": "\\b(%s)\\b" % alt(units),
        },
        "registers": {
            "name": "variable.language.register.ceps",
            "match": "\\b(%s)\\b" % alt(registers + modifiers),
        },
        "opcodes": {
            "patterns": [
                {
                    "begin": "\\b(%s)\\s*(\\()" % alt(opcodes),
                    "beginCaptures": {
                        "1": {"name": "support.function.opcode.ceps"},
                        "2": {"name": "punctuation.section.arguments.begin.ceps"},
                    },
                    "end": "\\)",
                    "endCaptures": {
                        "0": {"name": "punctuation.section.arguments.end.ceps"}
                    },
                    "patterns": [
                        {"include": "#si-units"},
                        {"include": "#registers"},
                        {"include": "#numbers"},
                        {"include": "#strings"},
                        {
                            "name": "variable.other.label.ceps",
                            "match": "[A-Za-z_]\\w*",
                        },
                    ],
                },
                {
                    "name": "support.function.opcode.ceps",
                    "match": "\\b(%s)\\b" % alt(opcodes),
                },
            ]
        },
        "sm-vocabulary": {
            "name": "support.class.sm.ceps",
            "match": "\\b(%s)\\b(?=\\s*\\{)" % alt(sm_vocabulary),
        },
        # `Kind a, b, c;` -- a declaration, whatever the kind happens to be. The
        # names it introduces are scoped too, so that a name which is really a
        # unit (D17) shows up as one at the point it is declared.
        "symbol-declaration": {
            "begin": "\\b([A-Z]\\w*)(?=\\s+[A-Za-z_]\\w*\\s*(?:,\\s*[A-Za-z_]\\w*\\s*)*;)",
            "beginCaptures": {"1": {"name": "storage.type.ceps"}},
            "end": ";",
            "patterns": [
                {"include": "#si-units"},
                {
                    "name": "entity.name.variable.ceps",
                    "match": "[A-Za-z_]\\w*",
                },
            ],
        },
        "struct-head": {
            "match": "(?<![\\w.])([A-Za-z_]\\w*)\\s*(?=\\{)",
            "captures": {"1": {"name": "entity.name.tag.ceps"}},
        },
        "numbers": {
            "patterns": [
                {
                    "name": "constant.numeric.float.ceps",
                    "match": "\\b\\d+\\.\\d*(?:[eE][-+]?\\d+)?\\b|\\.\\d+(?:[eE][-+]?\\d+)?\\b",
                },
                {
                    "name": "constant.numeric.integer.ceps",
                    "match": "\\b\\d+\\b",
                },
            ]
        },
        "operators": {
            "name": "keyword.operator.ceps",
            "match": "<=|>=|==|!=|->|<-|\\.\\.|[-+*/^~=<>|&!.:#]",
        },
    },
}

out_tm = os.path.join(HERE, "vscode", "syntaxes", "ceps.tmLanguage.json")
with open(out_tm, "w", encoding="utf-8") as f:
    json.dump(grammar, f, indent=2)
    f.write("\n")


# ---------------------------------------------------------------- emacs ----

def wrap(words, per_line=6, indent=4):
    """A quoted word list, wrapped, for embedding in generated source."""
    uniq = sorted(set(words))
    rows = [uniq[i:i + per_line] for i in range(0, len(uniq), per_line)]
    pad = " " * indent
    return ("\n" + pad).join(" ".join('"%s"' % w for w in row) for row in rows)


ELISP = r""";;; ceps-mode.el --- Major mode for the ceps language -*- lexical-binding: t; -*-

;; GENERATED by tools/editor-support/generate.py -- do not edit by hand.
;; Regenerate with: python3 tools/editor-support/generate.py

;;; Commentary:

;; Syntax highlighting for .ceps files, covering the language proper and the
;; Oblectamenta assembler.
;;
;; The face `ceps-si-unit-face' marks the fourteen names that silently resolve
;; to SI units rather than to whatever you meant -- see DEFECTS.md D17.  That
;; defect produces a wrong answer with exit code 0 and no diagnostic, so the
;; names are given a face of their own and deliberately do not look like names
;; you introduced.

;;; Code:

(defgroup ceps nil
  "Major mode for the ceps language."
  :group 'languages
  :prefix "ceps-")

(defface ceps-si-unit-face
  '((t :inherit font-lock-warning-face))
  "Face for names that silently resolve to an SI unit (DEFECTS.md D17).
A variable named `m' binds to metre, not to anything you declared."
  :group 'ceps)

(defconst ceps-control-keywords
  '(%(control)s)
  "Keywords the ceps lexer turns into a control token.")

(defconst ceps-declaration-keywords
  '(%(declaration)s)
  "Keywords the ceps lexer turns into a declaration token.")

(defconst ceps-si-units
  '(%(units)s)
  "Names that silently resolve to an SI unit.  See DEFECTS.md D17.")

(defconst ceps-registers
  '(%(registers)s)
  "Oblectamenta registers and operand modifiers.")

(defconst ceps-opcodes
  '(%(opcodes)s)
  "Oblectamenta opcodes.")

(defconst ceps-sm-vocabulary
  '(%(smvocab)s)
  "Structure names the state machine implementation looks for by name.")

(defconst ceps-font-lock-keywords
  `(;; kind X;
    ("\\_<\\(kind\\)\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\)"
     (1 font-lock-keyword-face) (2 font-lock-type-face))

    (,(regexp-opt ceps-control-keywords 'symbols) . font-lock-keyword-face)
    (,(regexp-opt ceps-declaration-keywords 'symbols) . font-lock-keyword-face)

    ;; D17.  Deliberately ahead of every rule that could claim these names,
    ;; including the declaration rule below: a name that is really a unit must
    ;; not be coloured as a name you introduced.
    (,(regexp-opt ceps-si-units 'symbols) . 'ceps-si-unit-face)

    (,(regexp-opt ceps-registers 'symbols) . font-lock-constant-face)
    (,(regexp-opt ceps-opcodes 'symbols) . font-lock-function-name-face)

    ;; sm{ states{ Actions{ t{ ...
    (,(concat "\\_<" (regexp-opt ceps-sm-vocabulary t) "\\_>\\s-*{")
     (1 font-lock-builtin-face))

    ;; Kind a, b, c;  -- the kind, then the names it introduces.
    ("\\_<\\([A-Z][A-Za-z0-9_]*\\)\\(\\s-+[A-Za-z_][A-Za-z0-9_]*\\s-*\\(?:,\\s-*[A-Za-z_][A-Za-z0-9_]*\\s-*\\)*\\);"
     (1 font-lock-type-face)
     ("[A-Za-z_][A-Za-z0-9_]*"
      (progn (goto-char (match-beginning 2)) (match-end 2))
      nil
      (0 font-lock-variable-name-face)))

    ;; Any other name heading a struct.
    ("\\(?:^\\|[^.[:alnum:]_]\\)\\([A-Za-z_][A-Za-z0-9_]*\\)\\s-*{"
     (1 font-lock-function-name-face))

    ("\\_<[0-9]+\\.[0-9]*\\(?:[eE][-+]?[0-9]+\\)?" . font-lock-constant-face)
    ("\\_<[0-9]+\\_>" . font-lock-constant-face))
  "Font lock keywords for `ceps-mode'.")

(defvar ceps-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; // line comments and /* block */ comments, as in C.
    (modify-syntax-entry ?/ ". 124b" table)
    (modify-syntax-entry ?* ". 23" table)
    (modify-syntax-entry ?\n "> b" table)
    (modify-syntax-entry ?_ "_" table)
    (modify-syntax-entry ?\" "\"" table)
    (modify-syntax-entry ?\\ "\\" table)
    table)
  "Syntax table for `ceps-mode'.")

;;;###autoload
(define-derived-mode ceps-mode prog-mode "ceps"
  "Major mode for editing ceps files."
  :syntax-table ceps-mode-syntax-table
  (setq-local font-lock-defaults '(ceps-font-lock-keywords))
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "//+\\s-*")
  (setq-local indent-tabs-mode nil))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.ceps\\'" . ceps-mode))

(provide 'ceps-mode)

;;; ceps-mode.el ends here
"""

out_el = os.path.join(HERE, "emacs", "ceps-mode.el")
with open(out_el, "w", encoding="utf-8") as f:
    f.write(ELISP % {
        "control": wrap(control),
        "declaration": wrap(declaration),
        "units": wrap(units),
        "registers": wrap(registers + modifiers),
        "opcodes": wrap(opcodes, per_line=5),
        "smvocab": wrap(sm_vocabulary),
    })


# ------------------------------------------------------------------ vim ----

def vim_keyword(group, words, per_line=8):
    uniq = sorted(set(words))
    rows = [uniq[i:i + per_line] for i in range(0, len(uniq), per_line)]
    return "\n".join("syn keyword %s %s" % (group, " ".join(r)) for r in rows)


def vim_match_before_brace(group, words):
    """Like vim_keyword, but only where the word heads a block."""
    alternation = "\\|".join(sorted(set(words), key=lambda w: (-len(w), w)))
    return 'syn match %s "\\<\\%%(%s\\)\\>\\ze\\s*{"' % (group, alternation)


VIM = r'''" Vim syntax file for the ceps language.
" GENERATED by tools/editor-support/generate.py -- do not edit by hand.
" Regenerate with: python3 tools/editor-support/generate.py

if exists("b:current_syntax")
  finish
endif

syn case match

" --- comments and strings -------------------------------------------------
syn keyword cepsTodo contained TODO FIXME XXX NOTE
syn match   cepsComment "//.*$" contains=cepsTodo
syn region  cepsComment start="/\*" end="\*/" contains=cepsTodo
syn match   cepsEscape  contained "\\."
syn region  cepsString  start=+"+ skip=+\\.+ end=+"+ contains=cepsEscape

" --- numbers --------------------------------------------------------------
syn match cepsNumber "\<\d\+\>"
syn match cepsFloat  "\<\d\+\.\d*\([eE][-+]\=\d\+\)\="

" --- keywords -------------------------------------------------------------
@@keywords@@

" --- names that are really SI units ---------------------------------------
" DEFECTS.md D17: these fourteen names silently resolve to a unit rather than
" to whatever you meant, with exit code 0 and no diagnostic. They are linked to
" Todo rather than to a quiet group on purpose -- being hard to overlook is the
" entire point of highlighting them.
@@units@@

" --- Oblectamenta ---------------------------------------------------------
@@opcodes@@
@@registers@@

" --- structure ------------------------------------------------------------
" Order matters here: when two matches start at the same position vim gives
" priority to the one defined last, so the specific rules follow the general.
syn match cepsStructHead "\<[A-Za-z_]\w*\ze\s*{"
@@smvocab@@

syn match cepsKindName "\%(\<kind\s\+\)\@<=[A-Za-z_]\w*"

" A declaration, `Kind a, b, c;`. This has to be a container rather than one
" match: a name that is really a unit must keep its own group, which it would
" not if the whole declaration were a single match.
syn match cepsDecl "\<[A-Z]\w*\s\+[A-Za-z_]\w*\%(\s*,\s*[A-Za-z_]\w*\)*\s*;"
      \ contains=cepsSIUnit,cepsDeclName,cepsDeclKind
syn match cepsDeclName contained "\<[A-Za-z_]\w*\>"
syn match cepsDeclKind contained "\<[A-Z]\w*\%(\s\+[A-Za-z_]\)\@="

" --- operators ------------------------------------------------------------
syn match cepsOperator "<=\|>=\|==\|!=\|->\|<-\|\.\.\|[-+*/^~=<>|&!.:#]"

" --- linkage --------------------------------------------------------------
hi def link cepsComment    Comment
hi def link cepsTodo       Todo
hi def link cepsString     String
hi def link cepsEscape     SpecialChar
hi def link cepsNumber     Number
hi def link cepsFloat      Float
hi def link cepsKeyword    Keyword
hi def link cepsSIUnit     Todo
hi def link cepsOpcode     Function
hi def link cepsRegister   Constant
hi def link cepsSMKeyword  Structure
hi def link cepsKindName   Type
hi def link cepsDeclKind   Type
hi def link cepsDeclName   Identifier
hi def link cepsStructHead Function
hi def link cepsOperator   Operator

let b:current_syntax = "ceps"
'''

out_vim = os.path.join(HERE, "vim", "syntax", "ceps.vim")
with open(out_vim, "w", encoding="utf-8") as f:
    text = VIM
    for placeholder, value in (
        ("@@keywords@@", vim_keyword("cepsKeyword", control + declaration)),
        ("@@units@@", vim_keyword("cepsSIUnit", units)),
        ("@@opcodes@@", vim_keyword("cepsOpcode", opcodes, per_line=6)),
        ("@@registers@@", vim_keyword("cepsRegister", registers + modifiers)),
        ("@@smvocab@@", vim_match_before_brace("cepsSMKeyword", sm_vocabulary)),
    ):
        text = text.replace(placeholder, value)
    f.write(text)

out_ftd = os.path.join(HERE, "vim", "ftdetect", "ceps.vim")
with open(out_ftd, "w", encoding="utf-8") as f:
    f.write('" GENERATED by tools/editor-support/generate.py\n'
            "autocmd BufRead,BufNewFile *.ceps set filetype=ceps\n")


# --------------------------------------------------------------- summary ----

for p in (out_tm, out_el, out_vim, out_ftd):
    print("wrote %s" % os.path.relpath(p, REPO))
print("  %3d opcodes, %2d registers/modifiers, %2d kinds"
      % (len(set(opcodes)), len(set(registers + modifiers)), len(set(kinds))))
print("  %3d keywords, %2d SI unit names" % (len(set(keywords)), len(set(units))))
