#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$project_dir/scripts/build.sh"
install_prefix="${USAGE_FLOAT_INSTALL_PREFIX:-$HOME/.local/opt/usage-float}"
cmake --install "$project_dir/build" --prefix "$install_prefix"
mkdir -p "$HOME/.local/bin" "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor/scalable/apps"
ln -sfn "$install_prefix/bin/usage-float" "$HOME/.local/bin/usage-float"
python3 - "$project_dir" "$install_prefix" <<'PY'
import pathlib, shutil, sys
source, prefix = map(pathlib.Path, sys.argv[1:])
home = pathlib.Path.home()
executable = str(prefix / 'bin/usage-float')
for old, new in [('\\', '\\\\'), ('"', '\\"'), ('`', '\\`'), ('$', '\\$'), ('%', '%%')]:
    executable = executable.replace(old, new)
app_id = 'io.usagefloat.UsageFloat'
desktop = (source / f'packaging/{app_id}.desktop').read_text()
desktop = desktop.replace('Exec=usage-float', 'Exec="' + executable + '"')
icon = prefix / f'share/icons/hicolor/128x128/apps/{app_id}.png'
desktop = desktop.replace(f'Icon={app_id}', f'Icon={icon}')
(home / f'.local/share/applications/{app_id}.desktop').write_text(desktop)
shutil.copytree(prefix / 'share/icons/hicolor', home / '.local/share/icons/hicolor', dirs_exist_ok=True)
for old in ['.local/share/applications/io.usagefloat.desktop.desktop',
            '.local/share/icons/hicolor/scalable/apps/io.usagefloat.desktop.svg']:
    (home / old).unlink(missing_ok=True)
old_startup = home / '.config/autostart/io.usagefloat.desktop.desktop'
if old_startup.exists():
    (old_startup.parent / f'{app_id}.desktop').write_text(desktop)
    old_startup.unlink()
print('Installed Usage Float:', prefix)
PY
if command -v update-desktop-database >/dev/null; then update-desktop-database "$HOME/.local/share/applications"; fi
if command -v gtk-update-icon-cache >/dev/null; then gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor"; fi
