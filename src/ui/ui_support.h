#pragma once

#include <QWidget>

namespace restoration {
bool confirmDiscard(QWidget* parent);
bool confirmDelete(QWidget* parent, const QString& name);
} // namespace restoration
