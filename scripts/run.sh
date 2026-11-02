#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ ! -x "$project_dir/build/usage-float" ]]; then "$project_dir/scripts/build.sh"; fi
if [[ -n "${WAYLAND_DISPLAY:-}" && -z "${QT_QPA_PLATFORM:-}" ]]; then export QT_QPA_PLATFORM=wayland; fi
exec "$project_dir/build/usage-float" "$@"
