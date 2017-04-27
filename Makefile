#
# sm4ceps - Makefile
#

includes :=  -I"include" -I"../ceps/core/include" -I"../ceps/core/include/include-gen" -I"." -I"pugixml-1.6/src" -I"../log4kmw/include" -I"core/src_gen/logging" -I"./utils" 
#cflags := -O2 -s -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
#cflags := -g3 -O2 -pg -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
cflags := -g3 -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
TARGET :=
OBJDIR := $(TARGET)
objfiles := serialization.o main.o state_machines.o sm_sim_core_asserts.o state_machine_simulation_core.o sm_sim_core_simulation_loop.o state_machine_simulation_core_action_handling.o state_machine_simulation_core_event_handling.o \
  state_machine_simulation_core_guard_handling.o cmdline_utils.o sm_raw_frame.o sm_xml_frame.o pugixml.o  cal_sender.o cal_receiver.o state_machine_simulation_core_plugin_interface.o state_machine_simulation_core_buildsms.o \
  log4kmw_events.o log4kmw_loggers.o log4kmw_records.o log4kmw_states.o log4kmw_serialization.o log4kmw_dynamic_bitset.o log4kmw_record.o log4kmw_utils.o sm_comm_naive_msg_prot.o cppgen.o dotgen.o livelogger.o rdwrn.o \
  sm_livelog_storage_utils.o signalgenerator.o gensm.o partitions.o cover_path.o sm_global_functions.o fibex_import.o can_layer_docgen.o asciidoc.o sm_sim_core_shadow_states.o sm_sim_process_sm.o
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../ceps/core/bin$(TARGET)/libcepscore.a
tutorial_dir := tutorial
cepslibs := ../ceps/core/bin
pugisrc = pugixml-1.6/src
log4kmwsrc = ../log4kmw/src
SM4CEPSLIB := libsm4ceps.a

all: $(TARGET)/sm $(TARGET)/sm_trace $(TARGET)/$(SM4CEPSLIB) $(TARGET)/ceps

$(TARGET)/$(SM4CEPSLIB): $(objfiles)
	echo $(objfiles)
	rm -f $(TARGET)/$(SM4CEPSLIB);\
	$(AR) rcs $(TARGET)/$(SM4CEPSLIB) $(objfiles)

$(TARGET)/sm: $(objfiles)
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o \
	$(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o $(cepslibs)/ceps_interpreter_macros.o $(cepslibs)/ceps_interpreter_functions.o \
	$(TARGET)/main.o $(TARGET)/state_machine_simulation_core.o  $(TARGET)/state_machines.o $(TARGET)/state_machine_simulation_core_action_handling.o \
	$(TARGET)/state_machine_simulation_core_plugin_interface.o $(TARGET)/state_machine_simulation_core_event_handling.o \
	$(TARGET)/state_machine_simulation_core_guard_handling.o $(TARGET)/sm_sim_core_simulation_loop.o  $(TARGET)/cppgen.o $(TARGET)/dotgen.o \
	$(TARGET)/cmdline_utils.o $(TARGET)/sm_sim_core_asserts.o $(TARGET)/sm_comm_naive_msg_prot.o $(TARGET)/serialization.o $(TARGET)/sm_raw_frame.o $(TARGET)/sm_xml_frame.o $(TARGET)/cal_sender.o $(TARGET)/cal_receiver.o $(TARGET)/state_machine_simulation_core_buildsms.o \
	$(TARGET)/pugixml.o $(TARGET)/log4kmw_events.o $(TARGET)/log4kmw_loggers.o $(TARGET)/log4kmw_records.o $(TARGET)/log4kmw_states.o \
	$(TARGET)/log4kmw_serialization.o $(TARGET)/log4kmw_dynamic_bitset.o $(TARGET)/log4kmw_record.o $(TARGET)/log4kmw_utils.o $(TARGET)/livelogger.o $(TARGET)/sm_livelog_storage_utils.o $(TARGET)/rdwrn.o $(TARGET)/signalgenerator.o $(TARGET)/gensm.o $(TARGET)/partitions.o $(TARGET)/cover_path.o $(TARGET)/sm_global_functions.o $(TARGET)/fibex_import.o $(TARGET)/can_layer_docgen.o $(TARGET)/asciidoc.o $(TARGET)/sm_sim_core_shadow_states.o $(TARGET)/sm_sim_process_sm.o -o $(TARGET)/sm

$(TARGET)/ceps:$(TARGET)/sm
	cp $(TARGET)/sm $(TARGET)/ceps

$(TARGET)/sm_trace: $(TARGET)/pugixml.o $(TARGET)/trace.o $(TARGET)/log4kmw_events.o $(TARGET)/log4kmw_loggers.o $(TARGET)/log4kmw_records.o $(TARGET)/log4kmw_states.o $(TARGET)/log4kmw_serialization.o \
$(TARGET)/log4kmw_dynamic_bitset.o $(TARGET)/log4kmw_record.o $(TARGET)/log4kmw_utils.o $(TARGET)/log4kmw_loggers_tests.o
	$(CXX)   $(cflags) $(includes) -ldl $(cepslibs)/ceps_ast.o $(cepslibs)/ceps.tab.o $(cepslibs)/ceps_interpreter.o $(cepslibs)/cepsparserdriver.o \
	$(cepslibs)/cepsruntime.o $(cepslibs)/cepslexer.o $(cepslibs)/symtab.o $(cepslibs)/ceps_interpreter_loop.o $(cepslibs)/ceps_interpreter_nodeset.o $(cepslibs)/ceps_interpreter_macros.o $(cepslibs)/ceps_interpreter_functions.o \
	$(TARGET)/trace.o $(TARGET)/log4kmw_events.o $(TARGET)/log4kmw_loggers.o $(TARGET)/log4kmw_records.o $(TARGET)/log4kmw_states.o \
	$(TARGET)/log4kmw_serialization.o $(TARGET)/log4kmw_dynamic_bitset.o $(TARGET)/log4kmw_record.o $(TARGET)/log4kmw_utils.o $(TARGET)/log4kmw_loggers_tests.o $(TARGET)/pugixml.o -o $(TARGET)/sm_trace


$(TARGET)/main.o: src/main.cpp
	$(CXX)   $(cflags) $(includes) src/main.cpp -c -o $(TARGET)/main.o
$(TARGET)/trace.o: core/src/trace.cpp
	$(CXX)   $(cflags) $(includes) core/src/trace.cpp -c -o $(TARGET)/trace.o
$(TARGET)/state_machine_simulation_core.o: core/src/state_machine_simulation_core.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core.cpp -c -o $(TARGET)/state_machine_simulation_core.o
$(TARGET)/state_machine_simulation_core_action_handling.o: core/src/state_machine_simulation_core_action_handling.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_action_handling.cpp -c -o $(TARGET)/state_machine_simulation_core_action_handling.o
$(TARGET)/state_machine_simulation_core_event_handling.o: core/src/state_machine_simulation_core_event_handling.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_event_handling.cpp -c -o $(TARGET)/state_machine_simulation_core_event_handling.o
$(TARGET)/state_machine_simulation_core_guard_handling.o: core/src/state_machine_simulation_core_guard_handling.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_guard_handling.cpp -c -o $(TARGET)/state_machine_simulation_core_guard_handling.o
$(TARGET)/sm_sim_core_simulation_loop.o: core/src/sm_sim_core_simulation_loop.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags)  $(includes) core/src/sm_sim_core_simulation_loop.cpp -c -o $(TARGET)/sm_sim_core_simulation_loop.o
$(TARGET)/cppgen.o: core/src/cppgenerator/cppgen.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/cppgenerator/cppgen.cpp -c -o $(TARGET)/cppgen.o
$(TARGET)/dotgen.o: core/src/dotgenerator/dotgen.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/dotgenerator/dotgen.cpp -c -o $(TARGET)/dotgen.o
$(TARGET)/sm_sim_core_asserts.o: core/src/sm_sim_core_asserts.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_core_asserts.cpp -c -o $(TARGET)/sm_sim_core_asserts.o
$(TARGET)/sm_comm_naive_msg_prot.o: core/src/sm_comm_naive_msg_prot.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_comm_naive_msg_prot.cpp -c -o $(TARGET)/sm_comm_naive_msg_prot.o
$(TARGET)/serialization.o: core/src/serialization.cpp
	$(CXX)   $(cflags) $(includes) core/src/serialization.cpp -c -o $(TARGET)/serialization.o  
$(TARGET)/state_machines.o: core/src/state_machines.cpp 
	$(CXX)   $(cflags) $(includes) core/src/state_machines.cpp -c -o $(TARGET)/state_machines.o
$(TARGET)/sm_raw_frame.o: core/src/sm_raw_frame.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_raw_frame.cpp -c -o $(TARGET)/sm_raw_frame.o
$(TARGET)/sm_xml_frame.o: core/src/sm_xml_frame.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_xml_frame.cpp -c -o $(TARGET)/sm_xml_frame.o
$(TARGET)/state_machine_simulation_core_plugin_interface.o: core/include/state_machine_simulation_core_plugin_interface.hpp
	$(CXX)   $(cflags) $(includes) core/src/state_machine_simulation_core_plugin_interface.cpp -c -o $(TARGET)/state_machine_simulation_core_plugin_interface.o
$(TARGET)/pugixml.o: $(pugisrc)/pugixml.cpp 
	$(CXX)   $(cflags) $(includes) $(pugisrc)/pugixml.cpp -c -o $(TARGET)/pugixml.o
$(TARGET)/cal_sender.o: core/src/cal_sender.cpp 
	$(CXX)   $(cflags) $(includes) core/src/cal_sender.cpp -c -o $(TARGET)/cal_sender.o
$(TARGET)/cal_receiver.o: core/src/cal_receiver.cpp 
	$(CXX)   $(cflags) $(includes) core/src/cal_receiver.cpp -c -o $(TARGET)/cal_receiver.o	
$(TARGET)/cmdline_utils.o: core/include/cmdline_utils.hpp
	$(CXX)   $(cflags) $(includes) core/src/cmdline_utils.cpp -c -o $(TARGET)/cmdline_utils.o
$(TARGET)/state_machine_simulation_core_buildsms.o: core/src/state_machine_simulation_core_buildsms.cpp core/include/state_machine_simulation_core.hpp
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
$(TARGET)/livelogger.o: core/src/livelog/livelogger.cpp core/include/livelog/livelogger.hpp
	$(CXX)   $(cflags) $(includes) core/src/livelog/livelogger.cpp -c -o $(TARGET)/livelogger.o	
$(TARGET)/sm_livelog_storage_utils.o: core/src/sm_livelog_storage_utils.cpp core/include/livelog/livelogger.hpp core/include/sm_livelog_storage_utils.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_livelog_storage_utils.cpp -c -o $(TARGET)/sm_livelog_storage_utils.o	
$(TARGET)/rdwrn.o: core/src/sockets/rdwrn.cpp
	$(CXX)   $(cflags) $(includes) core/src/sockets/rdwrn.cpp -c -o $(TARGET)/rdwrn.o	
$(TARGET)/signalgenerator.o: core/src/signalgenerator.cpp
	$(CXX)   $(cflags) $(includes) core/src/signalgenerator.cpp -c -o $(TARGET)/signalgenerator.o
$(TARGET)/gensm.o: core/src/modelling/gensm.cpp
	$(CXX)   $(cflags) $(includes) core/src/modelling/gensm.cpp -c -o $(TARGET)/gensm.o
$(TARGET)/partitions.o: core/src/modelling/partitions.cpp
	$(CXX)   $(cflags) $(includes) core/src/modelling/partitions.cpp -c -o $(TARGET)/partitions.o
$(TARGET)/cover_path.o: core/src/modelling/cover_path.cpp  
	$(CXX)   $(cflags) $(includes) core/src/modelling/cover_path.cpp -c -o $(TARGET)/cover_path.o
$(TARGET)/sm_global_functions.o: core/src/sm_global_functions.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_global_functions.cpp -c -o $(TARGET)/sm_global_functions.o
$(TARGET)/sm_sim_core_shadow_states.o: core/src/sm_sim_core_shadow_states.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_core_shadow_states.cpp -c -o $(TARGET)/sm_sim_core_shadow_states.o
$(TARGET)/fibex_import.o: utils/fibex_import.cpp utils/fibex_import.hpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) utils/fibex_import.cpp -c -o $(TARGET)/fibex_import.o
$(TARGET)/asciidoc.o: utils/asciidoc.cpp utils/asciidoc.hpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) utils/asciidoc.cpp -c -o $(TARGET)/asciidoc.o
$(TARGET)/can_layer_docgen.o: utils/can_layer_docgen.cpp utils/can_layer_docgen.hpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) utils/can_layer_docgen.cpp -c -o $(TARGET)/can_layer_docgen.o
$(TARGET)/sm_sim_process_sm.o: core/src/sm_sim_process_sm.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_process_sm.cpp -c -o $(TARGET)/sm_sim_process_sm.o
clean:
	rm $(TARGET)/*



