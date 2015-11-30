#-------------------------------------------------
#
# Project created by QtCreator 2015-11-24T15:53:23
#
#-------------------------------------------------

CONFIG   += c++11
CEPS_DIR = $$_PRO_FILE_PWD_/../../../ceps/core
SM4CEPS_DIR = $$_PRO_FILE_PWD_/../..
DEFINES += USE_KMW_MULTIBUS 

INCLUDEPATH += $$CEPS_DIR/include
INCLUDEPATH += $$CEPS_DIR/include/include_gen
INCLUDEPATH += $$SM4CEPS_DIR
INCLUDEPATH += $$SM4CEPS_DIR/kmw
INCLUDEPATH += $$SM4CEPS_DIR/include
INCLUDEPATH += $$SM4CEPS_DIR/pugixml-1.6/src

QT       -= core gui

TARGET = sm4cepscore
TEMPLATE = lib
CONFIG += staticlib

DEFINES += SM4CEPSCORE_LIBRARY
SOURCES   +=    $$SM4CEPS_DIR/core/src/cal_receiver.cpp \
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
    $$SM4CEPS_DIR/pugixml-1.6/src/pugixml.cpp


unix {
    target.path = /usr/lib
    INSTALLS += target
}

LIBS+= -lws2_32
