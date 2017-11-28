#-------------------------------------------------
#
# Project created by QtCreator 2017-11-24T10:14:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = minimal
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h \
    ../../../include/create_ceps_cloud_streaming_endpoint.h

FORMS += \
        mainwindow.ui


### START cepSCloud Streaming specifics
# Following lines enable cepSCloud streaming.
LIBS += -lws2_32
DEFINES += SIMBOX_HOST=\\\"tomas-cepsdev-win\\\"
DEFINES += SIMBOX_HOST_PORT=\\\"8181\\\"
QMAKE_CXXFLAGS += -std=c++11 -Wall -Wextra -pedantic
CEPSCLOUDTOOLS = ../../../../tools

INCLUDEPATH += $$CEPSCLOUDTOOLS/include
SOURCES += \
           $$CEPSCLOUDTOOLS/src/vcan_standard_ctrls.cpp \
           $$CEPSCLOUDTOOLS/src/cepscloud_streaming_common.cpp \
           $$CEPSCLOUDTOOLS/src/can_gateway.cpp \
           $$CEPSCLOUDTOOLS/src/cepscloud_streaming_endpoint_ws_api.cpp \
           $$CEPSCLOUDTOOLS/src/vcanstreams.cpp

LIBS +=   -L../../../../dev-tools/lib/win32/qt -lcryptopp


### END cepSCloud Streaming specifics
















