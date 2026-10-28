#include "application/project_service.h"

namespace restoration {

ProjectList ProjectService::load() {
  repository_.initialize();
  return repository_.findAll();
}

RestorationProject ProjectService::save(qint64 id, const ProjectDraft& draft) {
  const auto validated = validateDraft(draft);
  return id == 0 ? repository_.create(validated) : repository_.update(id, validated);
}

void ProjectService::remove(qint64 id) { repository_.remove(id); }

} // namespace restoration
