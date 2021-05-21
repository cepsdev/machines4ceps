#!/bin/bash
g++ -std=c++2a src/main.cpp -I include/ -o pltsq && ./pltsq > test.html && cat test.html
