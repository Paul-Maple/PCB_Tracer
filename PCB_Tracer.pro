QT += core gui concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent

TARGET = PCB_Tracer
TEMPLATE = app

# Увеличьте лимиты для лучшей производительности
QMAKE_CXXFLAGS += -fopenmp -O3 -march=native
LIBS += -fopenmp
CONFIG += c++11

# Добавьте поддержку OpenMP
QMAKE_CXXFLAGS += -fopenmp
LIBS += -fopenmp

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    trace.cpp \
    routingtask.cpp \
    helpdialog.cpp

HEADERS += \
    mainwindow.h \
    trace.h \
    routingtask.h \
    helpdialog.h

FORMS += \
    mainwindow.ui \
    helpdialog.ui
