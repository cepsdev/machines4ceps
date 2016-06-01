#
# sm4ceps - Makefile
#

includes :=  -I"include" -I"../ceps/core/include" -I"../ceps/core/include/include-gen" -I"." -I"pugixml-1.6/src" -I"../log4kmw/include" -I"core/src_gen/logging"
cflags := -O0 -g3 -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -fPIC -Wall 
TARGET :=
OBJDIR := bin/$(TARGET)
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../ceps/core/bin$(TARGET)/libcepscore.a
tutorial_dir := tutorial
cepslibs := ../ceps/core/$(OBJDIR)
pugisrc = pugixml-1.6/src
log4kmwsrc = ../log4kmw/src


all: $(TARGET)/sm $(TARGET)/sm_trace 

$(TARGET)/sm: $(TARGET)/serialization.o $(TARGET)/main.o $(TARGET)/state_machines.o $(TARGET)/sm_sim_core_asserts.o $(TARGET)/state_machine_simulation_core.o \
$(TARGET)/sm_sim_core_simulation_loop.o $(TARGET)/state_machine_simulation_core_action_handling.o $(TARGET)/state_machine_simulation_core_event_handling.o \
$(TARGET)/state_machine_simulation_core_guard_handling.o $(TARGET)/cmdline_utils.o $(TARGET)/sm_raw_frame.o $(TARGET)/sm_xml_frame.o $(TARGET)/pugixml.o $(TARGET)/cal_sender.o $(TARGET)/cal_receiver.o $(TARGET)/state_machine_simulation_core_buildsms.o \
$(TARGET)/log4kmw_events.o $(TARGET)/log4kmw_loggers.o $(TARGET)/log4kmw_records.o $(TARGET)/log4kmw_states.o $(TARGET)/log4kmw_serialization.o \
$(TARGET)/log4kmw_dynamic_bitset.o $(TARGET)/log4kmw_record.o $(TARGET)/log4kmw_utils.o $(TARGET)/sm_comm_naive_msg_prot
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o \
	$(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o \
	$(TARGET)/main.o $(TARGET)/state_machine_simulation_core.o  $(TARGET)/state_machines.o $(TARGET)/state_machine_simulation_core_action_handling.o \
	$(TARGET)/state_machine_simulation_core_event_handling.o $(TARGET)/state_machine_simulation_core_guard_handling.o $(TARGET)/sm_sim_core_simulation_loop.o \
	$(TARGET)/cmdline_utils.o $(TARGET)/sm_sim_core_asserts.o $(TARGET)/sm_comm_naive_msg_prot.o $(TARGET)/serialization.o $(TARGET)/sm_raw_frame.o $(TARGET)/sm_xml_frame.o $(TARGET)/cal_sender.o $(TARGET)/cal_receiver.o $(TARGET)/state_machine_simulation_core_buildsms.o \
	$(TARGET)/pugixml.o $(TARGET)/log4kmw_events.o $(TARGET)/log4kmw_loggers.o $(TARGET)/log4kmw_records.o $(TARGET)/log4kmw_states.o \
	$(TARGET)/log4kmw_serialization.o $(TARGET)/log4kmw_dynamic_bitset.o $(TARGET)/log4kmw_record.o $(TARGET)/log4kmw_utils.o -o $(TARGET)/sm

$(TARGET)/sm_trace: $(TARGET)/pugixml.o $(TARGET)/trace.o $(TARGET)/log4kmw_events.o $(TARGET)/log4kmw_loggers.o $(TARGET)/log4kmw_records.o $(TARGET)/log4kmw_states.o $(TARGET)/log4kmw_serialization.o \
$(TARGET)/log4kmw_dynamic_bitset.o $(TARGET)/log4kmw_record.o $(TARGET)/log4kmw_utils.o $(TARGET)/log4kmw_loggers_tests.o
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o \
	$(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o \
	$(TARGET)/trace.o $(TARGET)/log4kmw_events.o $(TARGET)/log4kmw_loggers.o $(TARGET)/log4kmw_records.o $(TARGET)/log4kmw_states.o \
	$(TARGET)/log4kmw_serialization.o $(TARGET)/log4kmw_dynamic_bitset.o $(TARGET)/log4kmw_record.o $(TARGET)/log4kmw_utils.o $(TARGET)/log4kmw_loggers_tests.o $(TARGET)/pugixml.o -o $(TARGET)/sm_trace



$(TARGET)/main.o: src/main.cpp
	$(CXX)   $(cflags) $(includes) src/main.cpp -c -o $(TARGET)/main.o
$(TARGET)/trace.o: core/src/trace.cpp
	$(CXX)   $(cflags) $(includes) core/src/trace.cpp -c -o $(TARGET)/trace.o
$(TARGET)/state_machine_simulation_core.o: core/src/state_machine_simulation_core.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core.cpp -c -o $(TARGET)/state_machine_simulation_core.o
$(TARGET)/state_machine_simulation_core_action_handling.o: core/src/state_machine_simulation_core_action_handling.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_action_handling.cpp -c -o $(TARGET)/state_machine_simulation_core_action_handling.o
$(TARGET)/state_machine_simulation_core_event_handling.o: core/src/state_machine_simulation_core_event_handling.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_event_handling.cpp -c -o $(TARGET)/state_machine_simulation_core_event_handling.o
$(TARGET)/state_machine_simulation_core_guard_handling.o: core/src/state_machine_simulation_core_guard_handling.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_guard_handling.cpp -c -o $(TARGET)/state_machine_simulation_core_guard_handling.o
$(TARGET)/sm_sim_core_simulation_loop.o: core/src/sm_sim_core_simulation_loop.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_core_simulation_loop.cpp -c -o $(TARGET)/sm_sim_core_simulation_loop.o
$(TARGET)/sm_sim_core_asserts.o: core/src/sm_sim_core_asserts.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_core_asserts.cpp -c -o $(TARGET)/sm_sim_core_asserts.o
$(TARGET)/sm_comm_naive_msg_prot.o: core/src/sm_comm_naive_msg_prot.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_comm_naive_msg_prot.cpp -c -o $(TARGET)/sm_comm_naive_msg_prot.o
$(TARGET)/serialization.o: core/src/serialization.cpp
	$(CXX)   $(cflags) $(includes) core/src/serialization.cpp -c -o $(TARGET)/serialization.o  
$(TARGET)/state_machines.o: core/src/state_machines.cpp 
	$(CXX)   $(cflags) $(includes) core/src/state_machines.cpp -c -o $(TARGET)/state_machines.o
$(TARGET)/sm_raw_frame.o: core/src/sm_raw_frame.cpp 
	$(CXX)   $(cflags) $(includes) core/src/sm_raw_frame.cpp -c -o $(TARGET)/sm_raw_frame.o
$(TARGET)/sm_xml_frame.o: core/src/sm_xml_frame.cpp 
	$(CXX)   $(cflags) $(includes) core/src/sm_xml_frame.cpp -c -o $(TARGET)/sm_xml_frame.o
$(TARGET)/pugixml.o: $(pugisrc)/pugixml.cpp 
	$(CXX)   $(cflags) $(includes) $(pugisrc)/pugixml.cpp -c -o $(TARGET)/pugixml.o
$(TARGET)/cal_sender.o: core/src/cal_sender.cpp 
	$(CXX)   $(cflags) $(includes) core/src/cal_sender.cpp -c -o $(TARGET)/cal_sender.o
$(TARGET)/cal_receiver.o: core/src/cal_receiver.cpp 
	$(CXX)   $(cflags) $(includes) core/src/cal_receiver.cpp -c -o $(TARGET)/cal_receiver.o	
$(TARGET)/cmdline_utils.o:
	$(CXX)   $(cflags) $(includes) core/src/cmdline_utils.cpp -c -o $(TARGET)/cmdline_utils.o
$(TARGET)/state_machine_simulation_core_buildsms.o: core/src/state_machine_simulation_core_buildsms.cpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_buildsms.cpp -c -o $(TARGET)/state_machine_simulation_core_buildsms.o
$(TARGET)/log4kmw_events.o: core/src_gen/logging/log4kmw_events.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4kmw_events.cpp -c -o $(TARGET)/log4kmw_events.o
$(TARGET)/log4kmw_loggers.o: core/src_gen/logging/log4kmw_loggers.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4kmw_loggers.cpp -c -o $(TARGET)/log4kmw_loggers.o	
$(TARGET)/log4kmw_records.o: core/src_gen/logging/log4kmw_records.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4kmw_records.cpp -c -o $(TARGET)/log4kmw_records.o
$(TARGET)/log4kmw_states.o: core/src_gen/logging/log4kmw_states.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4kmw_states.cpp -c -o $(TARGET)/log4kmw_states.o
$(TARGET)/log4kmw_loggers_tests.o: core/src_gen/logging/log4kmw_loggers_tests.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4kmw_loggers_tests.cpp -c -o $(TARGET)/log4kmw_loggers_tests.o	
$(TARGET)/log4kmw_serialization.o: $(log4kmwsrc)/log4kmw_serialization.cpp
	$(CXX)   $(cflags) $(includes) $(log4kmwsrc)/log4kmw_serialization.cpp -c -o $(TARGET)/log4kmw_serialization.o	
$(TARGET)/log4kmw_dynamic_bitset.o: $(log4kmwsrc)/log4kmw_dynamic_bitset.cpp
	$(CXX)   $(cflags) $(includes) $(log4kmwsrc)/log4kmw_dynamic_bitset.cpp -c -o $(TARGET)/log4kmw_dynamic_bitset.o	
$(TARGET)/log4kmw_record.o: $(log4kmwsrc)/log4kmw_record.cpp
	$(CXX)   $(cflags) $(includes) $(log4kmwsrc)/log4kmw_record.cpp -c -o $(TARGET)/log4kmw_record.o	
$(TARGET)/log4kmw_utils.o: $(log4kmwsrc)/log4kmw_utils.cpp
	$(CXX)   $(cflags) $(includes) $(log4kmwsrc)/log4kmw_utils.cpp -c -o $(TARGET)/log4kmw_utils.o	

	
clean:
	rm $(TARGET)/*

