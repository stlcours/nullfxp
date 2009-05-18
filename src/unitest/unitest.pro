# unitest.pro --- 
# 
# Author: liuguangzhao
# Copyright (C) 2007-2010 liuguangzhao@users.sf.net
# URL: http://www.qtchina.net http://nullget.sourceforge.net
# Created: 2009-05-18 22:04:17 +0800
# Last-Updated: 
# Version: $Id$
# 

TEMPLATE = app
TARGET = 
DESTDIR = ../../bin
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += .. ../libssh2/include/

QT += testlib

# Input
HEADERS += testnullfxp.h
SOURCES += testmain.cpp testnullfxp.cpp

SOURCES += ../basestorage.cpp ../utils.cpp ../globaloption.cpp \
../sshfileinfo.cpp

win32 {
    win32-g++ {
    } else {
        DEFINES += LIBSSH2_WIN32 _CRT_SECURE_NO_DEPRECATE
         CONFIG -= embed_manifest_exe
         CONFIG -= embed_manifest_dll
    }
    RC_FILE = unitest.rc
}

win32:LIBS += -lQtTest -lws2_32  -lgdi32 

target.path = /usr/bin
INSTALLS += target
