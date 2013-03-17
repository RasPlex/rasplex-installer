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
    xmlhandler.cpp

HEADERS  += installer.h \
    xmlhandler.h \
    diskwriter.h \
    zlib.h \
    zconf.h

win32 {
    SOURCES += diskwriter_windows.cpp
    HEADERS += diskwriter_windows.h
}
unix {
    SOURCES += diskwriter_unix.cpp
    HEADERS += diskwriter_unix.h
}

FORMS    += installer.ui

LIBS += -L3rd-party -lz
