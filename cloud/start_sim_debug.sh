#!/bin/sh
if [ "$#" -ne 2 ]; then
    echo "SYNOPSIS\n\t $0 apu|tcs simulation\nEXAMPLE\n\t $0 tcs sim1"
    exit
fi

gdb --args ../x86/ceps .ceps/prelude.ceps sim_nodes/$1/observables.ceps sim_nodes/$1/frames.ceps sim_nodes/$1/comm_tx.ceps sim_nodes/$1/encodings.ceps sim_nodes/$1/constraints.ceps sim_nodes/$1/sim/$2.ceps --quiet
