#-------------------------------------------------
#
# Project created by QtCreator 2016-10-25T14:21:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sviewer
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
                "../../../../graphviz/bin/include"

LIBS += -L../../../../ceps/core/bin -lcepscore
LIBS += -ldl -lpthread
LIBS += -L../../../../graphviz/bin/lib -lcgraph
LIBS += -L../../../../graphviz/bin/lib -lgvc
LIBS += -L../../../../graphviz/bin/lib -lcdt



SOURCES += main.cpp\
        mainwindow.cpp \
    render_statemachines.cpp

SOURCES += ../../../core/src/cal_receiver.cpp ../../../core/src/cal_sender.cpp ../../../core/src/cmdline_utils.cpp \
           ../../../core/src/serialization.cpp ../../../core/src/sm_comm_naive_msg_prot.cpp ../../../core/src/sm_livelog_storage_utils.cpp \
           ../../../core/src/sm_raw_frame.cpp ../../../core/src/sm_sim_core_asserts.cpp ../../../core/src/sm_sim_core_simulation_loop.cpp \
           ../../../core/src/sm_xml_frame.cpp ../../../core/src/state_machines.cpp ../../../core/src/state_machine_simulation_core_action_handling.cpp \
           ../../../core/src/state_machine_simulation_core_buildsms.cpp ../../../core/src/state_machine_simulation_core.cpp ../../../core/src/state_machine_simulation_core_event_handling.cpp \
           ../../../core/src/state_machine_simulation_core_guard_handling.cpp ../../../core/src/state_machine_simulation_core_plugin_interface.cpp ../../../core/src/livelog/livelogger.cpp \
           ../../../../log4kmw/src/log4kmw_dynamic_bitset.cpp ../../../../log4kmw/src/log4kmw_record.cpp ../../../../log4kmw/src/log4kmw_serialization.cpp ../../../../log4kmw/src/log4kmw_utils.cpp \
           ../../../core/src_gen/logging/log4kmw_states.cpp ../../../core/src_gen/logging/log4kmw_events.cpp ../../../core/src_gen/logging/log4kmw_records.cpp ../../../core/src_gen/logging/log4kmw_loggers.cpp \
           ../../../core/src/cppgenerator/cppgen.cpp ../../../core/src/sockets/rdwrn.cpp

            
HEADERS  += mainwindow.h \
    render_statemachines.h

FORMS    += mainwindow.ui
