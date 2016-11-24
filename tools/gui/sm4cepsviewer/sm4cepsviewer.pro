#-------------------------------------------------
#
# Project created by QtCreator 2016-11-09T14:26:25
#
#-------------------------------------------------

RESOURCES =  sm4cepsviewer.qrc

QT       += core gui
QT += svg
QT += opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sm4cepsviewer
TEMPLATE = app

 
QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -Wall

INCLUDEPATH += "../../../../ceps/core/include" \ 
               "../../../core/src_gen/logging" \
               "../../../../ceps/core/include/include-gen" \
               "."  \
               "../../../pugixml-1.6/src" \ 
               "../../../" \
               "../../../../log4kmw/include" \
                "../../../../graphviz/bin/include" \
                "../livelog"

LIBS += -L../../../../ceps/core/bin -lcepscore
LIBS += -ldl -lpthread
#LIBS += -L../../../../graphviz/bin/lib -lcgraph
#LIBS += -L../../../../graphviz/bin/lib -lgvc
#LIBS += -L../../../../graphviz/bin/lib -lcdt



SOURCES += main.cpp\
        mainwindow.cpp \   
    svgview.cpp \
    sm_model.cpp \
    smachines_window.cpp \
    mdibase.cpp \
    ../livelog/sm4ceps_livelog_treemodel.cpp

SOURCES += ../../../core/src/cal_receiver.cpp \
           ../../../core/src/cal_sender.cpp \
           ../../../core/src/cmdline_utils.cpp \
           ../../../core/src/serialization.cpp \
           ../../../core/src/sm_comm_naive_msg_prot.cpp \
           ../../../core/src/sm_livelog_storage_utils.cpp \
           ../../../core/src/sm_raw_frame.cpp \
           ../../../core/src/sm_sim_core_asserts.cpp \
           ../../../core/src/sm_sim_core_simulation_loop.cpp \
           ../../../core/src/sm_xml_frame.cpp \
           ../../../core/src/state_machines.cpp \
           ../../../core/src/state_machine_simulation_core_action_handling.cpp \
           ../../../core/src/state_machine_simulation_core_buildsms.cpp \
           ../../../core/src/state_machine_simulation_core.cpp \
           ../../../core/src/state_machine_simulation_core_event_handling.cpp \
           ../../../core/src/state_machine_simulation_core_guard_handling.cpp \
           ../../../core/src/state_machine_simulation_core_plugin_interface.cpp \
           ../../../core/src/livelog/livelogger.cpp \
           ../../../../log4kmw/src/log4kmw_dynamic_bitset.cpp \
           ../../../../log4kmw/src/log4kmw_record.cpp \
           ../../../../log4kmw/src/log4kmw_serialization.cpp \
           ../../../../log4kmw/src/log4kmw_utils.cpp \
           ../../../core/src_gen/logging/log4kmw_states.cpp \
           ../../../core/src_gen/logging/log4kmw_events.cpp \
           ../../../core/src_gen/logging/log4kmw_records.cpp \
           ../../../core/src_gen/logging/log4kmw_loggers.cpp \
           ../../../core/src/cppgenerator/cppgen.cpp \
           ../../../core/src/sockets/rdwrn.cpp \
           ../../../core/src/dotgenerator/dotgen.cpp

            
HEADERS  += mainwindow.h \ 
    svgview.h \
    common.h \
    sm_model.h \
    smachines_window.h \
    mdibase.h \
    ../livelog/sm4ceps_livelog_treemodel.h


HEADERS += ../../../core/include/base_defs.hpp \
           ../../../core/include/cal_receiver.hpp \
           ../../../core/include/cal_sender.hpp \
           ../../../core/include/cmdline_utils.hpp \
           ../../../core/include/events.hpp \
           ../../../core/include/serialization.hpp \
           ../../../core/include/sm_comm_naive_msg_prot.hpp \
           ../../../core/include/sm_ev_comm_layer.hpp \
           ../../../core/include/sm_execution_ctxt.hpp \
           ../../../core/include/sm_livelog_storage_ids.hpp \
           ../../../core/include/sm_livelog_storage_utils \
           ../../../core/include/sm_raw_frame.hpp \
           ../../../core/include/sm_xml_frame.hpp \
           ../../../core/include/state_machine.hpp \
           ../../../core/include/state_machine_simulation_core.hpp \
           ../../../core/include/state_machine_simulation_core_plugin_interface \
           ../../../core/include/state_machine_simulation_core_reg_fun.hpp \
           ../../../core/include/sockets/rdwrn.hpp \
           ../../../core/include/livelog/livelogger.hpp \
           ../../../core/include/dotgenerator/dotgen.hpp \
           ../../../core/include/cppgenerator/cppgen.hpp \
           ../../../../ceps/core/include/ceps_all.hh
           