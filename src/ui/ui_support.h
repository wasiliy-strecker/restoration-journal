#pragma once

#include <QWidget>

namespace restoration {
bool confirmDiscard(QWidget* parent);
bool confirmDelete(QWidget* parent, const QString& name);
bool confirmImport(QWidget* parent, qsizetype count);
bool confirmExportReplacement(QWidget* parent, const QString& filename);

} // namespace restoration
