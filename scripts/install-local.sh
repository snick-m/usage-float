#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$project_dir/scripts/build.sh"
install_prefix="${USAGE_FLOAT_INSTALL_PREFIX:-$HOME/.local/opt/usage-float}"
cmake --install "$project_dir/build" --prefix "$install_prefix"
mkdir -p "$HOME/.local/bin" "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor/scalable/apps"
ln -sfn "$install_prefix/bin/usage-float" "$HOME/.local/bin/usage-float"
python3 - "$project_dir" "$install_prefix" <<'PY'
import pathlib, sys
source, prefix = map(pathlib.Path, sys.argv[1:])
home = pathlib.Path.home()
executable = str(prefix / 'bin/usage-float')
for old, new in [('\\', '\\\\'), ('"', '\\"'), ('`', '\\`'), ('$', '\\$'), ('%', '%%')]:
    executable = executable.replace(old, new)
desktop = (source / 'packaging/io.usagefloat.desktop.desktop').read_text()
desktop = desktop.replace('Exec=usage-float', 'Exec="' + executable + '"')
(home / '.local/share/applications/io.usagefloat.desktop.desktop').write_text(desktop)
(home / '.local/share/icons/hicolor/scalable/apps/io.usagefloat.desktop.svg').write_bytes(
    (source / 'packaging/io.usagefloat.desktop.svg').read_bytes())
print('Installed Usage Float:', prefix)
PY
if command -v update-desktop-database >/dev/null; then update-desktop-database "$HOME/.local/share/applications"; fi
