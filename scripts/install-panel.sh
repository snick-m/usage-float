#!/usr/bin/env bash
set -euo pipefail
enable=false
if [[ "${1:-}" == '--enable' ]]; then enable=true; shift; fi
if [[ $# -ne 0 ]]; then printf 'Usage: %s [--enable]\n' "$0" >&2; exit 2; fi
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
extension_id='usage-float@usagefloat.io'
# Works from both a source checkout and share/usage-float in an installed package.
source_dir="$script_dir/../extensions/$extension_id"
if [[ ! -d "$source_dir" ]]; then source_dir="$script_dir/extensions/$extension_id"; fi
destination="${XDG_DATA_HOME:-$HOME/.local/share}/gnome-shell/extensions/$extension_id"
if [[ ! -f "$source_dir/metadata.json" ]]; then
    printf 'Cannot find the bundled GNOME panel extension.\n' >&2
    exit 1
fi
mkdir -p "$destination"
cp "$source_dir/extension.js" "$source_dir/format.js" "$source_dir/graph.js" "$source_dir/metadata.json" "$destination/"
printf 'Installed the optional Usage Float panel extension.\n'
if $enable; then
    if gnome-extensions enable "$extension_id" 2>/dev/null; then
        printf 'Enabled live usage in the GNOME panel (also works with Dash to Panel).\n'
        printf 'If updating an already loaded extension, log out and back in to load the new code.\n'
        exit 0
    fi
    # GNOME discovers new local extensions at login. Queue only this extension,
    # preserving all other enabled/disabled preferences.
    gjs -c '
        const Gio = imports.gi.Gio;
        const settings = new Gio.Settings({schema_id: "org.gnome.shell"});
        const uuid = "usage-float@usagefloat.io";
        const enabled = settings.get_strv("enabled-extensions");
        if (!enabled.includes(uuid)) settings.set_strv("enabled-extensions", [...enabled, uuid]);
        settings.set_strv("disabled-extensions", settings.get_strv("disabled-extensions").filter(id => id !== uuid));
        Gio.Settings.sync();
    '
    printf 'Enabled for your next session. Log out and back in to load the new native panel widget.\n'
    exit 0
fi
printf 'Log out and back in, then enable it with:\n  gnome-extensions enable %s\n' "$extension_id"
printf 'To remove it from the panel:\n  gnome-extensions disable %s\n' "$extension_id"
