Usage Float v0.2.0-rc.2 adds hybrid local usage updates and less frequent provider polling.

### New in this release candidate

- Codex allowance percentages and reset times update automatically from local session logs, without extra provider requests.
- Claude Code can export allowance snapshots locally through the optional status-line bridge. With Python 3 installed, run `python3 share/usage-float/claude-statusline.py --install` from an extracted package, then restart Claude Code terminal sessions. Existing status-line commands and display settings are preserved.
- Local data is checked every five seconds; newly created Codex session files are discovered within a minute.
- Background provider polling defaults to 15 minutes. Existing installations switch once on upgrade; the interval remains configurable in Settings.
- Newer local allowance windows replace older readings. Older local records, missing percentages, and expired windows cannot overwrite fresh data or appear as zero usage.
- Claude cooldowns remain respected by both automatic and manual refreshes. Polling continues when local data is unavailable.
- Tests cover incremental and partial Codex logs, multiple allowance buckets, timestamp ordering, interval migration, and Claude bridge setup/redraw behavior.

The Claude bridge requires Claude Code to provide rate-limit fields to its status line; headless integrations that do not run a status line continue to use polling. Local readings can lag usage on other devices until another CLI response arrives. Both local log formats and the Claude usage endpoint may change.

The native GNOME/Dash to Panel graph from RC 1 remains included. The extension targets GNOME 45–50; Windows and macOS retain the floating widget and tray/menu-bar behavior.

### Included features

- Session and weekly usage, reset countdowns, and independent provider status.
- Narrow capsule, wide usage bars, and expandable details.
- Used/remaining switch, dark tooltips, and native Wayland support.
- Optional native GNOME/Dash to Panel usage bar graph, reset details, and widget controls.
- Reuses existing CLI logins; no API key or separate widget sign-in.

### Downloads

- **Linux x86_64:** extract the `.tar.gz`, run `./UsageFloat`, or run `./install.sh` to add an Applications launcher. Built on Ubuntu 24.04; requires a compatible glibc desktop and graphics libraries. Qt and native Wayland plugins are bundled.
- **Windows x86_64:** extract the entire `.zip` and run `bin/usage-float.exe`. Keep the other files beside it.
- **macOS:** choose the Apple Silicon (`arm64`) or Intel (`x86_64`) `.dmg`, open it, and drag **Usage Float.app** into Applications.

Builds are not commercially code-signed or Apple-notarized. Platform launch warnings may require explicitly allowing the app.

Install and sign in to Claude Code and/or Codex before launching. On Wayland, your desktop controls Always on Top; enable the optional system title bar to access its window menu.

Every package has a SHA-256 checksum, also collected in `SHA256SUMS.txt`.
