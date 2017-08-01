#-------------------------------------------------
#
# Project created by QtCreator 2016-06-07T14:07:48
#
#-------------------------------------------------

QT       += core gui
QT       += network

QMAKE_CXXFLAGS += -std=gnu++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

unix {
    include (/usr/local/qwt-6.1.3/features/qwt.prf)
}

win32 {
    include (C:/qwt-6.1.3/features/qwt.prf)
}

TARGET = kc901
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    kcscalewidget.cpp \
    smithchart.cpp

HEADERS  += mainwindow.h \
    kcscalewidget.h \
    smithchart.h

FORMS    += mainwindow.ui

RESOURCES += \
    skin_darkorange.qrc

TRANSLATIONS = languages/en.ts  languages/de.ts
