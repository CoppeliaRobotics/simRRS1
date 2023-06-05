include(config.pri)

#QT -= core #core needed because of QProcess::startDetached
QT -= gui

TARGET = simRRS1
TEMPLATE = lib

DEFINES -= UNICODE
DEFINES += QT_COMPIL
CONFIG += shared plugin
INCLUDEPATH += "../include"

*-msvc* {
    QMAKE_CXXFLAGS += -O2
    QMAKE_CXXFLAGS += -W3
}
*-g++* {
    QMAKE_CXXFLAGS += -O3
    QMAKE_CXXFLAGS += -Wall
    QMAKE_CXXFLAGS += -fvisibility=hidden
    QMAKE_CXXFLAGS += -Wno-unused-parameter
    QMAKE_CXXFLAGS += -Wno-strict-aliasing
    QMAKE_CXXFLAGS += -Wno-empty-body
    QMAKE_CXXFLAGS += -Wno-write-strings

    QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
    QMAKE_CXXFLAGS += -Wno-unused-local-typedefs
    QMAKE_CXXFLAGS += -Wno-narrowing

    QMAKE_CFLAGS += -O3
    QMAKE_CFLAGS += -Wall
    QMAKE_CFLAGS += -Wno-strict-aliasing
    QMAKE_CFLAGS += -Wno-unused-parameter
    QMAKE_CFLAGS += -Wno-unused-but-set-variable
    QMAKE_CFLAGS += -Wno-unused-local-typedefs
}

INCLUDEPATH += $$BOOST_INCLUDEPATH

win32 {
    DEFINES += WIN_SIM
    LIBS += -lwinmm
    LIBS += -lWs2_32
    LIBS += -lShell32
}

macx {
    DEFINES += MAC_SIM
}

unix:!macx {
    DEFINES += LIN_SIM
}

SOURCES += \
    ../include/simLib/socketOutConnection.cpp \
    ../include/simLib/scriptFunctionData.cpp \
    ../include/simLib/scriptFunctionDataItem.cpp \
    ../include/simLib/simLib.cpp \
    inputOutputBlock.cpp \
    simRRS1.cpp

HEADERS +=\
    ../include/simLib/socketOutConnection.h \
    ../include/simLib/scriptFunctionData.h \
    ../include/simLib/scriptFunctionDataItem.h \
    ../include/simLib/simLib.h \
    inputOutputBlock.h \
    simRRS1.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

