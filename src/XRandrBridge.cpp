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
 * Returns a list with the available resolutions reported
 * by the xrandr-process for the given display
 */
QStringList XrandrDisplayResolutions (const int display) {
    Q_ASSERT (display >= 0);

    // Try to run xrandr --screen $display
    QProcess process;
    QStringList arguments = {"--screen", QString::number (display)};
    process.start ("xrandr", arguments);
    process.waitForFinished (1000);

    // If process fails, abort
    if (process.exitCode() != 0) {
        QMessageBox::warning (
                    Q_NULLPTR,
                    QObject::tr ("Error"),
                    QObject::tr ("Cannot run xrandr --screen %1").arg (display));
        qWarning() << Q_FUNC_INFO
                   << "xrandr returned exit code"
                   << process.exitCode();
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
