#-------------------------------------------------
#
# Project created by QtCreator 2015-11-24T15:53:23
#
#-------------------------------------------------

QMAKE_CXXFLAGS += -g3 -Wall -MD -fmessage-length=0 -std=c++17 -Wl,--no-as-needed -fPIC


CEPS_DIR = $$_PRO_FILE_PWD_/../../../ceps/core
SM4CEPS_DIR = $$_PRO_FILE_PWD_/../..
LOG4KMW_DIR = $$_PRO_FILE_PWD_/../../../log4kmw

INCLUDEPATH += $$CEPS_DIR/include
INCLUDEPATH += $$CEPS_DIR/include/include_gen
INCLUDEPATH += $$SM4CEPS_DIR
INCLUDEPATH += $$SM4CEPS_DIR/kmw
INCLUDEPATH += $$SM4CEPS_DIR/include
INCLUDEPATH += $$SM4CEPS_DIR/include/api/websocket
INCLUDEPATH += $$SM4CEPS_DIR/pugixml-1.6/src
INCLUDEPATH += $$SM4CEPS_DIR/../log4kmw/include
INCLUDEPATH += $$SM4CEPS_DIR/core/src_gen/logging

QT       -= core gui

TARGET = ceps
TEMPLATE = app

DEFINES += SM4CEPSCORE_LIBRARY
SOURCES   += $$SM4CEPS_DIR/src/main.cpp \    
    $$SM4CEPS_DIR/core/src/cal_receiver.cpp \
    $$SM4CEPS_DIR/core/src/cal_sender.cpp \
    $$SM4CEPS_DIR/core/src/cmdline_utils.cpp \
    $$SM4CEPS_DIR/core/src/serialization.cpp \
    $$SM4CEPS_DIR/core/src/sm_raw_frame.cpp \
    $$SM4CEPS_DIR/core/src/sm_xml_frame.cpp \
    $$SM4CEPS_DIR/core/src/state_machines.cpp \
    $$SM4CEPS_DIR/core/src/sm_sim_core_asserts.cpp \
    $$SM4CEPS_DIR/core/src/sm_comm_naive_msg_prot.cpp \
    $$SM4CEPS_DIR/core/src/sm_sim_core_simulation_loop.cpp \
    $$SM4CEPS_DIR/core/src/state_machine_simulation_core.cpp \
    $$SM4CEPS_DIR/core/src/state_machine_simulation_core_guard_handling.cpp \
    $$SM4CEPS_DIR/core/src/state_machine_simulation_core_event_handling.cpp \
    $$SM4CEPS_DIR/core/src/state_machine_simulation_core_action_handling.cpp \
    $$SM4CEPS_DIR/core/src/state_machine_simulation_core_plugin_interface.cpp \
    $$SM4CEPS_DIR/core/src/cppgenerator/cppgen.cpp \
    $$SM4CEPS_DIR/core/src/dotgenerator/dotgen.cpp \
    $$SM4CEPS_DIR/core/src/state_machine_simulation_core_buildsms.cpp \    
    $$LOG4KMW_DIR/src/log4kmw_utils.cpp \
    $$LOG4KMW_DIR/src/log4kmw_serialization.cpp \
    $$LOG4KMW_DIR/src/log4kmw_record.cpp \
    $$LOG4KMW_DIR/src/log4kmw_dynamic_bitset.cpp \
    $$SM4CEPS_DIR/core/src_gen/logging/log4kmw_events.cpp \
    $$SM4CEPS_DIR/core/src_gen/logging/log4kmw_loggers.cpp \
    $$SM4CEPS_DIR/core/src_gen/logging/log4kmw_records.cpp \
    $$SM4CEPS_DIR/core/src_gen/logging/log4kmw_states.cpp \
    $$SM4CEPS_DIR/core/src/livelog/livelogger.cpp \
    $$SM4CEPS_DIR/core/src/signalgenerator.cpp \
    $$SM4CEPS_DIR/core/src/sm_global_functions.cpp \
    $$SM4CEPS_DIR/core/src/sm_livelog_storage_utils.cpp \
    $$SM4CEPS_DIR/core/src/sm_sim_core_shadow_states.cpp \
    $$SM4CEPS_DIR/core/src/sm_sim_process_sm.cpp \
    $$SM4CEPS_DIR/core/src/sockets/rdwrn.cpp \
    $$SM4CEPS_DIR/core/src/modelling/cover_path.cpp \
    $$SM4CEPS_DIR/core/src/modelling/gensm.cpp \
    $$SM4CEPS_DIR/core/src/modelling/partitions.cpp \
    $$SM4CEPS_DIR/core/src/api/websocket/ws_api.cpp \
    $$SM4CEPS_DIR/core/src/api/virtual_can/virtual_can_api.cpp \
    $$SM4CEPS_DIR/utils/asciidoc.cpp \
    $$SM4CEPS_DIR/utils/can_layer_docgen.cpp \
    $$SM4CEPS_DIR/utils/concept_dependency_graph.cpp \
    $$SM4CEPS_DIR/utils/fibex_import.cpp \
    $$SM4CEPS_DIR/utils/stddoc.cpp \
    $$CEPS_DIR/src/ceps_ast.cpp \
    $$CEPS_DIR/src-gen/ceps.tab.cpp \
    $$CEPS_DIR/src/ceps_interpreter.cpp \
    $$CEPS_DIR/src/cepsparserdriver.cpp \
    $$CEPS_DIR/src/cepsruntime.cpp \
    $$CEPS_DIR/src/cepslexer.cpp \
    $$CEPS_DIR/src/symtab.cpp \
    $$CEPS_DIR/src/ceps_interpreter_loop.cpp \
    $$CEPS_DIR/src/ceps_interpreter_nodeset.cpp \
    $$CEPS_DIR/src/ceps_interpreter_macros.cpp \
    $$CEPS_DIR/src/ceps_interpreter_functions.cpp \
    $$SM4CEPS_DIR/pugixml-1.6/src/pugixml.cpp

LIBS+= -L$SM4CEPS_DIR../cryptopp
LIBS+=  -ldl -lpthread -lrt -lcryptopp

QMAKE_POST_LINK += $$quote(cp ceps ../../x86/ceps)


