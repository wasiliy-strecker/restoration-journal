#pragma once
#include "application/collection_file.h"
#include <QJsonObject>
namespace restoration {
QJsonObject encodeEntry(const ProjectDraft& draft);
ProjectDraft decodeEntry(const QJsonObject& object);
QByteArray entryFingerprint(const ProjectDraft& draft);
} // namespace restoration
