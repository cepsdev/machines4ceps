#
# sm4ceps - Makefile
#

includes :=  -I"include" -I"../../../ceps/core/include" -I"../../../ceps/core/include/include-gen" -I"." -I"../.."
cflags := -g3 -Wall -MD -fmessage-length=0 -std=c++1y -L"../../x86" -L"../../../ceps/core/bin"
OBJDIR := bin
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../../../ceps/core/bin/libcepscore.a
cepslibs := ../../../ceps/core/bin

all: antriebe.so

antriebe.so: Antriebe.o
	$(CXX)   $(cflags) -shared  Antriebe.o ../../x86/state_machine_simulation_core_action_handling.o -lcepscore -o Antriebe.so
Antriebe.o: Antriebe.cpp
	$(CXX)   $(cflags) -fPIC  $(includes) -c Antriebe.cpp -o Antriebe.o

clean:

	rm Antriebe.o
	rm Antriebe.so 

