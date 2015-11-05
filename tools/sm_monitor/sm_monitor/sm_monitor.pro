#-------------------------------------------------
#
# Project created by QtCreator 2015-09-06T11:51:21
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sm_monitor
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wall

INCLUDEPATH += ../../../../ceps/core/include\
               ../../../..\
               ../../../../sm4ceps
SOURCES += ../../../../sm4ceps/core/src/serialization.cpp \
    simcoreconnectdlg.cpp

LIBS += -L../../../../ceps/core/bin -lcepscore

SOURCES += main.cpp\
        mainwindow.cpp \
    comm.cpp

HEADERS  += mainwindow.h \
      simcoreconnectdlg.h

FORMS    += mainwindow.ui \
    simcoreconnectdlg.ui

RESOURCES += \
    sm_monitor.qrc
