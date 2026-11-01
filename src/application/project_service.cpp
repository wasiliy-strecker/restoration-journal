#include "application/project_service.h"

namespace restoration {

void ProjectRepository::withSavedProjects(const ProjectSnapshotConsumer& consume) {
  // Immutable fallback for replaceable repositories. SQLite avoids retaining all photos.
  const auto snapshot = transferSnapshot();
  consume([&snapshot](const ProjectReceiver& receive) {
    for (const auto& project : snapshot)
      receive(project);
  });
}

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
