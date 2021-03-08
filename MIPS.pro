#-------------------------------------------------
#
# Project created by QtCreator 2015-06-27T10:54:52
#
#-------------------------------------------------

QT       += core
QT       += gui
QT       += serialport
QT       += network
QT       += widgets printsupport
QT       += script

win32:RC_ICONS += GAACElogo.ico
ICON = GAACElogo.icns

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MIPS
TEMPLATE = app

//CONFIG += static

SOURCES += main.cpp\
        mips.cpp \
        console.cpp \
        settingsdialog.cpp \
        ringbuffer.cpp \
        pse.cpp \
        comms.cpp \
        twave.cpp \
    dcbias.cpp \
    dio.cpp \
    rfdriver.cpp \
    psg.cpp \
    program.cpp \
    help.cpp \
    arb.cpp \
    faims.cpp \
    singlefunnel.cpp \
    filament.cpp \
    softlanding.cpp \
    grid.cpp \
    arbwaveformedit.cpp \
    qcustomplot.cpp \
    psviewer.cpp \
    softlanding2.cpp \
    adc.cpp \
    controlpanel.cpp \
    mipscomms.cpp \
    cmdlineapp.cpp \
    script.cpp \
    cdirselectiondlg.cpp \
    scriptingconsole.cpp \
    rfamp.cpp \
    tcpserver.cpp \
    timinggenerator.cpp \
    compressor.cpp \
    properties.cpp \
    plot.cpp \
    device.cpp \
    shuttertg.cpp

HEADERS  += mips.h \
    console.h \
    settingsdialog.h \
    ringbuffer.h \
    pse.h \
    comms.h \
    twave.h \
    dcbias.h \
    dio.h \
    rfdriver.h \
    psg.h \
    program.h \
    help.h \
    arb.h \
    faims.h \
    singlefunnel.h \
    filament.h \
    softlanding.h \
    grid.h \
    arbwaveformedit.h \
    qcustomplot.h \
    psviewer.h \
    softlanding2.h \
    adc.h \
    controlpanel.h \
    mipscomms.h \
    cmdlineapp.h \
    script.h \
    cdirselectiondlg.h \
    scriptingconsole.h \
    rfamp.h \
    tcpserver.h \
    timinggenerator.h \
    compressor.h \
    properties.h \
    plot.h \
    device.h \
    shuttertg.h

FORMS    += mips.ui \
    settingsdialog.ui \
    pse.ui \
    help.ui \
    singlefunnel.ui \
    softlanding.ui \
    grid.ui \
    arbwaveformedit.ui \
    psviewer.ui \
    softlanding2.ui \
    controlpanel.ui \
    mipscomms.ui \
    cmdlineapp.ui \
    script.ui \
    scriptingconsole.ui \
    rfamp.ui \
    timinggenerator.ui \
    compressor.ui \
    properties.ui \
    plot.ui \
    shuttertg.ui

RESOURCES += \
    files.qrc

QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.01
QMAKE_MAC_SDK = macosx10.14









