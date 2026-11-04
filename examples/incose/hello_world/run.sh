#!/bin/sh
#
# run.sh - derive a state machine from an INCOSE/EARS requirements document and run it.
#
#   ./run.sh            the conforming document
#   ./run.sh --bad      additionally load nonconforming.req, to show the report
#   ./run.sh --model    print the derived model instead of running it
#
# See SPEC.md for what any of this means.

set -e

CEPS="${CEPS:-}"
if [ -z "$CEPS" ]; then
  if [ -x ../../../bin/ceps ]; then
    CEPS=../../../bin/ceps
  elif command -v ceps >/dev/null 2>&1; then
    CEPS=ceps
  else
    echo "ceps not found. Build it with ./make_machines.sh, or set CEPS=/path/to/ceps." >&2
    exit 1
  fi
fi

DOCS="door_lock.req"
MODE=""

for arg in "$@"; do
  case "$arg" in
    --bad)   DOCS="door_lock.req nonconforming.req" ;;
    --model) MODE="--pe" ;;
    *)       echo "unknown option: $arg" >&2; exit 2 ;;
  esac
done

if [ -n "$MODE" ]; then
  # The derived model, without executing it.
  exec "$CEPS" ears.ceps.lex $DOCS extract.ceps --pe
fi

exec "$CEPS" ears.ceps.lex $DOCS extract.ceps simulate.ceps
