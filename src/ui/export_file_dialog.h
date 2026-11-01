#pragma once

#include <QString>

class QWidget;
class QDir;

namespace restoration {
QString exportFilename(const QString& entered, bool pdf = false);
QString chooseExportFile(QWidget* parent, const QDir& directory, const QString& suggestedName,
                         bool selectDirectory, bool pdf = false);
} // namespace restoration
