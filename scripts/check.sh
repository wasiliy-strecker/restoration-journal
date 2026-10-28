#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/env.sh
restoration_preset="${1:-dev}"
mapfile -d '' restoration_files < <(find src tests -type f \( -name '*.h' -o -name '*.cpp' \) -print0)
clang-format --dry-run --Werror "${restoration_files[@]}"
cmake --preset "$restoration_preset"
cmake --build --preset "$restoration_preset" --parallel 4
ctest --preset "$restoration_preset"
if [[ "$restoration_preset" != asan ]]; then
  mapfile -d '' restoration_sources < <(find src -name '*.cpp' -print0)
  clang-tidy -p "build/$restoration_preset" "${restoration_sources[@]}" --quiet
fi
git diff --check
