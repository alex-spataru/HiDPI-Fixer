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

#include "Global.h"
#include "MainWindow.h"

#include <QDebug>
#include <QDirIterator>

/**
 * Reads the given user \a args and takes appropiate actions
 *
 * \param args The arguments provided by the user or the system
 * \returns \c true If the GUI should not be executed (quit directly after cmd output)
 */
static bool ManageArguments (const QString& args) {
    // Used to know whenever to launch the app or not
    bool quit = false;

    // Make arguments lower case (to simplify checking)
    QString argument = args.toLower();

    // Delete everything created by HiDPI Fixer
    if (argument == "-u" || argument == "--uninstall") {
        // Delete location of scripts
        QDir home (HiDPI_FixerHome);
        if (home.exists()) {
            if (home.removeRecursively())
                qDebug() << "Folder" << HiDPI_FixerHome << "removed successfully";
            else
                qDebug() << "[Error] Failed to remove folder" << HiDPI_FixerHome
                         << "you will need to manually remove it";
        }

        // Delete startup launchers
        QDir dir;
        QDirIterator it (HiDPI_AutostartDir);
        while (it.hasNext())
        {
            it.next();
            if (it.fileName().contains (HiDPI_AutostartBase) && 
                it.fileName().endsWith (".desktop")) {
                if (dir.remove (it.filePath()))
                    qDebug() << "Removed" << qPrintable (it.filePath());
                else
                    qDebug() << "[Error] Failed to remove" 
                             << qPrintable (it.fileName());
            }
        }

        // Notify user
        qDebug() << "Uninstall finished, have a nice day!";

        // Quit app
        quit = true;
    }

    // Show application version
    else if (argument == "-v" || argument == "--version") {
        quit = true;
        qDebug() << qPrintable (qApp->applicationName()) << "version" 
                 << qPrintable (qApp->applicationVersion());
        qDebug() << "Copyright (c) 2018 Alex Spataru <https://github.com/alex-spataru>";
        qDebug() << "Released under the MIT License";
    }

    // Show help menu
    else if (!argument.isEmpty()) {
        if (argument != "-h" && argument != "--help")
            qDebug() << "Invalid argument " << argument;
        else
            quit = true;

        qDebug() << "Usage: hidpi-fixer [options]";
        qDebug() << "Where options are:";
        qDebug() << "  -v, --version    Show application version";
        qDebug() << "  -u, --uninstall  Remove all scripts and startup launchers created by HiDPI Fixer";
        qDebug() << "  -h, --help       Show this menu";
    }

    // Return quit status
    return quit;
}

/**
 * Main entry point of the application
 *
 * \param argc Argument count
 * \param argv Argument data
 */
int main (int argc, char **argv)
{
    QApplication app (argc, argv);
    app.setApplicationVersion ("1.1");
    app.setApplicationName ("HiDPI Fixer");

    QString arguments;
    for (int i = 1; i < argc; ++i)
        arguments.append (argv [i]);

    if (!ManageArguments (arguments)) {
        MainWindow window;
        window.show();
        return app.exec();
    }

    return 0;
}
