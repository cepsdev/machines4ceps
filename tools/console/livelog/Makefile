includes :=  -I"include" -I"../../../../ceps/core/include" -I"../../../../ceps/core/include/include-gen" -I"." -I"../../../" -I"../../../pugixml-1.6/src" -I"../../../../log4kmw/include" -I"../../../../core/src_gen/logging"
cflags := -O2 -s -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
#cflags := -g3 -O2 -pg -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
#cflags := -g3 -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
TARGET :=
OBJDIR := ./bin
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../ceps/core/bin$(TARGET)/libcepscore.a
tutorial_dir := tutorial
cepslibs := ../../../../ceps/core/bin
pugisrc = pugixml-1.6/src
log4kmwsrc = ../log4kmw/src


all: livelog

livelog: $(OBJDIR)/main.o 
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o \
	$(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o \
	$(cepslibs)/pugixml.o $(OBJDIR)/main.o -o livelog
$(OBJDIR)/main.o: src/main.cpp
	$(CXX)   $(cflags) $(includes) -c src/main.cpp -o $(OBJDIR)/main.o


