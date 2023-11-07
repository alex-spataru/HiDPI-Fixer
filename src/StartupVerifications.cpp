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

#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QDirIterator>

#ifdef Q_OS_LINUX
#    include <QX11Info>
#endif

#include "Global.h"
#include "StartupVerifications.h"

/**
 * Reads the given user \a args and takes appropiate actions, this function
 * also prohibits non-Linux users and non-X11 users from running the app.
 *
 * \param args The arguments provided by the user or the system
 * \returns \c true If the GUI should not be executed (quit directly after cmd output)
 */
bool StartupVerifications(int argc, char **argv)
{
    // Check if we are running on GNU/Linux
#ifndef Q_OS_LINUX
    QMessageBox::warning(Q_NULLPTR, QObject::tr("Warning"),
                         QObject::tr("This application is intended for Linux "
                                     "distributions only!"));
    return false;
#endif

    // Construct arguments
    QString arguments;
    for (int i = 1; i < argc; ++i)
        arguments.append(argv[i]);

    // Make all arguments lower case (for easier handling)
    arguments = arguments.toLower();

    // Delete everything created by HiDPI Fixer
    if (arguments == "-u" || arguments == "--uninstall")
    {
        // Delete location of scripts
        QDir home(SCRIPTS_HOME);
        if (home.exists())
        {
            if (home.removeRecursively())
                qDebug() << "Folder" << SCRIPTS_HOME << "removed successfully.";
            else
                qDebug() << "[Error] Failed to remove folder" << SCRIPTS_HOME
                         << "you will need to manually remove it.";
        }

        // Delete startup launchers
        QDir dir;
        QDirIterator it(AUTOSTART_LOCATION);
        while (it.hasNext())
        {
            it.next();
            if (it.fileName().contains(AUTOSTART_PATTERN)
                && it.fileName().endsWith(".desktop"))
            {
                if (dir.remove(it.filePath()))
                    qDebug() << "Removed" << qPrintable(it.filePath()) << ".";
                else
                    qDebug() << "[Error] Failed to remove" << qPrintable(it.fileName())
                             << ".";
            }
        }

        // Reset GNOME scaling factor
        QProcess process;
        const QStringList args
            = QStringList { "reset", "org.gnome.desktop.interface", "scaling-factor" };
        process.start("gsettings", args);

        // Notify user
        qDebug() << "Uninstall finished, have a nice day!";

        // Quit app
        return false;
    }

    // Show application version
    else if (arguments == "-v" || arguments == "--version")
    {
        qDebug() << qPrintable(APP_NAME) << "version" << qPrintable(APP_VERSION);
        qDebug() << "Copyright (c) 2018 Alex Spataru <https://github.com/alex-spataru>.";
        qDebug() << "Released under the MIT License.";
        return false;
    }

    // Show help menu
    else if (arguments == "-h" || arguments == "--help")
    {
        qDebug() << "Usage: hidpi-fixer [options]";
        qDebug() << "Where options are:";
        qDebug() << "  -v, --version    Show application version";
        qDebug() << "  -u, --uninstall  Remove all scripts and startup launchers created "
                    "by HiDPI Fixer";
        qDebug() << "  -h, --help       Show this menu";
        return false;
    }

    // Invalid argument, warn user, but run the application
    else if (!arguments.isEmpty())
    {
        qDebug() << "Warning: Invalid argument " << arguments
                 << "type --help to show available options.";
        return true;
    }

    // Check that an XServer is running (we do this check last, so that
    // the user can uninstall on any display server)
#ifdef Q_OS_LINUX
    if (!QX11Info::isPlatformX11())
    {
        QMessageBox::warning(Q_NULLPTR, QObject::tr("Warning"),
                             QObject::tr("You are not running this application "
                                         "on an X11 instance!"));
    }
#endif

    // So far, so good!
    return true;
}
