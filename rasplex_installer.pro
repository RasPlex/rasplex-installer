#-------------------------------------------------
#
# Project created by QtCreator 2013-03-14T18:13:26
#
#-------------------------------------------------

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rasplex_installer
TEMPLATE = app


SOURCES += main.cpp\
        installer.cpp \
    xmlhandler.cpp \
    diskwriter.cpp

HEADERS  += installer.h \
    xmlhandler.h \
    diskwriter.h

FORMS    += installer.ui
