#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
qt_root="${USAGE_FLOAT_QT_ROOT:-$HOME/.local/share/usage-float-sdk/6.8.3/gcc_64}"
prefix_args=()
if [[ -d "$qt_root" ]]; then
    prefix_path="$qt_root"
    local_headers="$HOME/.local/share/usage-float-sdk/sysroot/usr"
    if [[ -d "$local_headers" ]]; then prefix_path="$prefix_path;$local_headers"; fi
    prefix_args+=("-DCMAKE_PREFIX_PATH=$prefix_path")
fi
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Release "${prefix_args[@]}"
cmake --build "$project_dir/build" --parallel "${USAGE_FLOAT_BUILD_JOBS:-4}"
ctest --test-dir "$project_dir/build" --output-on-failure
