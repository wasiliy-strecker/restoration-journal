#pragma once

#include <QString>

namespace restoration {
QString defaultDataDirectory();
QString resolveDataDirectory(const QString& overridePath, bool explicitlyProvided);
} // namespace restoration
