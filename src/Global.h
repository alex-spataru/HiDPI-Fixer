/*
 * Copyright (c) 2018 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QDir>
#include <QString>

/**
 * Define application information
 */
static const QString APP_VERSION = "1.4";
static const QString APP_NAME = "HiDPI Fixer";

/**
 * Defines the folder in which the HiDPI-Fixer scripts are stored
 */
static const QString SCRIPTS_HOME = QString("%1/.hidpi-fixer").arg(QDir::homePath());

/**
 * Defines the file location and name pattern for startup scripts
 */
static const QString AUTOSTART_PATTERN = "HiDPI-Fixer_";
static const QString AUTOSTART_LOCATION = QString("%1/.config/autostart").arg(QDir::homePath());

#endif
