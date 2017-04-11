#-------------------------------------------------
#
# Project created by QtCreator 2017-04-11T09:17:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hello_world
TEMPLATE = app
QMAKE_CXXFLAGS += -std=c++14

INCLUDEPATH += "../../../ceps/core/include" \ 
               "../../core/src_gen/logging" \
               "../../../ceps/core/include/include-gen" \
               "../../" \
               "../../../log4kmw/include"

LIBS += -L../../../ceps/core/bin -L../../x86  -lsm4ceps -lcepscore -ldl -lpthread
               
SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
