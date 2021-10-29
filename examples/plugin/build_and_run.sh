#!/bin/bash
rm *.o ; rm *.so ; rm*.d ; make -B && LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH ceps --plugin./libev2sctp.so example.ceps   --format ansi --doc-option state-machines-show-only-states --quiet
