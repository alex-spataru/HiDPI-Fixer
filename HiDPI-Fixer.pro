#
# Copyright (c) 2018 Alex Spataru <https://github.com/alex-spataru>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

#-------------------------------------------------------------------------------
# Project configuration
#-------------------------------------------------------------------------------

TEMPLATE = app
TARGET = HiDPI-Fixer

#-------------------------------------------------------------------------------
# Import Qt modules
#-------------------------------------------------------------------------------

QT += gui
QT += core
QT += widgets

#-------------------------------------------------------------------------------
# Deploy config
#-------------------------------------------------------------------------------

linux:!android {
    QT += x11extras

    target.path = /usr/bin
    icon.path = /usr/share/pixmaps
    desktop.path = /usr/share/applications
    icon.files += $$PWD/deploy/linux/hidpi-fixer.png
    desktop.files += $$PWD/deploy/linux/hidpi-fixer.desktop

    TARGET = hidpi-fixer
    INSTALLS += target desktop icon
}

#-------------------------------------------------------------------------------
# Make options
#-------------------------------------------------------------------------------

UI_DIR = uic
MOC_DIR = moc
RCC_DIR = qrc
OBJECTS_DIR = obj

CONFIG += c++11

#-------------------------------------------------------------------------------
# Import source code
#-------------------------------------------------------------------------------

SOURCES += \
    $$PWD/src/main.cpp \
    $$PWD/src/MainWindow.cpp \
    $$PWD/src/StartupVerifications.cpp \
    $$PWD/src/XRandrBridge.cpp

HEADERS += \
    $$PWD/src/MainWindow.h \
    $$PWD/src/Global.h \
    $$PWD/src/StartupVerifications.h \
    $$PWD/src/XRandrBridge.h

FORMS += \
    $$PWD/src/MainWindow.ui
    
#-------------------------------------------------------------------------------
# Import resources
#-------------------------------------------------------------------------------

RESOURCES += \
    $$PWD/images/images.qrc
