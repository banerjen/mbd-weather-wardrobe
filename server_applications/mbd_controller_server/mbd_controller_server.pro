#-------------------------------------------------
#
# Project created by QtCreator 2014-11-24T19:33:40
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mbd_controller_server
TEMPLATE = app

INCLUDEPATH += "/usr/include/libxml2"

SOURCES += main.cpp\
        wwcontrollerwindow.cpp

HEADERS  += wwcontrollerwindow.h \
    clothes_representation.h

FORMS    += wwcontrollerwindow.ui

LIBS     += -lyaml-cpp -lxml2
