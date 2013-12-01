#-------------------------------------------------
#
# Project created by QtCreator 2013-03-14T18:13:26
#
#-------------------------------------------------

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rasplex-installer
TEMPLATE = app

SOURCES += main.cpp\
    installer.cpp \
    xmlhandler.cpp \
    confighandler.cpp \
    downloadmanager.cpp \
    diskwriter.cpp

static { # everything below takes effect with CONFIG += static
    CONFIG += static
    CONFIG += staticlib # this is needed if you create a static library, not a static executable
    DEFINES += STATIC
    message("~~~ static build ~~~") # this is for information, that the static build is done
}

HEADERS  += installer.h \
    xmlhandler.h \
    diskwriter.h \
    zlib.h \
    zconf.h \
    confighandler.h \
    downloadmanager.h \
    deviceenumerator.h

win32 {
    SOURCES += diskwriter_windows.cpp \
        confighandler_windows.cpp \
        deviceenumerator_windows.cpp
    HEADERS += diskwriter_windows.h \
        confighandler_windows.h \
        deviceenumerator_windows.h
    CONFIG += rtti
    QMAKE_LFLAGS  = -static -static-libgcc
    RC_FILE = rasplex-installer.rc
}
unix {
    QMAKE_CXXFLAGS += -fPIC
    SOURCES += diskwriter_unix.cpp \
        confighandler_unix.cpp \
        deviceenumerator_unix.cpp
    HEADERS += diskwriter_unix.h \
        confighandler_unix.h \
        deviceenumerator_unix.h
    LIBS += -lblkid
}

FORMS    += installer.ui

LIBS += -L3rd-party -lz

OTHER_FILES +=

RESOURCES += \
    resources.qrc
