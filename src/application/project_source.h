#pragma once

#include "domain/project.h"
#include <functional>

namespace restoration {
using ProjectReceiver = std::function<void(const ProjectDraft&)>;
// Replayable saved projects. Both layout passes must see the same contents and order.
using ProjectSource = std::function<void(const ProjectReceiver&)>;
using ProjectSnapshotConsumer = std::function<void(const ProjectSource&)>;
} // namespace restoration
