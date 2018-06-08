#-------------------------------------------------
#
# Project created by QtCreator 2015-11-24T15:53:23
#
#-------------------------------------------------

QMAKE_CXXFLAGS += -O1 -g3 -Wall -MD -fmessage-length=0 -std=c++17 -Wl,--no-as-needed -fPIC


CEPS_DIR = $$_PRO_FILE_PWD_/../../../ceps/core


INCLUDEPATH += $$CEPS_DIR/include
INCLUDEPATH += $$CEPS_DIR/include/include_gen
INCLUDEPATH += $$CEPS_DIR/../../pugixml/src

QT       -= core gui

TARGET = ceps-jit
TEMPLATE = app

SOURCES   +=     $$CEPS_DIR/src/ceps_ast.cpp \
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
    $$CEPS_DIR/../../pugixml/src/pugixml.cpp \
    ../main.cpp

LIBS+=  -ldl -lpthread -lrt

#QMAKE_POST_LINK += $$quote(cp ceps ../../x86/ceps)


