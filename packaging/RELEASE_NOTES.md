Usage Float is a thin desktop capsule for Claude Code and Codex subscription allowances.

- Session and weekly usage, reset countdowns, and independent provider status.
- Narrow capsule, wide usage bars, and expandable details.
- Used/remaining switch, dark tooltips, and native Wayland support.
- Reuses existing CLI logins; no API key or separate widget sign-in.

### Downloads

- **Linux x86_64:** extract the `.tar.gz`, run `./UsageFloat`, or run `./install.sh` to add an Applications launcher. Built on Ubuntu 24.04; requires a compatible glibc desktop and graphics libraries. Qt and native Wayland plugins are bundled.
- **Windows x86_64:** extract the entire `.zip` and run `bin/usage-float.exe`. Keep the other files beside it.
- **macOS:** choose the Apple Silicon (`arm64`) or Intel (`x86_64`) `.dmg`, open it, and drag **Usage Float.app** into Applications.

Builds are not commercially code-signed or Apple-notarized. Platform launch warnings may require explicitly allowing the app.

Install and sign in to Claude Code and/or Codex before launching. On Wayland, your desktop controls Always on Top; enable the optional system title bar to access its window menu.

Every package has a SHA-256 checksum, also collected in `SHA256SUMS.txt`.
