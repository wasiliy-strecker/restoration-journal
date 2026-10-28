#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QUuid>
#include <QVector>
#include <stdexcept>

namespace restoration {

enum class ProjectStatus : quint8 { Planned, Started, Finished };
QString storedStatus(ProjectStatus status);
QString displayStatus(ProjectStatus status);
ProjectStatus statusFromStorage(const QString& value);

struct ChecklistStep {
  QUuid id;
  QString text;
  bool completed = false;
  qint64 position = 0;
  bool operator==(const ChecklistStep&) const = default;
};
using Checklist = QVector<ChecklistStep>;
Checklist validateChecklist(Checklist steps);

struct ProjectDraft {
  QString name;
  QString material;
  QString location;
  QString note;
  ProjectStatus status = ProjectStatus::Planned;
  QByteArray photo = {};
  Checklist steps = {};
  bool operator==(const ProjectDraft&) const = default;
};

struct RestorationProject {
  qint64 id = 0;
  ProjectDraft fields;
  bool hasPhoto = false;
  bool operator==(const RestorationProject&) const = default;
};

// List summaries never contain photo bytes. Full drafts are loaded by ID.
RestorationProject summarize(qint64 id, ProjectDraft draft);

using ProjectList = QVector<RestorationProject>;

class ValidationError final : public std::runtime_error {
public:
  explicit ValidationError(const QString& message) : std::runtime_error(message.toStdString()) {}
};

ProjectDraft validateDraft(ProjectDraft draft);
QString searchKey(const QString& text);
ProjectList searchProjects(const ProjectList& projects, const QString& query);

} // namespace restoration

Q_DECLARE_METATYPE(restoration::ProjectDraft)
Q_DECLARE_METATYPE(restoration::RestorationProject)
Q_DECLARE_METATYPE(restoration::ProjectList)

Q_DECLARE_METATYPE(restoration::ProjectStatus)
