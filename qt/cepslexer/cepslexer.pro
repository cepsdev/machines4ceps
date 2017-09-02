#-------------------------------------------------
#
# Project created by QtCreator 2015-11-24T15:53:23
#
#-------------------------------------------------

QMAKE_CXXFLAGS += -g3 -Wall -MD -fmessage-length=0 -std=c++17 -Wl,--no-as-needed -fPIC



SM4CEPS_DIR = $$_PRO_FILE_PWD_/../..
LOG4KMW_DIR = $$_PRO_FILE_PWD_/../../../log4kmw

INCLUDEPATH += SM4CEPS_DIR = $$_PRO_FILE_PWD_/../..


QT       -= core gui

TARGET = cepslexer
TEMPLATE = app


SOURCES   += $$SM4CEPS_DIR/core/src/transform/streamtransform_main.cpp \    
    $$SM4CEPS_DIR/core/src/transform/streamtransform.cpp 
    

QMAKE_POST_LINK += $$quote(cp cepslexer ../../x86/cepslexer)


