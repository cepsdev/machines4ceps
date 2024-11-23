#
# machines4ceps - Makefile
#

includes :=  -I"include" -I"../ceps/core/include" -I"../ceps/core/include/include-gen" -I"." -I"../pugixml/src" -I"../log4ceps/include" -I"core/src_gen/logging" -I"./utils" 
#cflags := -std=c++17 -O3 -s -Wall -MD -fmessage-length=0 -Wl,--no-as-needed
#cflags := -g3 -O2 -pg -Wall -MD -fmessage-length=0 -std=c++1y -Wl,--no-as-needed -ldl -lpthread -lrt -fPIC -Wall
cflags := -O3  -Wall -MD -fmessage-length=0 -std=c++17 -Wl,--no-as-needed -fPIC
TARGET :=
OBJDIR := $(TARGET)
objfiles := serialization.o main.o state_machines.o sm_sim_core_asserts.o state_machine_simulation_core.o sm_sim_core_simulation_loop.o state_machine_simulation_core_action_handling.o state_machine_simulation_core_event_handling.o \
  state_machine_simulation_core_guard_handling.o cmdline_utils.o sm_raw_frame.o sm_xml_frame.o pugixml.o  cal_sender.o cal_receiver.o state_machine_simulation_core_plugin_interface.o state_machine_simulation_core_buildsms.o \
  log4ceps_events.o log4ceps_loggers.o log4ceps_records.o log4ceps_states.o log4ceps_serialization.o log4ceps_dynamic_bitset.o log4ceps_record.o log4ceps_utils.o sm_comm_naive_msg_prot.o cppgen.o dotgen.o livelogger.o rdwrn.o \
  sm_livelog_storage_utils.o signalgenerator.o gensm.o partitions.o cover_path.o sm_global_functions.o fibex_import.o can_layer_docgen.o asciidoc.o sm_sim_core_shadow_states.o sm_sim_process_sm.o concept_dependency_graph.o stddoc.o \
   generic_tcp_communication.o ceps_websocket.o sm_sim_core_timer.o sm_sim_execute_action_seq.o streamtransform.o\
  ws_api.o virtual_can_api.o docgen_formats.o docgen_sm.o docgen_macros.o docgen_docwriter_ansi_console.o docgen_docwriter_html5.o\
   docgen_docwriter_markdown_jira_style.o docgen_docwriter_factory.o docgen_theme_factory.o  docgen.o docgen_ifelse.o\
   docgen_docwriter_markdown_github_style.o  docgen_docwriter_markdown_minimal.o oblectamenta-assembler.o vm_base.o
objfiles := $(patsubst %,$(OBJDIR)/%,$(objfiles))
CEPSLIB := ../ceps/core/$(TARGET)/libcepscore.a
tutorial_dir := tutorial
cepslibs := ../ceps/core/bin
pugisrc = ../pugixml/src
log4cepssrc = ../log4ceps/src
SM4CEPSLIB := libsm4ceps.a
cepsinc := ../ceps/core/include

all: $(TARGET)/ceps

$(TARGET)/$(SM4CEPSLIB): $(objfiles)
	echo $(objfiles)
	rm -f $(TARGET)/$(SM4CEPSLIB);\
	$(AR) rcs $(TARGET)/$(SM4CEPSLIB) $(objfiles)

$(TARGET)/ceps: $(objfiles) $(CEPSLIB)
	$(CXX)   $(cflags) $(includes) -L"../cryptopp" -L"../ceps/core/$(TARGET)" \
	$(TARGET)/main.o \
	$(TARGET)/state_machine_simulation_core.o \
	$(TARGET)/state_machines.o \
	$(TARGET)/state_machine_simulation_core_action_handling.o \
	$(TARGET)/state_machine_simulation_core_plugin_interface.o \
	$(TARGET)/state_machine_simulation_core_event_handling.o \
	$(TARGET)/state_machine_simulation_core_guard_handling.o \
	$(TARGET)/sm_sim_core_simulation_loop.o  \
	$(TARGET)/cppgen.o $(TARGET)/dotgen.o \
	$(TARGET)/cmdline_utils.o \
	$(TARGET)/sm_sim_core_asserts.o \
	$(TARGET)/sm_comm_naive_msg_prot.o \
	$(TARGET)/serialization.o \
	$(TARGET)/sm_raw_frame.o \
	$(TARGET)/sm_xml_frame.o \
	$(TARGET)/cal_sender.o \
	$(TARGET)/cal_receiver.o \
	$(TARGET)/state_machine_simulation_core_buildsms.o \
	$(TARGET)/pugixml.o \
	$(TARGET)/log4ceps_events.o \
	$(TARGET)/log4ceps_loggers.o \
	$(TARGET)/log4ceps_records.o \
	$(TARGET)/log4ceps_states.o \
	$(TARGET)/log4ceps_serialization.o \
	$(TARGET)/log4ceps_dynamic_bitset.o \
	$(TARGET)/log4ceps_record.o \
	$(TARGET)/log4ceps_utils.o \
	$(TARGET)/livelogger.o \
	$(TARGET)/sm_livelog_storage_utils.o \
	$(TARGET)/rdwrn.o \
	$(TARGET)/signalgenerator.o \
	$(TARGET)/gensm.o \
	$(TARGET)/partitions.o \
	$(TARGET)/cover_path.o \
	$(TARGET)/sm_global_functions.o \
	$(TARGET)/fibex_import.o \
	$(TARGET)/can_layer_docgen.o \
	$(TARGET)/asciidoc.o \
	$(TARGET)/sm_sim_core_shadow_states.o \
	$(TARGET)/sm_sim_process_sm.o \
	$(TARGET)/concept_dependency_graph.o \
	$(TARGET)/streamtransform.o \
	$(TARGET)/ws_api.o \
	$(TARGET)/virtual_can_api.o \
	$(TARGET)/stddoc.o 	\
	$(TARGET)/docgen.o \
	$(TARGET)/docgen_sm.o \
	$(TARGET)/docgen_formats.o \
	$(TARGET)/docgen_macros.o \
	$(TARGET)/docgen_docwriter_ansi_console.o \
	$(TARGET)/docgen_docwriter_markdown_jira_style.o \
	$(TARGET)/docgen_docwriter_markdown_github_style.o \
	$(TARGET)/docgen_theme_factory.o \
	$(TARGET)/ceps_websocket.o \
	$(TARGET)/generic_tcp_communication.o \
	$(TARGET)/sm_sim_core_timer.o \
	$(TARGET)/sm_sim_execute_action_seq.o \
	$(TARGET)/oblectamenta-assembler.o \
	$(TARGET)/vm_base.o \
	$(TARGET)/docgen_docwriter_factory.o $(TARGET)/docgen_ifelse.o $(TARGET)/docgen_docwriter_html5.o $(TARGET)/docgen_docwriter_markdown_minimal.o -o $(TARGET)/ceps -ldl -lpthread -lrt -lcepscore

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
$(TARGET)/log4ceps_events.o: core/src_gen/logging/log4ceps_events.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4ceps_events.cpp -c -o $(TARGET)/log4ceps_events.o
$(TARGET)/log4ceps_loggers.o: core/src_gen/logging/log4ceps_loggers.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4ceps_loggers.cpp -c -o $(TARGET)/log4ceps_loggers.o	
$(TARGET)/log4ceps_records.o: core/src_gen/logging/log4ceps_records.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4ceps_records.cpp -c -o $(TARGET)/log4ceps_records.o
$(TARGET)/log4ceps_states.o: core/src_gen/logging/log4ceps_states.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4ceps_states.cpp -c -o $(TARGET)/log4ceps_states.o
$(TARGET)/log4ceps_loggers_tests.o: core/src_gen/logging/log4ceps_loggers_tests.cpp
	$(CXX)   $(cflags) $(includes) core/src_gen/logging/log4ceps_loggers_tests.cpp -c -o $(TARGET)/log4ceps_loggers_tests.o	
$(TARGET)/log4ceps_serialization.o: $(log4cepssrc)/log4ceps_serialization.cpp
	$(CXX)   $(cflags) $(includes) $(log4cepssrc)/log4ceps_serialization.cpp -c -o $(TARGET)/log4ceps_serialization.o	
$(TARGET)/log4ceps_dynamic_bitset.o: $(log4cepssrc)/log4ceps_dynamic_bitset.cpp
	$(CXX)   $(cflags) $(includes) $(log4cepssrc)/log4ceps_dynamic_bitset.cpp -c -o $(TARGET)/log4ceps_dynamic_bitset.o	
$(TARGET)/log4ceps_record.o: $(log4cepssrc)/log4ceps_record.cpp
	$(CXX)   $(cflags) $(includes) $(log4cepssrc)/log4ceps_record.cpp -c -o $(TARGET)/log4ceps_record.o	
$(TARGET)/log4ceps_utils.o: $(log4cepssrc)/log4ceps_utils.cpp
	$(CXX)   $(cflags) $(includes) $(log4cepssrc)/log4ceps_utils.cpp -c -o $(TARGET)/log4ceps_utils.o	
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
$(TARGET)/concept_dependency_graph.o: utils/concept_dependency_graph.cpp utils/concept_dependency_graph.hpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) utils/concept_dependency_graph.cpp -c -o $(TARGET)/concept_dependency_graph.o
$(TARGET)/stddoc.o: utils/stddoc.cpp utils/stddoc.hpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) utils/stddoc.cpp -c -o $(TARGET)/stddoc.o	
$(TARGET)/ws_api.o: core/src/api/websocket/ws_api.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/api/websocket/ws_api.cpp -c -o $(TARGET)/ws_api.o	
$(TARGET)/virtual_can_api.o: core/src/api/virtual_can/virtual_can_api.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/api/virtual_can/virtual_can_api.cpp -c -o $(TARGET)/virtual_can_api.o	
$(TARGET)/streamtransform.o: core/src/transform/streamtransform.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/transform/streamtransform.cpp -c -o $(TARGET)/streamtransform.o	
$(TARGET)/docgen.o: core/src/docgen/docgenerator.cpp core/include/docgen/docgenerator.hpp core/include/docgen/docgenerator_docwriter_factory.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator.cpp -c -o $(TARGET)/docgen.o	
$(TARGET)/docgen_sm.o: core/src/docgen/docgenerator_statemachine.cpp core/include/docgen/docgenerator.hpp core/include/state_machine_simulation_core.hpp ${cepsinc}/*
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_statemachine.cpp -c -o $(TARGET)/docgen_sm.o			
$(TARGET)/docgen_formats.o: core/src/docgen/docgenerator_formats.cpp core/include/docgen/docgenerator.hpp core/include/state_machine_simulation_core.hpp ${cepsinc}/*
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_formats.cpp -c -o $(TARGET)/docgen_formats.o
$(TARGET)/docgen_macros.o: core/src/docgen/docgenerator_macros.cpp core/include/docgen/docgenerator.hpp core/include/state_machine_simulation_core.hpp ${cepsinc}/*
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_macros.cpp -c -o $(TARGET)/docgen_macros.o
$(TARGET)/docgen_docwriter_factory.o: core/src/docgen/docgenerator_docwriter_factory.cpp core/include/docgen/docgenerator.hpp core/include/docgen/docgenerator_docwriter_factory.hpp core/include/docgen/docgenerator_docwriter_ansi_console.hpp core/include/docgen/docgenerator_docwriter_markdown_jira_style.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_docwriter_factory.cpp -c -o $(TARGET)/docgen_docwriter_factory.o
$(TARGET)/docgen_docwriter_ansi_console.o: core/src/docgen/docgenerator_docwriter_ansi_console.cpp core/include/docgen/docgenerator.hpp core/include/docgen/docgenerator_docwriter_ansi_console.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_docwriter_ansi_console.cpp -c -o $(TARGET)/docgen_docwriter_ansi_console.o
$(TARGET)/docgen_docwriter_markdown_jira_style.o: core/src/docgen/docgenerator_docwriter_markdown_jira_style.cpp core/include/docgen/docgenerator_docwriter_markdown_jira_style.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_docwriter_markdown_jira_style.cpp -c -o $(TARGET)/docgen_docwriter_markdown_jira_style.o
$(TARGET)/docgen_docwriter_markdown_github_style.o: core/src/docgen/docgenerator_docwriter_markdown_github_style.cpp core/include/docgen/docgenerator_docwriter_markdown_github_style.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_docwriter_markdown_github_style.cpp -c -o $(TARGET)/docgen_docwriter_markdown_github_style.o
$(TARGET)/docgen_theme_factory.o: core/src/docgen/docgenerator_theme_factory.cpp core/include/docgen/docgenerator_theme_factory.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_theme_factory.cpp -c -o $(TARGET)/docgen_theme_factory.o
$(TARGET)/docgen_ifelse.o: core/src/docgen/docgenerator_ifelse.cpp core/include/docgen/docgenerator.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_ifelse.cpp -c -o $(TARGET)/docgen_ifelse.o
$(TARGET)/docgen_docwriter_html5.o: core/src/docgen/docgenerator_docwriter_html5.cpp core/include/docgen/docgenerator.hpp core/include/docgen/docgenerator_docwriter_html5.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_docwriter_html5.cpp -c -o $(TARGET)/docgen_docwriter_html5.o
$(TARGET)/ceps_websocket.o: core/src/websocket.cpp core/include/websocket.hpp
	$(CXX)   $(cflags) $(includes) core/src/websocket.cpp -c -o $(TARGET)/ceps_websocket.o
$(TARGET)/generic_tcp_communication.o: core/src/sockets/generic_tcp_communication.cpp
	$(CXX)   $(cflags) $(includes) core/src/sockets/generic_tcp_communication.cpp -c -o $(TARGET)/generic_tcp_communication.o
$(TARGET)/sm_sim_core_timer.o: core/src/sm_sim_core_timer.cpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_core_timer.cpp -c -o $(TARGET)/sm_sim_core_timer.o
$(TARGET)/sm_sim_execute_action_seq.o:  core/src/sm_sim_execute_action_seq.cpp core/include/state_machine_simulation_core.hpp
	$(CXX)   $(cflags) $(includes) core/src/sm_sim_execute_action_seq.cpp -c -o $(TARGET)/sm_sim_execute_action_seq.o
$(TARGET)/docgen_docwriter_markdown_minimal.o: core/src/docgen/docgenerator_docwriter_markdown_minimal.cpp core/include/docgen/docgenerator_docwriter_markdown_minimal.hpp
	$(CXX)   $(cflags) $(includes) core/src/docgen/docgenerator_docwriter_markdown_minimal.cpp -c -o $(TARGET)/docgen_docwriter_markdown_minimal.o
$(TARGET)/oblectamenta-assembler.o: core/src/vm/oblectamenta-assembler.cpp core/include/vm/vm_base.hpp core/include/vm/oblectamenta-assembler.hpp
	$(CXX)   $(cflags) $(includes) core/src/vm/oblectamenta-assembler.cpp -c -o $(TARGET)/oblectamenta-assembler.o
$(TARGET)/vm_base.o: core/src/vm/vm_base.cpp core/include/vm/vm_base.hpp
	$(CXX)   $(cflags) $(includes) core/src/vm/vm_base.cpp -c -o $(TARGET)/vm_base.o

clean:
	rm $(TARGET)/*



