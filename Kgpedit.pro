TARGET = Kgp-edit
TEMPLATE = app
CONFIG += x86 ppc
QT += network \
    webkit  \
    printsupport    \
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
    finddialog.cpp \
    preferencesdialog.cpp \
    codeeditor.cpp \
    participantspane.cpp \
    chatpane.cpp \
    client.cpp \
    findtoolbar.cpp \
    server.cpp \
    chatbrowser.cpp \
    utilities.cpp \
    announcedocumentdialog.cpp \
    firstrundialog.cpp \
    aboutdialog.cpp \
    helpdialog.cpp
HEADERS += mainwindow.h \
    document.h \
    connecttodocument.h \
    finddialog.h \
    preferencesdialog.h \
    codeeditor.h \
    participantspane.h \
    chatpane.h \
    client.h \
    findtoolbar.h \
    enu.h \
    server.h \
    chatbrowser.h \
    utilities.h \
    announcedocumentdialog.h \
    firstrundialog.h \
    aboutdialog.h \
    helpdialog.h
FORMS += mainwindow.ui \
    document.ui \
    connecttodocument.ui \
    finddialog.ui \
    preferencesdialog.ui \
    participantspane.ui \
    chatpane.ui \
    findtoolbar.ui \
    announcedocumentdialog.ui \
    firstrundialog.ui \
    aboutdialog.ui \
    helpdialog.ui
RESOURCES += \
    kgpeditresc.qrc
OTHER_FILES +=
