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

#include <QDebug>
#include <QObject>
#include <QProcess>
#include <QMessageBox>

#include "XRandrBridge.h"

/**
 * Returns a list with all the displays detected by Xrandr
 */
QStringList XrandrGetAvailableDisplays() {
    QProcess process;
    QStringList arguments = {"--listactivemonitors"};

    // Try to run xrandr --listactivemonitors
    process.start ("xrandr", arguments);
    process.waitForFinished (1000);

    // If process fails, abort
    if (process.exitCode()!= 0) {
        QMessageBox::warning (
                    Q_NULLPTR,
                    QObject::tr ("Error"),
                    QObject::tr ("Cannot execute xrandr --listactivemonitors"));
        qWarning() << Q_FUNC_INFO
                   << "xrandr returned exit code"
                   << process.exitCode();
        return QStringList();
    }

    // Used to know if something went bad
    bool ok = true;

    // Get process output
    QString output = QString (process.readAllStandardOutput());
    QStringList lines = output.split (QChar ('\n'));

    // Get monitor count
    QString mcStr = lines.at (0);
    int monitorCount = mcStr.replace (QRegExp ("[^0-9]"), "").toInt (&ok);
    if (!ok) {
        QMessageBox::warning (
                    Q_NULLPTR,
                    QObject::tr ("Error"),
                    QObject::tr ("Cannot get monitor count"));
        qWarning() << Q_FUNC_INFO
                   << "Cannot get monitor count";
        return QStringList();
    }

    // Get name of each monitor
    QStringList displays;
    for (int i = 1; i <= monitorCount; ++i) {
        QStringList monitorInfo = lines.at(i).split (" ");
        displays.append (monitorInfo.last());
    }

    // Check if display list is empty
    if (displays.isEmpty()) {
        QMessageBox::warning (
                    Q_NULLPTR,
                    QObject::tr ("Error"),
                    QObject::tr ("Display list is empty"));
        qWarning() << Q_FUNC_INFO
                   << "Display list is empty";
    }

    // Returned obtained displays
    return displays;
}

/**
 * Returns the modeline string needed to create a resolution
 * with a width of @a w and a height of @h
 */
QString CvtGetModeline(const int w, const int h) {
    Q_ASSERT (w > 0);
    Q_ASSERT (h > 0);

    // Create arguments
    QStringList arguments;
    arguments.append(QString::number(w));
    arguments.append(QString::number(h));

    // Create process
    QProcess process;
    process.start("cvt", arguments);
    process.waitForFinished(1000);

    // If process fails, abort
    if (process.exitCode()!= 0) {
        QMessageBox::warning (
                    Q_NULLPTR,
                    QObject::tr ("Error"),
                    QObject::tr ("Cannot execute cvt %1 %2").arg(w).arg(h));
        qWarning() << Q_FUNC_INFO
                   << "cvt returned exit code"
                   << process.exitCode();
        return "";
    }

    // Get process output
    QString output = QString (process.readAllStandardOutput());

    // Get modeline (construct string backwards until we find two '"' chars)
    QString modeline;
    int quoteCount = 0;
    int pos = output.length();
    while (pos > 0 && quoteCount < 2) {
        --pos;

        if (output.at(pos) == '"')
            ++quoteCount;

        modeline.append(output.at (pos));
    }

    // Invert modeline string
    QString copy = modeline;
    modeline.clear();
    for (int i = 0; i < copy.length(); ++i)
        modeline.append(copy.at (copy.length() - 1 - i));

    // Return modeline
    return modeline;
}

/**
 * Returns the preffered resolution of the given display
 */
QSize XrandrPrefferedResolution (const int display) {
    Q_ASSERT (XrandrGetAvailableDisplays().count() > display);

    // Get display name
    QString dispName = XrandrGetAvailableDisplays().at (display);

    // Get preferred resolution [TODO]
    QSize res;
    res.setWidth(1920);
    res.setHeight(1080);

    // Return resolution size
    return res;
}

/**
 * Returns the resolution name/mode for the given @a modeline
 */
QString CvtGetResolutionName (const QString modeline) {
    Q_ASSERT (!modeline.isEmpty());

    // Construct resulution name (append to string until two '"' chars are found)
    QString name;
    int pos = 0;
    int quoteCharCount = 0;
    while (quoteCharCount < 2 && pos < modeline.length()) {
        if (modeline.at(pos) == '"')
            ++quoteCharCount;

        name.append(modeline.at(pos));
        ++pos;
    }

    // Return result
    return name;
}
