QT       += core testlib network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++11 testcase
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += \
    main.cpp \
    tst_protocol.cpp \
    tst_format.cpp

HEADERS += \
    tst_protocol.h \
    tst_format.h

# 包含上层源码路径（如需直接引用 serve1/client1 的头文件）
INCLUDEPATH += .. \
               ../../C/MyClient1 \
               ../../W/MyClient1
