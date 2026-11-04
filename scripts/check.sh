#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/env.sh
restoration_preset="${1:-dev}"
restoration_files=()
while IFS= read -r -d '' restoration_file; do
  restoration_files+=("$restoration_file")
done < <(find src tests -type f \( -name '*.h' -o -name '*.cpp' \) -print0)
clang-format --dry-run --Werror "${restoration_files[@]}"
cmake --preset "$restoration_preset"
cmake --build --preset "$restoration_preset" --parallel 4
ctest --preset "$restoration_preset"
if [[ "$restoration_preset" != asan ]]; then
  if command -v clang-tidy >/dev/null 2>&1; then
    restoration_sources=()
    while IFS= read -r -d '' restoration_source; do
      restoration_sources+=("$restoration_source")
    done < <(find src -name '*.cpp' -print0)
    clang-tidy -p "build/$restoration_preset" "${restoration_sources[@]}" --quiet
  elif [[ "$(uname -s)" == Darwin ]]; then
    python3 scripts/analyze_macos.py "build/$restoration_preset/compile_commands.json"
  else
    echo "clang-tidy is required for static analysis" >&2
    exit 1
  fi
fi
git diff --check
