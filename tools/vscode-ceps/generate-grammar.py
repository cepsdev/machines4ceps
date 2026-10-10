#!/usr/bin/env python3
"""Generate syntaxes/ceps.tmLanguage.json.

The word lists this grammar needs already exist in the sources, so they are read
from there rather than retyped:

  opcodes/registers/modifiers  core/include/oblectamenta_decls.ceps
  SI unit names                ../ceps/core/src/ceps_interpreter_eval_id.cpp
  keywords                     ../ceps/core/src/cepslexer.cpp

A retyped list is a list that drifts. Run this after changing any of them:

    python3 tools/vscode-ceps/generate-grammar.py
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
        sys.exit("generate-grammar.py: missing source: %s" % path)
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

out = os.path.join(HERE, "syntaxes", "ceps.tmLanguage.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(grammar, f, indent=2)
    f.write("\n")

print("wrote %s" % os.path.relpath(out, REPO))
print("  %3d opcodes, %2d registers/modifiers, %2d kinds"
      % (len(set(opcodes)), len(set(registers + modifiers)), len(set(kinds))))
print("  %3d keywords, %2d SI unit names" % (len(set(keywords)), len(set(units))))
print("  units: %s" % " ".join(sorted(set(units))))
