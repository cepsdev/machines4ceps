#
# sm4ceps - Makefile
#

includes :=  -I"include" -I"../../../ceps/core/include" -I"../../../ceps/core/include/include-gen" -I"." -I"../.."
cflags := -g3 -Wall -MD -fmessage-length=0 -std=c++1y -L"../../x86" -L"../../../ceps/core/bin"
OBJDIR := bin
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../../../ceps/core/bin/libcepscore.a
cepslibs := ../../../ceps/core/bin
sm4cepsobjectfiles :=../../x86/sm_sim_core_asserts.o ../../x86/sm_sim_core_simulation_loop.o ../../x86/cmdline_utils.o ../../x86/serialization.o ../../x86/state_machine_simulation_core_event_handling.o ../../x86/state_machine_simulation_core_guard_handling.o ../../x86/cal_sender.o ../../x86/cal_receiver.o  ../../x86/state_machines.o ../../x86/sm_xml_frame.o ../../x86/sm_raw_frame.o ../../x86/sm_comm_naive_msg_prot.o ../../x86/state_machine_simulation_core.o ../../x86/state_machine_simulation_core_action_handling.o

all: antriebe.so

antriebe.so: Antriebe.o
	$(CXX)   $(cflags) -shared  Antriebe.o $(sm4cepsobjectfiles) -lcepscore -o Antriebe.so
Antriebe.o: Antriebe.cpp
	$(CXX)   $(cflags) -fPIC  $(includes) -c Antriebe.cpp -o Antriebe.o

clean:

	rm Antriebe.o
	rm Antriebe.so 

