include(config.pri)

#QT -= core #core needed because of QProcess::startDetached
QT -= gui

TARGET = v_repExtRRS1
TEMPLATE = lib

DEFINES -= UNICODE
DEFINES += QT_COMPIL
CONFIG += shared
INCLUDEPATH += "../include"

*-msvc* {
    QMAKE_CXXFLAGS += -O2
    QMAKE_CXXFLAGS += -W3
}
*-g++* {
    QMAKE_CXXFLAGS += -O3
    QMAKE_CXXFLAGS += -Wall
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
    DEFINES += WIN_VREP
    LIBS += -lwinmm
    LIBS += -lWs2_32
    LIBS += -lShell32
}

macx {
    DEFINES += MAC_VREP
}

unix:!macx {
    DEFINES += LIN_VREP
}

SOURCES += \
    ../common/socketOutConnection.cpp \
    ../common/scriptFunctionData.cpp \
    ../common/scriptFunctionDataItem.cpp \
    ../common/v_repLib.cpp \
    inputOutputBlock.cpp \
    v_repExtRRS1.cpp

HEADERS +=\
    ../include/socketOutConnection.h \
    ../include/scriptFunctionData.h \
    ../include/scriptFunctionDataItem.h \
    ../include/v_repLib.h \
    inputOutputBlock.h \
    v_repExtRRS1.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

