#pragma once

#include "application/project_service.h"
#include "pdf/pdf_export.h"

#include <QImage>
#include <QObject>
#include <QThread>
#include <functional>
#include <memory>

namespace restoration {

enum class Operation : quint8 { Load, Save, Remove, Import, Export, Language };
using RepositoryFactory = std::function<std::unique_ptr<ProjectRepository>()>;

class StorageWorker final : public QObject {
  Q_OBJECT
public:
  explicit StorageWorker(RepositoryFactory factory);

public slots:
  void load();
  void save(qint64 id, const restoration::ProjectDraft& draft);
  void remove(qint64 id);
  void saveLanguage(const QString& directory, const QString& preference);
  void read(qint64 id, quint64 token);
  void importPhotoFile(const QString& path);
  void inspectExport(qint64 id, const QString& path, const QString& dataDirectory);
  void writeExport(qint64 id, const restoration::ExportTarget& target, bool approved,
                   const QString& dataDirectory);

signals:
  void exportTargetReady(qint64 id, const restoration::ExportTarget& target);
  void exported(const QString& path);
  void readReady(quint64 token, qint64 id, const restoration::ProjectDraft& draft,
                 const QImage& preview);
  void readFailed(quint64 token, const QString& message);
  void imported(const QByteArray& bytes, const QImage& preview);
  void loaded(const restoration::ProjectList& projects);
  void saved(const restoration::RestorationProject& project);
  void removed(qint64 id);
  void languageSaved(const QString& preference);
  void failed(restoration::Operation operation, const QString& message);

private:
  RepositoryFactory factory_;
  std::unique_ptr<ProjectRepository> repository_;
};

class StorageController final : public QObject {
  Q_OBJECT
public:
  explicit StorageController(RepositoryFactory factory, QObject* parent = nullptr);
  ~StorageController() override;
  bool isBusy() const { return busy_; }
  bool load();
  bool save(qint64 id, const ProjectDraft& draft);
  bool remove(qint64 id);
  bool saveLanguage(const QString& directory, const QString& preference);
  void read(qint64 id, quint64 token);
  bool importPhotoFile(const QString& path);
  bool prepareExport(qint64 id, const QString& path, const QString& dataDirectory);
  void writeExport(qint64 id, const ExportTarget& target, bool approved,
                   const QString& dataDirectory);
  void cancelExport();

signals:
  void exportTargetReady(qint64 id, const restoration::ExportTarget& target);
  void exported(const QString& path);
  void readReady(quint64 token, qint64 id, const restoration::ProjectDraft& draft,
                 const QImage& preview);
  void readFailed(quint64 token, const QString& message);
  void imported(const QByteArray& bytes, const QImage& preview);
  void loaded(const restoration::ProjectList& projects);
  void saved(const restoration::RestorationProject& project);
  void removed(qint64 id);
  void languageSaved(const QString& preference);
  void failed(restoration::Operation operation, const QString& message);
  void busyChanged(bool busy);
  void readRequested(qint64 id, quint64 token);
  void importRequested(const QString& path);
  void inspectExportRequested(qint64 id, const QString& path, const QString& dataDirectory);
  void exportRequested(qint64 id, const restoration::ExportTarget& target, bool approved,
                       const QString& dataDirectory);
  void loadRequested();
  void saveRequested(qint64 id, const restoration::ProjectDraft& draft);
  void removeRequested(qint64 id);
  void languageRequested(const QString& directory, const QString& preference);

private:
  bool begin();
  void finish();
  QThread thread_;
  bool busy_ = false;
  bool exportPending_ = false;
};

} // namespace restoration
Q_DECLARE_METATYPE(restoration::Operation)
