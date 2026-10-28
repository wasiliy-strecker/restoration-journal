#pragma once

#include "application/project_repository.h"
#include "pdf/pdf_renderer.h"
#include <QByteArray>
#include <QStringList>
#include <functional>

namespace restoration {
QString suggestedPdfName(const QString& name);
struct ExportTarget {
  QString path;
  QByteArray directoryIdentity;
  bool exists = false;
  QByteArray identity;
  QByteArray digest;
  QByteArray revision;
  bool operator==(const ExportTarget&) const = default;
};
// Called on worker before asking for replacement permission and again before publishing.
ExportTarget inspectExportTarget(const QString& path, const QString& dataDirectory);
using PdfRenderer = std::function<void(const ProjectDraft&, QIODevice&)>;
void exportProjectPdf(ProjectRepository& repository, qint64 id, const ExportTarget& target,
                      bool replaceApproved, const QString& dataDirectory,
                      const PdfRenderer& renderer = renderProjectPdf);
} // namespace restoration
Q_DECLARE_METATYPE(restoration::ExportTarget)
