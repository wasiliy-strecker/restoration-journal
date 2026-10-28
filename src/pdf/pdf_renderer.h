#pragma once

#include "domain/project.h"
#include <QIODevice>

namespace restoration {
class ExportError final : public std::runtime_error {
public:
  explicit ExportError(const QString& message) : std::runtime_error(message.toStdString()) {}
};
// Register bundled resources before starting workers, on the GUI thread.
void initializePdfFonts();
// No widgets, SQL, source paths or external resource loading. Device stays owned by caller.
void renderProjectPdf(const ProjectDraft& project, QIODevice& output);
} // namespace restoration
