#-------------------------------------------------
#
# Project created by QtCreator 2016-10-19T09:47:00
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = livelog
TEMPLATE = app


QMAKE_CXXFLAGS += -std=c++14
QMAKE_CXXFLAGS += -Wall

INCLUDEPATH += "../../../../ceps/core/include" \ 
               "../../../../ceps/core/include/include-gen" \
               "."  \
               "../../../pugixml-1.6/src" \ 
               "../../../"
               
SOURCES += main.cpp\
        mainwindow.cpp \
        ../../../core/src/livelog/livelogger.cpp \
        ../../../core/src/sockets/rdwrn.cpp \
        ../../../core/src/sm_livelog_storage_utils.cpp \
    sm4ceps_livelog_treemodel.cpp
        
HEADERS  += mainwindow.h \
    sm4ceps_livelog_treemodel.h

FORMS    += mainwindow.ui
