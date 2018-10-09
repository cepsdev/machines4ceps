#
# sm4ceps - Makefile
#

includes := -I"." -I"../.." -I"../../../ceps/core/include"
cflags := -g3 -Wall -MD -fmessage-length=0 -std=c++1y 
cepslibs := ../../../ceps/core/bin


all: antriebe.so

antriebe.so: Antriebe.o
	$(CXX)   $(cflags) \
        $(cepslibs)/ceps_ast.o \
        $(cepslibs)/ceps.tab.o \
        $(cepslibs)/ceps_interpreter.o \
        $(cepslibs)/cepsparserdriver.o \
        $(cepslibs)/cepsruntime.o \
        $(cepslibs)/cepslexer.o \
        $(cepslibs)/symtab.o \
        $(cepslibs)/ceps_interpreter_loop.o \
        $(cepslibs)/ceps_interpreter_nodeset.o \
        $(cepslibs)/ceps_interpreter_macros.o \
        $(cepslibs)/ceps_interpreter_functions.o -shared  Antriebe.o -o Antriebe.so
Antriebe.o: Antriebe.cpp
	$(CXX)   $(cflags) -fPIC  $(includes) -c Antriebe.cpp -o Antriebe.o

clean:

	rm Antriebe.o
	rm Antriebe.so 

