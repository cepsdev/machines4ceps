#
# sm4ceps - Makefile
#

includes :=  -I"include" -I"../../../ceps/core/include" -I"../../../ceps/core/include/include-gen" -I"." -I"../.."
cflags := -O0 -g3 -Wall -MD -fmessage-length=0 -std=c++1y 
OBJDIR := bin
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../../../ceps/core/bin/libcepscore.a
cepslibs := ../../../ceps/core/bin

all: simple_plugin.so

simple_plugin.so: simple_plugin.o
	$(CXX)   $(cflags) -shared $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o $(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o simple_plugin.o -o simple_plugin.so
simple_plugin.o: simple_plugin.cpp
	$(CXX)   $(cflags)-fPIC $(includes) -c simple_plugin.cpp -o simple_plugin.o

clean:

	rm simple_plugin.o
	rm simple_plugin.so 

