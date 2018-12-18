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
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopServices>

#include <cmath>

#include "Global.h"
#include "MainWindow.h"
#include "XRandrBridge.h"

#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // Generate UI components
    ui = new Ui::MainWindow;
    ui->setupUi (this);

    // Set monospace font and min. size for script preview
    QFont font;
    font.setPixelSize (10);
    font.setFamily ("Monospace");
    ui->ScriptPreview->setFont (font);
    ui->ScriptPreview->setMinimumWidth (390);
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
    connect (ui->ReportBugMenu,       SIGNAL (triggered()),
             this,                      SLOT (reportBugs()));
    connect (ui->AboutQtMenu,         SIGNAL (triggered()),
             qApp, SLOT (aboutQt()));

    // Populate controls
    ui->ScriptPreview->setPlainText ("");
    ui->AppName->setText (qApp->applicationName());
    ui->DisplaysCombo->addItems (XrandrGetAvailableDisplays());
}

/**
 * Deletes the UI components when the window is destroyed
 */
MainWindow::~MainWindow()
{
    // Delete test script
    QFile file (SCRIPTS_HOME + "/test");
    if (file.exists())
        file.remove();

    // De-allocate UI memory
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
    QString scriptPath = QString ("%1/scripts/%2")
            .arg (SCRIPTS_HOME)
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
            QString qssf = QString::number ((int) ceil (ui->ScaleFactor->value()));
            QString cmd = "\n"
                          "# Adapt Qt apps to HiDPI config [HiDPI-Fixer]\n"
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
    QString launcherPath = AUTOSTART_LOCATION + "/" +
            AUTOSTART_PATTERN + dispName + ".desktop";

    // Create .config and autostart folders if not present
    QFileInfo info (launcherPath);
    QDir dir (info.absolutePath());
    if (!dir.exists())
        dir.mkpath (".");

    // Create launcher file
    QFile file (launcherPath);
    if (file.open (QFile::WriteOnly)) {
        // Set launcher data
        QString data = QString ("[Desktop Entry]\n"
                                "Type=Application\n"
                                "Exec=bash \"" + scriptPath +
                                "\"\n"
                                "Hidden=false\n"
                                "NoDisplay=false\n"
                                "X-GNOME-Autostart-enabled=true\n"
                                "Name=Apply HiDPI Config for " + dispName +
                                "\nComment=Created by HiDPI-Fixer");

        // Write launcher data to file
        file.write (data.toUtf8());
        file.close();

        // Notify user
        QMessageBox::information (this,
                                  tr ("Info"),
                                  tr ("Changes applied, its recommended to "
                                      "logout and login again to test that "
                                      "the script works as intended."));
    }
}

/**
 * Saves the current script to the test script location and
 * executes the script.
 */
void MainWindow::testScript() {
    saveAndExecuteScript (SCRIPTS_HOME + "/test");
}

/**
 * Opens the GitHub issues page
 */
void MainWindow::reportBugs() {
    QDesktopServices::openUrl (QUrl ("https://github.com/alex-spataru"
                                     "/HiDPI-Fixer/issues"));
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
 * user changes the display
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
    int factor = static_cast<int>(ceil (scale));

    // Calculate the screen multiplying factor
    qreal multFactor = floor ((factor / scale) * 1000) / 1000.0;

    // Scale factor is 1...we don't need a script!
    if (factor == 1) {
        ui->ScriptPreview->setPlainText ("");
        return;
    }

    // Check if current selected resolution is valid
    QStringList size = ui->ResolutionsComboBox->currentText().split ("x");
    if (size.count() != 2) {
        qWarning() << "Invalid resolution"
                   << ui->ResolutionsComboBox->currentText();
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Invalid resolution \"%1\"!")
                              .arg (ui->ResolutionsComboBox->currentText()));
        return;
    }

    // Get resolution width and height
    QSize res;
    res.setWidth(size.at(0).toInt());
    res.setHeight(size.at(1).toInt());

    // Get target resolution
    int width = static_cast<int>(ceil(res.width() * multFactor));
    int height = static_cast<int>(ceil(res.height() * multFactor));

    // Get modeline, resolution name and display name
    QString modeline = CvtGetModeline (width, height);
    QString resName = CvtGetResolutionName (modeline);
    QString dispName = ui->DisplaysCombo->currentText();

    // Validate modeline and resName
    if (modeline.isEmpty()) {
        ui->ScriptPreview->clear();
        ui->ScriptPreview->setPlainText("# Error :(\n");
        return;
    }

    // Create script
    QString script;
    script.append("#!/bin/bash\n\n");
    script.append("# THIS SCRIPT COMES WITH NO WARRANTIES, USE IT AT YOUR\n"
                  "# OWN RISK\n\n");

    // Create new resolution
    script.append("# Create new resolution\n");
    script.append(QString("xrandr --newmode %1\n\n").arg(modeline));

    // Register resolution with current display
    script.append(QString("# Register resolution with %1\n").arg(dispName));
    script.append(QString("xrandr --addmode %1 %2\n\n").arg(dispName).arg(resName));

    // Change resolution for current display
    script.append(QString("# Change resolution for %1\n").arg(dispName));
    script.append(QString("xrandr --output %1 --mode %2\n\n").arg(dispName).arg(resName));

    // Set scaling factor (GNOME)
    script.append("# Change scaling factor (GNOME)\n");
    script.append(QString("gsettings set org.gnome.desktop.interface scaling-factor %1\n").arg(factor));

    // Update controls
    ui->ScriptPreview->setPlainText(script);
}

/**
 * Saves the script to the given location (creating the directories if
 * necessary), makes the new file executable and tries to execute
 * the newly created script/program.
 */
int MainWindow::saveAndExecuteScript (const QString& location) {
    // Get script text
    QString scriptData = ui->ScriptPreview->document()->toPlainText();

    // Script is empty
    if (scriptData.isEmpty()) {
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
        file.write (scriptData.toUtf8());
        file.close();
    }

    // Cannot open file for writing, warn user
    else {
        qWarning() << Q_FUNC_INFO
                   << "Cannot open" << file.fileName() << "for writing!";
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot open %1 for writing!")
                              .arg (file.fileName()));
        return 1;
    }

    // Make script executable
    QStringList arguments = {"+x", file.fileName()};
    if (QProcess::execute ("chmod", arguments) != 0) {
        qWarning() << Q_FUNC_INFO
                   << "Cannot execute chmod" << arguments;
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot make file \"%1\" executable!")
                              .arg (file.fileName()));
        return 1;
    }

    // Run file
    if (QProcess::execute (file.fileName()) != 0) {
        qWarning() << Q_FUNC_INFO
                   << "Cannot execute" << file.fileName();
        QMessageBox::warning (this,
                              tr ("Error"),
                              tr ("Cannot run script at %1")
                              .arg (file.fileName()));
        return 1;
    }

    // Notify caller that everything is OK
    return 0;
}

/**
 * Updates the resolutions ComboBox when the user selects
 * another display
 */
void MainWindow::updateResolutionCombo (const int index)
{
    ui->ResolutionsComboBox->clear();
    ui->ResolutionsComboBox->addItems (XrandrGetAvailableResolutions (index));
}
