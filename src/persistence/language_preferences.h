#pragma once

#include <QDir>

namespace restoration {
struct LanguagePreference {
  QString value = QStringLiteral("auto");
  bool readFailed = false;
};
LanguagePreference readLanguagePreference(const QString& directory);
void saveLanguagePreference(const QDir& directory, const QString& preference);
} // namespace restoration
