#pragma once

#include "application/project_repository.h"

namespace restoration {

class ProjectService final {
public:
  explicit ProjectService(ProjectRepository& repository) : repository_(repository) {}
  ProjectList load();
  RestorationProject save(qint64 id, const ProjectDraft& draft);
  void remove(qint64 id);

private:
  ProjectRepository& repository_;
};

} // namespace restoration
