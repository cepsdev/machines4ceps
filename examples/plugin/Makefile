cflags := -g3  -Wall -MD  -std=c++17  -fPIC
CEPSOBJDIR := ../../../ceps/core/bin
objfiles_ceps :=   ceps_ast.o ceps_interpreter.o ceps_interpreter_functions.o  
objfiles := $(patsubst %,$(CEPSOBJDIR)/%,$(objfiles_ceps))


all: libev2sctp.so

libev2sctp.so: $(objfiles) ev2sctp.o
	g++ $(cflags) -shared  -o libev2sctp.so ev2sctp.o $(objfiles)  
ev2sctp.o:  ev2sctp.cpp
	g++  $(cflags) -I"../../../log4ceps/include" -I"../../core/src_gen/logging" -I"../../../ceps/core/include" -I"../../" ev2sctp.cpp -c   -o ev2sctp.o

