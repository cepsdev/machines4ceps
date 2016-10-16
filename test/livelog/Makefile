includes :=  -I"include" -I"../../../ceps/core/include" -I"../../../ceps/core/include/include-gen" -I"." -I"../../" -I"../../pugixml-1.6/src" -I"../../../log4kmw/include" -I"../../../core/src_gen/logging"
cflags := -O2 -s -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
#cflags := -g3 -O2 -pg -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
#cflags := -g3 -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
TARGET :=
OBJDIR := ./bin
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../../../ceps/core/bin$(TARGET)/libcepscore.a
tutorial_dir := tutorial
cepslibs := ../../../ceps/core/bin
pugisrc = pugixml-1.6/src
log4kmwsrc = ../../../log4kmw/src


all: livelog_source

livelog_source: $(OBJDIR)/main.o $(OBJDIR)/rdwrn.o $(OBJDIR)/sm_livelog_storage_utils.o $(OBJDIR)/livelogger.o
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o \
	$(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o \
	$(cepslibs)/ceps_interpreter_functions.o $(cepslibs)/ceps_interpreter_macros.o $(cepslibs)/pugixml.o $(OBJDIR)/main.o $(OBJDIR)/rdwrn.o $(OBJDIR)/sm_livelog_storage_utils.o $(OBJDIR)/livelogger.o -o livelog_source
$(OBJDIR)/main.o: ../../core/src/livelog/test.cpp 
	$(CXX)   $(cflags) $(includes) -c ../../core/src/livelog/test.cpp -o $(OBJDIR)/main.o
$(OBJDIR)/rdwrn.o: ../../core/src/sockets/rdwrn.cpp 
	$(CXX)   $(cflags) $(includes) -c ../../core/src/sockets/rdwrn.cpp -o $(OBJDIR)/rdwrn.o
$(OBJDIR)/sm_livelog_storage_utils.o: ../../core/src/sm_livelog_storage_utils.cpp 
	$(CXX)   $(cflags) $(includes) -c ../../core/src/sm_livelog_storage_utils.cpp -o $(OBJDIR)/sm_livelog_storage_utils.o
$(OBJDIR)/livelogger.o: ../../core/src/livelog/livelogger.cpp 
	$(CXX)   $(cflags) $(includes) -c ../../core/src/livelog/livelogger.cpp -o $(OBJDIR)/livelogger.o
