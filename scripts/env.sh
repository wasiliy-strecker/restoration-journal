#!/usr/bin/env bash
# Optional machine-local SDK paths. No dependency on another application.
restoration_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -f "$restoration_root/.tools/env.sh" ]]; then
  source "$restoration_root/.tools/env.sh"
fi
if [[ -n "${RESTAURIERUNGSHEFT_QT_ROOT:-}" ]]; then
  export CMAKE_PREFIX_PATH="$RESTAURIERUNGSHEFT_QT_ROOT${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
  export PATH="$RESTAURIERUNGSHEFT_QT_ROOT/bin:$PATH"
fi
