#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/env.sh
cmake --preset dev
cmake --build --preset dev --parallel 4
if [[ "$(uname -s)" == Darwin ]]; then
  exec build/dev/restaurierungsheft.app/Contents/MacOS/restaurierungsheft "$@"
fi
exec build/dev/restaurierungsheft "$@"
