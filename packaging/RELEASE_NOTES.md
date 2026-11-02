Usage Float v0.2.0-rc.1 is a release candidate for the native GNOME/Dash to Panel usage graph.

### New in this release candidate

- Four native taskbar bars show Claude and Codex session/weekly allowances.
- Used/remaining display, warning colors, missing-data markers, and stale-reading indicators.
- Exact percentages and reset countdowns in the panel popup, plus refresh and widget controls.
- Optional panel installer: run `share/usage-float/install-panel.sh --enable` from the Linux package. Log out and back in when prompted, then launch Usage Float.
- The taskbar widget replaces the duplicate tray icon while connected and shares the app's readings without extra account requests.

The panel extension targets GNOME 45–50. Graph rendering and D-Bus lifecycle are tested locally; this RC is intended for desktop integration feedback across supported GNOME versions. Windows and macOS retain the floating widget and tray/menu-bar behavior.

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
