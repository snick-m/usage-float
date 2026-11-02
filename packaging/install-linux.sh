#!/bin/sh
set -eu
source_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
destination="$HOME/.local/opt/usage-float"
data_home="${XDG_DATA_HOME:-$HOME/.local/share}"
mkdir -p "$destination" "$HOME/.local/bin" "$data_home/applications" "$data_home/icons/hicolor/128x128/apps"
if [ "$source_dir" != "$destination" ]; then cp -R "$source_dir/." "$destination/"; fi
ln -sfn "$destination/UsageFloat" "$HOME/.local/bin/usage-float"
escaped=$(printf '%s' "$destination/UsageFloat" | sed 's/\\/\\\\/g;s/"/\\"/g;s/`/\\`/g;s/\$/\\$/g;s/%/%%/g')
cat > "$data_home/applications/io.usagefloat.UsageFloat.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Usage Float
Comment=Claude Code and Codex subscription allowances
Exec="$escaped"
Icon=$destination/share/icons/hicolor/128x128/apps/io.usagefloat.UsageFloat.png
Terminal=false
Categories=Utility;Monitor;
StartupWMClass=UsageFloat
EOF
cp "$destination/share/icons/hicolor/128x128/apps/io.usagefloat.UsageFloat.png" "$data_home/icons/hicolor/128x128/apps/"
if command -v update-desktop-database >/dev/null 2>&1; then update-desktop-database "$data_home/applications"; fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then gtk-update-icon-cache -f -t "$data_home/icons/hicolor"; fi
printf 'Installed Usage Float. Open it from Applications or run %s\n' "$HOME/.local/bin/usage-float"
