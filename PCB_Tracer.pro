QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PCB_Tracer
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \

HEADERS += \
    mainwindow.h \

FORMS += \
    mainwindow.ui
