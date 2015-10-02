#
# sm4ceps - Makefile
#

includes :=  -I"include" -I"../ceps/core/include" -I"../ceps/core/include/include-gen" -I"."
cflags := -O0 -g3 -Wall -MD -fmessage-length=0 -std=c++1y
OBJDIR := bin
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../ceps/core/bin/libcepscore.a
tutorial_dir := tutorial
cepslibs := ../ceps/core/bin

all: x86/sm2plantuml x86/sm4ceps

x86/sm2plantuml:x86/serialization.o  x86/main.o x86/state_machines.o x86/sm_sim_core_asserts.o x86/state_machine_simulation_core.o x86/sm_sim_core_simulation_loop.o x86/state_machine_simulation_core_action_handling.o x86/state_machine_simulation_core_event_handling.o x86/state_machine_simulation_core_guard_handling.o  x86/cmdline_utils.o x86/sm_comm_naive_msg_prot.o x86/sm_raw_frame.o
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o $(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o x86/main.o x86/state_machine_simulation_core.o  x86/state_machines.o x86/cmdline_utils.o  x86/state_machine_simulation_core_action_handling.o x86/state_machine_simulation_core_event_handling.o x86/state_machine_simulation_core_guard_handling.o x86/sm_raw_frame.o x86/sm_sim_core_simulation_loop.o x86/sm_sim_core_asserts.o x86/sm_comm_naive_msg_prot.o x86/serialization.o -o x86/sm2plantuml

x86/sm4ceps: x86/serialization.o x86/main.o x86/state_machines.o x86/sm_sim_core_asserts.o x86/state_machine_simulation_core.o x86/sm_sim_core_simulation_loop.o x86/state_machine_simulation_core_action_handling.o x86/state_machine_simulation_core_event_handling.o x86/state_machine_simulation_core_guard_handling.o x86/cmdline_utils.o x86/sm_raw_frame.o
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o $(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o x86/main.o x86/state_machine_simulation_core.o  x86/state_machines.o x86/state_machine_simulation_core_action_handling.o x86/state_machine_simulation_core_event_handling.o x86/state_machine_simulation_core_guard_handling.o x86/sm_sim_core_simulation_loop.o x86/cmdline_utils.o x86/sm_sim_core_asserts.o x86/sm_comm_naive_msg_prot.o x86/serialization.o x86/sm_raw_frame.o -o x86/sm4ceps

x86/main.o:
	$(CXX)   $(cflags) $(includes) src/main.cpp -c -o x86/main.o

x86/state_machine_simulation_core.o: core/src/state_machine_simulation_core.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core.cpp -c -o x86/state_machine_simulation_core.o
x86/state_machine_simulation_core_action_handling.o: core/src/state_machine_simulation_core_action_handling.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_action_handling.cpp -c -o x86/state_machine_simulation_core_action_handling.o
x86/state_machine_simulation_core_event_handling.o: core/src/state_machine_simulation_core_event_handling.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_event_handling.cpp -c -o x86/state_machine_simulation_core_event_handling.o
x86/state_machine_simulation_core_guard_handling.o: core/src/state_machine_simulation_core_guard_handling.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_guard_handling.cpp -c -o x86/state_machine_simulation_core_guard_handling.o
x86/sm_sim_core_simulation_loop.o: core/src/sm_sim_core_simulation_loop.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_core_simulation_loop.cpp -c -o x86/sm_sim_core_simulation_loop.o
x86/sm_sim_core_asserts.o: core/src/sm_sim_core_asserts.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_core_asserts.cpp -c -o x86/sm_sim_core_asserts.o
x86/sm_comm_naive_msg_prot.o: core/src/sm_comm_naive_msg_prot.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_comm_naive_msg_prot.cpp -c -o x86/sm_comm_naive_msg_prot.o
x86/serialization.o: core/src/serialization.cpp
	$(CXX)   $(cflags) $(includes) core/src/serialization.cpp -c -o x86/serialization.o  
x86/state_machines.o: core/src/state_machines.cpp 
	$(CXX)   $(cflags) $(includes) core/src/state_machines.cpp -c -o x86/state_machines.o

x86/cmdline_utils.o:
	$(CXX)   $(cflags) $(includes) core/src/cmdline_utils.cpp -c -o x86/cmdline_utils.o
x86/sm_raw_frame.o:
	$(CXX)   $(cflags) $(includes) core/src/sm_raw_frame.cpp -c -o x86/sm_raw_frame.o
clean:
	rm x86/*

