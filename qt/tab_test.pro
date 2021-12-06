QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

RC_FILE = icon.rc

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    image_change.cpp \
    imageitem.cpp \
    imagescene.cpp \
    imagewidget.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Launch.h \
    image_change.h \
    imageitem.h \
    imagescene.h \
    imagewidget.h \
    mainwindow.h \
    pangolin_lib.h \
    reconstruction.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += D:/download/opencv450/build/install/include
INCLUDEPATH += D:/download/opencv450/build/install/include/opencv2
INCLUDEPATH += D:/download/vcpkg/installed/x64-windows/include

LIBS += -LD:/download/opencv450/build/install/x64/vc16/bin
LIBS += -LD:/download/opencv450/build/install/x64/vc16/lib -lopencv_world450
LIBS += -LD:/download/vcpkg/installed/x64-windows/bin
LIBS += -LD:/download/vcpkg/installed/x64-windows/lib -lpangolin -lavcodec -lavfilter -ljpeg \
                                                      -lavformat -lavutil -lavdevice -lGlU32  -lzlib \
                                                      -llibpng16 -lOpenGL32 -lswresample -lswscale -lturbojpeg -lglew32
LIBS += -lGdi32 -lUser32

win32: LIBS += -L$$PWD/./ -lreconstruction

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/./reconstruction.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/./libreconstruction.a

win32: LIBS += -L$$PWD/./ -lStaticLib1

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/./StaticLib1.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/./libStaticLib1.a

win32: LIBS += -L$$PWD/./ -lpangolin_lib

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/./pangolin_lib.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/./libpangolin_lib.a

DISTFILES += \
    StaticLib1.lib \
    pangolin_lib.lib \
    reconstruction.lib
