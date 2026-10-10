#!/usr/bin/env python3
"""Run the ceps examples embedded in Markdown and check their stated output.

Documentation that quotes output nobody re-runs is a specification that drifts,
which is the failure mode this project exists to oppose. This script makes the
quoted output load-bearing.

A checked example is a ``ceps`` fence immediately followed by an ``output``
fence::

    ```ceps
    a{1;};
    ```
    ```output
    (STRUCT "a" (INT 1))
    ```

The fence info string selects how the snippet is run:

    ```ceps          evaluate and print the document   (ceps --pe FILE)
    ```ceps run      run it, including any Simulation  (ceps FILE)

A ``ceps`` fence with no ``output`` fence after it is *not* checked. Those are
counted and listed, because an unchecked example is exactly the kind of silent
omission this project is about.

Comparison ignores indentation, runs of whitespace and blank lines, and masks
hexadecimal addresses as ``<ADDR>``; ceps emits them for macros and they differ
per run. Line *breaks* are significant: the documented output should have the
same line structure the tool produces, so that the page shows what a reader
would actually see.

Usage:
    tools/check-doc-examples.py [FILE.md ...]      # defaults to the set below
    tools/check-doc-examples.py --list             # show what would be checked
"""

import difflib
import os
import re
import subprocess
import sys
import tempfile

DEFAULT_DOCS = [
    "CEPS-LANG.md",
    "doc/scribble-concept/README.md",
    "doc/model-revision/README.md",
]

FENCE = re.compile(
    r"^```ceps(?P<info>[^\n]*)\n(?P<code>.*?)^```[ \t]*\n"
    r"(?:[ \t]*\n)*"
    r"^```output[^\n]*\n(?P<want>.*?)^```[ \t]*$",
    re.MULTILINE | re.DOTALL,
)
ANY_CEPS_FENCE = re.compile(r"^```ceps(?P<info>[^\n]*)$", re.MULTILINE)
HEX = re.compile(r"0x[0-9a-fA-F]+")


def repo_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def ceps_binary():
    path = os.path.join(repo_root(), "bin", "ceps")
    if not os.access(path, os.X_OK):
        sys.exit("error: %s is missing or not executable; build it first" % path)
    return path


def normalise(text):
    out = []
    for line in text.splitlines():
        line = HEX.sub("<ADDR>", line)
        line = " ".join(line.split())
        line = re.sub(r"\s+\)", ")", line)
        line = re.sub(r"\(\s+", "(", line)
        if line:
            out.append(line)
    return out


def line_of(text, offset):
    return text.count("\n", 0, offset) + 1


def run_snippet(binary, code, mode):
    with tempfile.NamedTemporaryFile("w", suffix=".ceps", delete=False) as handle:
        handle.write(code)
        name = handle.name
    try:
        argv = [binary, name] if mode == "run" else [binary, "--pe", name]
        done = subprocess.run(argv, capture_output=True, text=True, timeout=60)
        return done.returncode, done.stdout + done.stderr
    finally:
        os.unlink(name)


def check_file(binary, path, listing=False):
    with open(path, encoding="utf-8") as handle:
        text = handle.read()

    checked = [m for m in FENCE.finditer(text)]
    covered = {m.start() for m in checked}
    unchecked = [m for m in ANY_CEPS_FENCE.finditer(text) if m.start() not in covered]

    failures = 0
    for match in checked:
        line = line_of(text, match.start())
        mode = (match.group("info") or "").strip() or "pe"
        if mode not in ("pe", "run"):
            print("%s:%d: unknown fence mode %r" % (path, line, mode))
            failures += 1
            continue
        if listing:
            print("%s:%d: %s" % (path, line, mode))
            continue
        code = match.group("code")
        rc, got = run_snippet(binary, code, mode)
        if rc != 0:
            print("%s:%d: ceps exited %d" % (path, line, rc))
            print("\n".join("    " + l for l in got.splitlines()[:20]))
            failures += 1
            continue
        want_lines, got_lines = normalise(match.group("want")), normalise(got)
        if want_lines != got_lines:
            print("%s:%d: output does not match" % (path, line))
            diff = difflib.unified_diff(
                want_lines, got_lines, "documented", "actual", lineterm=""
            )
            print("\n".join("    " + l for l in diff))
            failures += 1

    return len(checked), unchecked, failures


def main(argv):
    listing = "--list" in argv
    docs = [a for a in argv if not a.startswith("-")] or DEFAULT_DOCS
    root = repo_root()
    binary = ceps_binary()

    total_checked = 0
    total_unchecked = []
    total_failures = 0

    for doc in docs:
        path = doc if os.path.isabs(doc) else os.path.join(root, doc)
        if not os.path.exists(path):
            print("%s: missing" % doc)
            total_failures += 1
            continue
        rel = os.path.relpath(path, root)
        if rel.startswith(".."):
            rel = path
        n, unchecked, failures = check_file(binary, path, listing)
        total_checked += n
        total_unchecked += [(rel, line_of(open(path, encoding="utf-8").read(), m.start()))
                            for m in unchecked]
        total_failures += failures

    print()
    print("checked   %d example(s)" % total_checked)
    print("unchecked %d ceps block(s) with no documented output" % len(total_unchecked))
    for rel, line in total_unchecked:
        print("          %s:%d" % (rel, line))
    if total_failures:
        print("FAILED    %d example(s)" % total_failures)
    return 1 if total_failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
