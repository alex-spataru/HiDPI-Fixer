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
        QStringList monitorInfo = lines.at(i).split (QChar(' '));
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
 * Returns a list with the available resolutions reported
 * by the xrandr-process for the given display
 */
QStringList XrandrGetAvailableResolutions (const int display) {
    Q_ASSERT (display >= 0);

    // Try to run xrandr --screen $display
    QProcess process;
    process.start ("xrandr");
    process.waitForFinished (1000);

    // If process fails, abort
    if (process.exitCode() != 0) {
        QMessageBox::warning (
                    Q_NULLPTR,
                    QObject::tr ("Error"),
                    QObject::tr ("Cannot run xrandr"));
        qWarning() << Q_FUNC_INFO
                   << "xrandr returned exit code"
                   << process.exitCode();
        return QStringList();
    }

    // Get process output
    QString output = QString (process.readAllStandardOutput());

    // Separate process output lines
    QStringList lines = output.split("\n");

    // Remove all lines that do not contain a resolution
    // or connected display flags
    QStringList usefulInformation;
    for (int i = 0; i < lines.count(); ++i) {
        // Add lines that contain resultions
        if (lines.at(i).startsWith("   "))
            usefulInformation.append(lines.at(i));

        // Add lines that contain info about connected displays
        else if (lines.at(i).contains(" connected"))
            usefulInformation.append(lines.at(i));
    }

    // Create resolution info list
    // Root:
    //   - Item 1
    //      - <Display name 1>
    //      - <Resolutions for display 1>
    //   - Item 2
    //      - <Display name 2>
    //      - <Resolutions for display 2>
    //   - ...
    int displayCount = -1;
    QList<QStringList> screenInformation;
    for (int i = 0; i < usefulInformation.count(); ++i) {
        // Line contains display information, add new entry to table
        if (!usefulInformation.at(i).startsWith(" ")) {
            // Create list
            QStringList display;

            // Get display name
            int pos = 0;
            QString name;
            QString displayDetails = usefulInformation.at(i);
            while (displayDetails.at(pos) != QChar(' ')) {
                name.append(displayDetails.at(pos));
                ++pos;
            }

            // Begin list with display name
            display.append(name);

            // Add list to root
            screenInformation.append(display);

            // Update display count (to register remaining resolutions
            // with the new display
            ++displayCount;
        }

        // Append next resolution to current display (if any)
        else if (screenInformation.count() > 0 && displayCount >= 0) {
            // Get resolution mode line
            QString modeDetails = usefulInformation.at(i);

            // Create regular expresion to match only the resolution
            // string (<width>x<height>)
            QRegExp rx ("([0-9])+x+([0-9]*)+     ");

            // Match found, register resolution
            if (modeDetails.contains(rx)) {
                // Get copy of current display information
                QStringList currentData = screenInformation.at(displayCount);

                // Add match to resolution list
                QString resolution = rx.capturedTexts().first();
                if (!currentData.contains(resolution)) {
                    currentData.append(resolution);
                    screenInformation.replace(displayCount, currentData);
                }
            }
        }
    }

    // Get resolutions for current display
    QStringList resolutions;
    QString displayName = XrandrGetAvailableDisplays().at(display);
    for (int i = 0; i < screenInformation.count(); ++i) {
       QString name = screenInformation.at(i).first();
       if (name == displayName) {
           resolutions = screenInformation.at(i);
           resolutions.removeFirst();
       }
    }

    // Validate resolution list
    QStringList validatedResolutions;
    for (int i = 0; i < resolutions.count(); ++i) {
        // Check if resolution string is valid (<width>x<height>)
        QStringList size = resolutions.at(i).split('x');
        if (size.count() == 2) {
            // Get resolution size
            int w = size.at(0).toInt();
            int h = size.at(1).toInt();

            // Skip resultions smaller than 640x480
            if (w >= 640 && h >= 480)
                validatedResolutions.append(resolutions.at(i));
        }
    }

    // Return obtained resolutions
    return validatedResolutions;
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

    // Remove linebreak characters from modeline
    modeline = modeline.replace(QChar('\n'), "");

    // Return modeline
    return modeline;
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
