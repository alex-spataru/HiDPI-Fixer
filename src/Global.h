#ifndef GLOBAL_H
#define GLOBAL_H

#include <QDir>
#include <QFile>
#include <QString>

/**
 * Defines the folder in which the HiDPI-Fixer scripts are stored
 */
static const QString HiDPI_FixerHome = QString ("%1/.hidpi-fixer").arg (QDir::homePath());

/**
 * Defines the file location and name pattern for startup scripts
 */
static const QString HiDPI_AutostartBase = "HiDPI-Fixer_";
static const QString HiDPI_AutostartDir = QString ("%1/.config/autostart").arg (QDir::homePath());

#endif // GLOBAL_H
