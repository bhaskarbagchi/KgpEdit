TARGET = Kgp-edit
TEMPLATE = app
CONFIG += x86 ppc
QT += network \
    webkit  \
    webkitwidgets

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
# Windows RC file
RC_FILE = kgpeditresc.qrc

# Mac icon/plist
ICON =
QMAKE_INFO_PLIST =
SOURCES += main.cpp \
    mainwindow.cpp \
    document.cpp \
    connecttodocument.cpp \
    codeeditor.cpp \
    participantspane.cpp \
    chatpane.cpp \
    client.cpp \
    server.cpp \
    chatbrowser.cpp \
    utilities.cpp \
    announcedocumentdialog.cpp \
    firstrundialog.cpp
HEADERS += mainwindow.h \
    document.h \
    connecttodocument.h \
    codeeditor.h \
    participantspane.h \
    chatpane.h \
    client.h \
    enu.h \
    server.h \
    chatbrowser.h \
    utilities.h \
    announcedocumentdialog.h \
    firstrundialog.h
FORMS += mainwindow.ui \
    document.ui \
    connecttodocument.ui \
    participantspane.ui \
    chatpane.ui \
    announcedocumentdialog.ui \
    firstrundialog.ui
RESOURCES += \
    kgpeditresc.qrc
OTHER_FILES +=
