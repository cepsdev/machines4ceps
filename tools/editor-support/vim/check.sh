#!/bin/sh
# Run check.vim against the fixture. Pass -v to dump every token's groups.
#
#     sh tools/editor-support/vim/check.sh
set -e

here=$(cd "$(dirname "$0")" && pwd)
sample="${2:-$here/../sample.ceps}"

verbose=''
[ "$1" = "-v" ] && verbose="--cmd 'let g:ceps_check_verbose = 1'"

exec vim -es -u NONE -N -i NONE \
    ${verbose:+--cmd} ${verbose:+"let g:ceps_check_verbose = 1"} \
    --cmd "set rtp^=$here" \
    -c 'syntax enable' \
    -c 'set filetype=ceps' \
    -c "source $here/check.vim" \
    "$sample"
