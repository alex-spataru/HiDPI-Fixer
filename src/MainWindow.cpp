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
#include <QScreen>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>

#include <QtMath>
#include <QX11Info>

#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // Check if we are running on GNU/Linux
#ifndef Q_OS_LINUX
    QMessageBox::warning (this,
                          tr ("Warning"),
                          tr ("This application is intended for Linux "\
                              "distributions only!"));
    forceQuit();
#endif

    // Check that an XServer is running
    if (!QX11Info::isPlatformX11()) {
        QMessageBox::warning (this,
                              tr ("Warning"),
                              tr ("You are not running this application "\
                                  "on an X11 instance!"));
        forceQuit();
    }

    // Generate UI components
    ui = new Ui::MainWindow;
    ui->setupUi (this);

    // Set monospace font and min. size for script preview
    QFont font;
    font.setPixelSize (10);
    font.setFamily ("Monospace");
    ui->ScriptPreview->setFont (font);
    ui->ScriptPreview->setMinimumWidth (360);
    ui->ScriptPreview->setMinimumHeight (120);

    // Resize window to minimum size
    resize (0, 0);
    setMinimumSize (size());
    setMaximumSize (size());
    setWindowFlags (Qt::WindowCloseButtonHint);

    // Set window title
    setWindowTitle (qApp->applicationName() + " " + qApp->applicationVersion());

    // Connect signals/slots
    connect (ui->QuitMenu,            SIGNAL (triggered()),
             this,                      SLOT (close()));
    connect (ui->CloseButton,         SIGNAL (clicked()),
             this,                      SLOT (close()));
    connect (ui->DisplaysCombo,       SIGNAL (currentIndexChanged (int)),
             this,                      SLOT (updateResolutionCombo (int)));
    connect (ui->ScaleFactor,         SIGNAL (valueChanged (double)),
             this,                      SLOT (generateScript (double)));
    connect (ui->ScriptPreview,       SIGNAL (textChanged()),
             this,                      SLOT (updateScriptExecControls()));
    connect (ui->ResolutionsComboBox, SIGNAL (currentIndexChanged (int)),
             this,                      SLOT (updateScript (int)));
    connect (ui->DisplaysCombo,       SIGNAL (currentIndexChanged (int)),
             this,                      SLOT (updateScript (int)));
    connect (ui->TestButton,          SIGNAL (clicked()),
             this,                      SLOT (testScript()));
    connect (ui->SaveScriptButton,    SIGNAL (clicked()),
             this,                      SLOT (saveScript()));
    connect (ui->SaveScriptMenu,      SIGNAL (triggered()),
             this,                      SLOT (saveScript()));
    connect (ui->WaitTime,            SIGNAL (valueChanged (int)),
             this,                      SLOT (updateScript (int)));
    connect (ui->ReportBugMenu,       SIGNAL (triggered()),
             this,                      SLOT (reportBugs()));
    connect (ui->AboutQtMenu,         SIGNAL (triggered()),
             qApp,                      SLOT (aboutQt()));

    // Populate controls
    ui->ScriptPreview->setPlainText ("");
    ui->AppName->setText (qApp->applicationName());
    ui->DisplaysCombo->addItems(availableDisplays());
}

/**
 * Deletes the UI components when the window is destroyed
 */
MainWindow::~MainWindow()
{
    if (ui != nullptr)
        delete ui;
}

/**
 * Modifies the .profile file (for Qt apps) and creates an autostart
 * job for the script.
 */
void MainWindow::saveScript() {
    // Create script specific to the selected display
    QString dispName = ui->DisplaysCombo->currentText();
    QString scriptPath = QString ("%1/.hdpi-fixer/scripts/%2")
            .arg (QDir::homePath())
            .arg (dispName);

    // There was an error saving (or running the script)
    if (saveAndExecuteScript (scriptPath) != 0) {
        qWarning() << Q_FUNC_INFO
                   << "Error while saving/running"
                   << scriptPath;
        return;
    }

    // Modify Qt DPI settings
    if (ui->FixQtDpiCheckbox->isChecked()) {
        // Get .profile file path
        QString profilePath = QString ("%1/.profile").arg (QDir::homePath());

        // Open file for editing
        QFile file (profilePath);
        if (file.open (QIODevice::WriteOnly | QIODevice::Append)) {
            QString qssf = QString::number (qCeil (ui->ScaleFactor->value()));
            QString cmd = "\n"
                          "# Adapt Qt apps to HiDPI display config [HiDPI-Fixer]\n"
                          "export QT_SCALE_FACTOR=1\n"
                          "export QT_AUTO_SCREEN_SCALE_FACTOR=0\n"
                          "export QT_SCREEN_SCALE_FACTORS=" + qssf + "\n";

            // Append changes to current profile data
            file.write (cmd.toUtf8());
            file.close();
        }

        // There was an error editing .profile
        else {
            qWarning() << Q_FUNC_INFO
                       << "Cannot open"
                       << profilePath
                       << "for reading/writing!";
            QMessageBox::warning (this,
                                  tr ("Error"),
                                  tr ("Cannot open \"%1\" for editing!")
                                  .arg (profilePath));
        }
    }

    // Get launcher file name
    QString launcherPath = QString ("%1/.config/autostart/%2.desktop")
            .arg (QDir::homePath())
            .arg (dispName);

    // Create .config and autostart folders if not present
    QFileInfo info (launcherPath);
    QDir dir (info.absolutePath());
    if (!dir.exists())
        dir.mkpath (".");

    // Create launcher file
    QFile file (launcherPath);
    if (file.open (QFile::WriteOnly)) {
        // Set launcher data
        QString data = "[Desktop Entry]\n"
                       "Type=Application\n"
                       "Exec=bash \"" + scriptPath +"\"\n"
                       "Hidden=false\n"
                       "NoDisplay=false\n"
                       "X-GNOME-Autostart-enabled=true\n"
                       "Name=Apply HiDPI Config for " + dispName + "\n"
                       "Comment=Created by HiDPI-Fixer";

        // Write launcher data to file
        file.write (data.toUtf8());
        file.close();

        // Notify user
        QMessageBox::information (this,
                                  tr ("Info"),
                                  tr ("Changes applied, its recommended to logout "
                                      "and login again to test that the script works as intended."));
    }
}

/**
 * Saves the current script to the test script location and
 * executes the script.
 */
void MainWindow::testScript() {
    QString filePath = QString ("%1/.hdpi-fixer/runtime/test").arg (QDir::homePath());
    saveAndExecuteScript (filePath);
}

/**
 * Opens the GitHub issues page
 */
void MainWindow::reportBugs() {
    QDesktopServices::openUrl (QUrl ("https://github.com/alex-spataru/HiDPI-Fixer/issues"));
}

/**
 * Enables or disables the test script and save script controls
 * depending if the script is available or not
 */
void MainWindow::updateScriptExecControls()
{
    // There is not script available, disable test and save buttons
    QString script = ui->ScriptPreview->document()->toPlainText();
    if (script.length() == 0) {
        ui->TestButton->setEnabled (false);
        ui->SaveScriptMenu->setEnabled (false);
        ui->SaveScriptButton->setEnabled (false);
    }

    // The script is available, enable controls
    else {
        ui->TestButton->setEnabled (true);
        ui->SaveScriptMenu->setEnabled (true);
        ui->SaveScriptButton->setEnabled (true);
    }
}

/**
 * Dummy function, used to re-generate the script when the
 * user changes the display and/or the resolution
 */
void MainWindow::updateScript (const int unused) {
    (void) unused;
    generateScript (ui->ScaleFactor->value());
}

/**
 * Generates a script that uses xrandr to resize the
 * contents of the screen to the given \a scale
 */
void MainWindow::generateScript (const qreal scale)
{
    // Calculate int scaling factor
    int factor = qCeil (scale);

    // Calculate the screen multiplying factor
    qreal multFactor = factor / scale;

    // Scale factor is 1...we don't need a script!
    if (factor == 1) {
        ui->ScriptPreview->setPlainText ("");
        return;
    }

    // Check if current selected resolution is valid
    QStringList size = ui->ResolutionsComboBox->currentText().split ("x");
    if (size.count() != 2) {
        qWarning() << "Invalid resolution" << ui->ResolutionsComboBox->currentText();
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Invalid resolution \"%1\"!").arg (ui->ResolutionsComboBox->currentText()));
        return;
    }

    // Get resolution width and height
    int width = size.at (0).toInt();
    int height = size.at (1).toInt();

    // Calculate panning
    int pWidth = qCeil (width * multFactor);
    int pHeight = qCeil (height * multFactor);

    // Construct xrandr command
    QString xcmd;
    xcmd.append (QString ("xrandr --output %1 --size %2x%3\n")
                 .arg (ui->DisplaysCombo->currentText())
                 .arg (width)
                 .arg (height));
    xcmd.append (QString ("xrandr --output %1 --scale %2x%2 --panning %3x%4")
                 .arg (ui->DisplaysCombo->currentText())
                 .arg (multFactor)
                 .arg (pWidth)
                 .arg (pHeight));

    // Add shebang
    QString script;
    script.append ("#!/bin/bash\n\n");

    // Wait time (to apply changes after GNOME)
    script.append ("# Wait some seconds before applying changes\n");
    script.append ("sleep " + QString::number (ui->WaitTime->value()));
    script.append ("\n\n");

    // Enable rotation lock (to avoid ugly shit when rotating the screen)
    script.append ("# Enable rotation lock  to avoid\n"
                   "# issues with xrandr.\n");
    script.append ("gsettings set org.gnome.settings-daemon.peripherals.touchscreen orientation-lock true");
    script.append ("\n\n");

    // Set GNOME scaling factor (int)
    script.append ("# Set GNOME's scaling factor\n"
                   "# Modify this if using another\n"
                   "# DE environment.\n");
    script.append ("gsettings set org.gnome.desktop.interface scaling-factor ");
    script.append (QString::number (factor));
    script.append ("\n\n");

    // Xrandr code
    script.append ("# Xrandr scaling hack, --panning is used\n"
                   "# in order to let the mouse navigate\n"
                   "# in all of the 'generated' screen space.\n");
    script.append (xcmd);
    script.append ("\n");

    // Echo code
    script.append ("echo \"Script finished execution\"\n");

    // Update controls
    ui->ScriptPreview->setPlainText (script);
}

/**
 * Updates the resolutions ComboBox when the user selects
 * another display
 */
void MainWindow::updateResolutionCombo (const int index)
{
    ui->ResolutionsComboBox->clear();
    ui->ResolutionsComboBox->addItems (availableResolutions(index));
}

/**
 * Forces the application to quit.
 * This function is only used when the application is launched
 * on an OS different than GNU/Linux or is not executed with a X Server.
 */
void MainWindow::forceQuit() {
    quick_exit (EXIT_FAILURE);
}

/**
 * Saves the script to the given location (creating the directories if
 * necessary), makes the new file executable and tries to execute
 * the newly created script/program.
 */
int MainWindow::saveAndExecuteScript (const QString& location) {
    // Get script text
    QString script = ui->ScriptPreview->document()->toPlainText();

    // Script is empty
    if (script.isEmpty()) {
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("The script is empty!"));
        return 1;
    }

    // Create .hdpi-fixer folder if not present
    QFileInfo info (location);
    QDir dir (info.absolutePath());
    if (!dir.exists())
        dir.mkpath (".");

    // Save script to file
    QFile file (location);
    if (file.open (QFile::WriteOnly)) {
        file.write (script.toUtf8());
        file.close();
    }

    // Cannot open file for writing, warn user
    else {
        qWarning() << Q_FUNC_INFO
                   << "Cannot open" << file.fileName() << "for writing!";
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot open %1 for writing!").arg (file.fileName()));
        return 1;
    }

    // Make script executable
    QProcess chmod;
    QStringList arguments = {"+x", file.fileName()};
    if (chmod.execute ("chmod", arguments) != 0) {
        qWarning() << Q_FUNC_INFO
                   << "Cannot execute chmod" << arguments;
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot make file \"%1\" executable!")
                              .arg (file.fileName()));
        return 1;
    }

    // Run file
    QProcess test;
    if (test.execute (file.fileName()) != 0) {
        qWarning() << Q_FUNC_INFO
                   << "Cannot execute" << file.fileName();
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot run script at %1").arg (file.fileName()));
        return 1;
    }

    // Notify caller that everything is OK
    return 0;
}

/**
 * Returns a list with all the displays detected by Xrandr
 */
QStringList MainWindow::availableDisplays() {
    QProcess process;
    QStringList arguments = {"--listactivemonitors"};

    // Try to run xrandr --listactivemonitors
    process.start ("xrandr", arguments);
    process.waitForFinished (1000);

    // If process fails, abort
    if (process.exitCode()!= 0) {
        qWarning() << Q_FUNC_INFO
                   << "xrandr returned exit code"
                   << process.exitCode();
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot execute xrandr --listactivemonitors"));
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
        qWarning() << Q_FUNC_INFO
                   << "Cannot get monitor count";
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot get monitor count"));
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
        qWarning() << Q_FUNC_INFO
                   << "Error while obtaining monitor names";
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot obtain monitor names"));
    }

    // Returned obtained displays
    return displays;
}

/**
 * Returns a list with the available resolutions reported
 * by the xrandr-process for the given display
 */
QStringList MainWindow::availableResolutions (const int displayIndex) {
    Q_ASSERT (displayIndex >= 0);

    QProcess process;
    QStringList arguments = {"--screen", QString::number (displayIndex)};

    // Try to run xrandr --screen $displayIndex
    process.start ("xrandr", arguments);
    process.waitForFinished (1000);

    // If process fails, abort
    if (process.exitCode() != 0) {
        qWarning() << Q_FUNC_INFO
                   << "xrandr returned exit code"
                   << process.exitCode();
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot obtain available resolutions"));
        return QStringList();
    }

    // Get process output
    QString output = QString (process.readAllStandardOutput());

    // Get resolutions
    QStringList resolutions;
    QRegExp rx ("([0-9])+x+([0-9]*)+     ");
    while (output.contains (rx)) {
        // Get index of match
        int pos = rx.indexIn (output);

        // Add match to resolutions (and avoid duplicates)
        if (!rx.capturedTexts().isEmpty())
            if (!resolutions.contains (rx.capturedTexts().first()))
                resolutions.append (rx.capturedTexts().first());

        // Remove match from original string
        output.remove (pos, resolutions.last().length());
    }

    // Return obtained resolutions
    return resolutions;
}
