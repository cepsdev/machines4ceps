#!/bin/bash
rm *.o ; rm *.so ; rm*.d ; make && ceps --plugin./libev2sctp.so example.ceps
