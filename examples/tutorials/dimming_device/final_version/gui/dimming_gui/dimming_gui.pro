#-------------------------------------------------
#
# Project created by QtCreator 2015-09-17T03:44:09
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

UTILS = $$_PRO_FILE_PWD_/../../../../../../..

TARGET = dimming_gui
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wall

INCLUDEPATH += $$UTILS/ceps/core/include \
               $$UTILS \
               $$UTILS/statemachines \
               $$UTILS/statemachines/pugixml-1.6/src

LIBS += -L../../../../../../../ceps/core/bin -lcepscore
LIBS += -ldl -fPIC

SOURCES += main.cpp\
        mainwindow.cpp \
    ../../../../../../core/src/cmdline_utils.cpp \
    ../../../../../../core/src/serialization.cpp \
    ../../../../../../core/src/sm_comm_naive_msg_prot.cpp \
    ../../../../../../core/src/sm_raw_frame.cpp \
    ../../../../../../core/src/sm_sim_core_asserts.cpp \
    ../../../../../../core/src/sm_sim_core_simulation_loop.cpp \
    ../../../../../../core/src/state_machine_simulation_core_action_handling.cpp \
    ../../../../../../core/src/state_machine_simulation_core_event_handling.cpp \
    ../../../../../../core/src/state_machine_simulation_core_guard_handling.cpp \
    ../../../../../../core/src/state_machine_simulation_core.cpp \
    ../../../../../../core/src/state_machines.cpp

HEADERS  += mainwindow.h \
    ../../../../../../core/src/serialization.cpp~ \
    ../../../../../../core/src/sm_comm_naive_msg_prot.cpp~ \
    ../../../../../../core/src/sm_raw_frame.cpp~

FORMS    += mainwindow.ui
