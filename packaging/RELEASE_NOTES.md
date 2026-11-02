Usage Float v0.2.0-rc.3 fixes refresh animations and tooltip flashing, and adds version details.

### New in this release candidate

- Usage bars keep their existing values during refreshes. Unchanged readings no longer flash an animation from zero; changed readings animate from the current value in both the wide capsule and expanded details.
- Provider hover areas survive usage updates, keeping open tooltips stable.
- Tooltip windows pass pointer input through, preventing flashing when a tooltip overlaps the cursor.
- The app version, including its release candidate suffix, appears in the tray menu and tooltip and the GNOME panel details.
- Dash to Panel is optional. The live panel graph requires the Usage Float GNOME extension and also works with GNOME's stock top panel; the regular tray icon is static.
- Regression checks cover bar identity across updates, unchanged-reading animations, tooltip input handling, and version details.

The hybrid local usage updates and 15-minute provider polling introduced in RC 2 remain included. The optional Claude status-line bridge requires Python 3 and Claude Code rate-limit fields. Local readings can lag usage on other devices until another CLI response arrives.

The GNOME extension targets GNOME 45–50. Updating an already loaded extension requires logging out and back in to load its new code. The app fixes take effect after restarting Usage Float.

### Included features

- Session and weekly usage, reset countdowns, and independent provider status.
- Narrow capsule, wide usage bars, and expandable details.
- Used/remaining switch, dark tooltips, and native Wayland support.
- Optional native GNOME/Dash to Panel usage bar graph, reset details, and widget controls.
- Reuses existing CLI logins; no API key or separate widget sign-in.

### Downloads

- **Linux x86_64:** extract the `.tar.gz`, run `./UsageFloat`, or run `./install.sh` to add an Applications launcher. Built on Ubuntu 24.04; requires a compatible glibc desktop and graphics libraries. Qt and native Wayland plugins are bundled.
- **Windows x86_64:** extract the entire `.zip` and run `bin/usage-float.exe`. Keep the other files beside it.
- **macOS:** download the Apple Silicon (`arm64`) `.dmg`, open it, and drag **Usage Float.app** into Applications.

Builds are not commercially code-signed or Apple-notarized. Platform launch warnings may require explicitly allowing the app.

Install and sign in to Claude Code and/or Codex before launching. On Wayland, your desktop controls Always on Top; enable the optional system title bar to access its window menu.

Every package has a SHA-256 checksum, also collected in `SHA256SUMS.txt`.
