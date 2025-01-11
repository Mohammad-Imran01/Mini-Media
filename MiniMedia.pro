QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = miniMedia

RC_ICONS = miniMedia.ico

CONFIG += c++17
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



FORMS += \
    src/gui/mainwindow.ui

HEADERS += \
    src/common/imagecropper.h \
    src/gui/mainwindow.h \
    src/gui/mediaplayer.h \
    src/theme/themehandler.h

SOURCES += \
    src/common/imagecropper.cpp \
    src/gui/mainwindow.cpp \
    src/gui/mediaplayer.cpp \
    src/main.cpp \
    src/theme/themehandler.cpp

RESOURCES += \
    resource.qrc
