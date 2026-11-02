# Usage Float

A small desktop allowance monitor for **Claude Code** and **Codex**, built with
C++17 and Qt 6 Quick/QML. No browser engine, web server, API key, or separate
widget sign-in. Native Wayland is supported; your compositor controls window
placement and Always on Top.

**[Download releases](https://github.com/snick-m/usage-float/releases)** ·
[Build status](https://github.com/snick-m/usage-float/actions/workflows/build.yml)

## Download and install

Release packages include Qt; you do not need the development SDK to run them.

- **Linux x86_64:** extract the `.tar.gz`, run `./UsageFloat`, or run `./install.sh`
  to install it under your home directory and add the Applications icon. Packages
  target Ubuntu 24.04 or a compatible glibc desktop, with native Wayland plugins.
- **Windows x86_64:** extract the full `.zip`, then run `bin/usage-float.exe`.
- **macOS:** download the Apple Silicon (`arm64`) or Intel (`x86_64`) `.dmg` and
  drag **Usage Float.app** into Applications.

These builds are unsigned/not notarized. SHA-256 checksums accompany the downloads.
You still need your existing Claude Code and/or Codex CLI login.

## What it does

- Session and weekly usage, countdowns, and additional model-specific limits.
- Configured default model underneath each provider name (`Auto` when unset).
- Compact two-provider view, expanded detail cards, and borderless capsule by default.
- Local allowance updates plus 15-minute provider polling, independent asynchronous
  refreshes, timeouts, Claude cooldown/backoff, and
  clearly marked stale readings when a provider fails.
- Tray menu (when your desktop provides a tray), single-instance activation,
  configurable polling/background opacity, and optional startup at login.
- Local CLI logins only; credentials are never saved by the widget or logged.

## Build

Requires CMake 3.21+, a C++17 compiler, and **Qt 6.8+** with Core, Gui, Widgets,
Network, Quick, QuickControls2, Svg, and Test. Qt 6.8.3 is used on the development laptop.
Linux builds also require Qt DBus and WaylandClient.
Use a Qt release that supports your target OS.

On a distribution with Qt 6.8+ packages, the development dependencies are typically:

```sh
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev qt6-wayland qt6-svg-dev \
    libgl-dev libopengl-dev libwayland-dev libxkbcommon-dev \
    qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    qml6-module-qtquick-window qml6-module-qtqml-workerscript
```

Some distributions ship an older Qt; use a recent SDK from the Qt installer instead.
For a custom SDK:

```sh
export USAGE_FLOAT_QT_ROOT=/path/to/Qt/6.8.3/gcc_64
./scripts/build.sh
./scripts/run.sh
```

Or use CMake directly on Linux, macOS, or Windows:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Qt's deployment script is included in the CMake install step. macOS/Windows
source paths are implemented, but builds and desktop integration have only been
verified locally on Ubuntu/Wayland. GitHub Actions builds, tests, deploys Qt, and
smoke-tests the installed app on Linux, Windows, Apple Silicon macOS, and Intel
macOS. See the linked workflow for the current result. Signing and notarization
are not included in this initial version.

### This laptop

The Qt SDK is installed under `~/.local/share/usage-float-sdk/6.8.3/gcc_64`.
Missing OpenGL development headers were extracted from Ubuntu packages into a
user-local `sysroot` next to it; no system packages were changed. `build.sh`
detects both paths automatically. This SDK is external to the project repository.

## Run and install

```sh
./scripts/run.sh                          # actual account allowances
./scripts/run.sh --demo                   # sample data, isolated preferences
./scripts/run.sh --demo --compact         # compact sample view
./scripts/run.sh --check                  # one read, sanitized JSON, then exit
./scripts/install-local.sh               # install app and menu launcher on Linux
```

The app appears as **Usage Float** in your app menu. Launching it again brings
the existing window forward. Closing hides to the tray when a tray is available;
otherwise it exits. Use Settings or Ctrl+Q to quit explicitly. Startup at login is
opt-in in Settings; enable it after installing so it points to the installed app.

| Shortcut | Action |
|---|---|
| Double-click capsule | Expand details |
| Drag capsule | Move the window |
| Right-click / ⋯ | Capsule menu |
| Ctrl+R | Refresh usage |
| Ctrl+M | Compact / expanded view |
| Ctrl+W | Toggle wide capsule with usage bars |
| Ctrl+, | Settings |
| Ctrl+Q | Quit |

### Wayland

The launcher uses native Wayland when available. **Right-click the system title
bar and select Always on Top** (wording depends on the desktop). The app starts as a thin, borderless capsule. To access that menu, temporarily
enable **System title bar** from the capsule’s ⋯ menu or Settings.
Whether that state survives decoration changes or restarts depends on the
compositor. Qt cannot universally enforce it on native Wayland. The internal
drag handle uses an interactive system move, rather than setting coordinates.
Position restoration is performed only on other window systems.

The capsule’s **⋯ → Wide usage bars** switch adds session and weekly progress bars
for each provider while keeping the same thin, borderless capsule. The preference
is remembered; Ctrl+W toggles it too. `--wide` starts directly in this view.

Use **⋯ → Show remaining instead of used** (also in Settings and the tray menu)
to switch all percentages and bar lengths between used and remaining allowance.
The preference is remembered. Warning colors always reflect usage nearing the
limit, so a low remaining balance can still show a red bar.

## Optional GNOME panel indicator

On GNOME 45–50 (including Ubuntu), you can put a live usage bar graph in the
panel. Four vertical bars show Claude session/weekly on the left (amber) and
Codex session/weekly on the right (mint). Bars fill from bottom to top and
turn amber/red as used allowance approaches the limit. Install the app normally, then run:

```sh
./scripts/install-panel.sh --enable
# Log out and back in if prompted, so GNOME discovers the new extension.
```

For a downloaded Linux package, the installer is
`share/usage-float/install-panel.sh` inside the extracted package.
Launch Usage Float, then select **Hide floating widget** in the panel menu
to use just the panel. The menu also provides all quota windows and reset
countdowns, refresh, a used/remaining switch, and **Open Usage Float**.
Exact percentages and reset times are in the popup. The used/remaining switch
also changes the bar heights, while warning colors always reflect used allowance.
Enable **Start at login** in the app if desired. Stale or failed readings are
dimmed with warning dots; missing data shows crossed tracks, distinct from zero.
If the app stops, the
indicator shows offline and its Open action starts the installed app.

The extension shares the app's existing readings over the local session D-Bus;
it never reads credentials, polls providers, or writes usage to disk. Demo mode
uses a separate service and does not replace the live panel readings.
Disable it with `gnome-extensions disable usage-float@usagefloat.io`; disabling
also brings the floating widget back. This is a GNOME top-panel extension;
KDE panels and the Ubuntu application dock are separate desktop integrations.

**Dash to Panel:** when it replaces the stock top panel, this live bar graph
appear directly in its taskbar using native GNOME Shell widgets. No tray icon
or Dash to Panel patch is required. The app automatically hides its duplicate
tray icon while the native panel widget is connected and restores it if that
connection disappears. Closing the floating window keeps monitoring in the panel.

## How accounts work

### Hybrid refresh

Usage Float checks local allowance data every five seconds and polls both
providers every **15 minutes**, including when local data is available. Existing
installations switch to 15 minutes once on upgrade; you can change the interval
in Settings afterward. Startup and manual refresh also poll, subject to Claude's
cooldown. Local reads never trigger a provider request.

- **Codex:** automatically reads allowance snapshots from recent
  `$CODEX_HOME/sessions/**/*.jsonl` files (default `~/.codex/sessions`). These are
  server-reported percentages and reset times, not estimates from token counts.
  New session files are discovered within a minute; existing files are read
  incrementally with bounded reads. No setup is required.
- **Claude Code:** enable the local status-line bridge with Python 3:

  ```sh
  python3 scripts/claude-statusline.py --install
  ```

  For an installed package, the script is `share/usage-float/claude-statusline.py`.
  Restart Claude Code sessions after installing. The bridge preserves an existing
  status-line command and its display settings, backs up settings before changing
  them, and exports only allowance data to
  `$CLAUDE_CONFIG_DIR/usage-float.json` (default `~/.claude/usage-float.json`). It
  makes no network calls. Claude Code must supply the `rate_limits` status-line
  fields; otherwise polling continues to provide readings. Status-line redraws
  with no new response do not refresh the observation timestamp.

Only local observations from the last 15 minutes are accepted. Newer observations
replace matching allowance windows; older local records cannot overwrite newer
poll results. Missing and expired local windows never become zero usage. Other
provider windows from the last poll remain visible. Local snapshots may lag
activity on other devices until the CLI receives another response; scheduled
polling fills that gap. Local log formats can change, so polling remains the
fallback when no usable local data is present.

To undo the Claude bridge, restore the `statusLine` entry from
`usage-float-statusline-previous.json` in the Claude config folder (or remove the
entry if that file is `{}`). The bridge keeps a small response marker per session
under `usage-float-statusline-state/` to distinguish redraws across active
sessions. Those files and `usage-float.json` can be deleted after disabling it.

### Provider polling and accounts

**Claude Code:** reads `~/.claude/.credentials.json` on each refresh, or
`$CLAUDE_CONFIG_DIR/.credentials.json` when configured. On macOS, it also tries
Claude Code's `Claude Code-credentials` Keychain entry. The access token is sent
only to `https://api.anthropic.com/api/oauth/usage`, with redirects disabled.
The widget never refreshes or rewrites Claude tokens: Claude Code owns that
lifecycle. If authentication expires, open Claude Code or run `claude auth login`.
The usage endpoint is undocumented and may change or reject third-party reads.

**Codex:** starts the locally installed `codex app-server`, initializes its
newline-delimited JSON protocol, and calls `account/rateLimits/read`. Codex owns
its login and token refresh. The widget does not read Codex credentials. No model
prompts are sent. If needed, run `codex login`. Set an executable path in Settings
if a desktop launch cannot find the CLI. Windows npm `.cmd` shims are supported.
Windows Claude installations inside WSL require running the widget in that Linux
environment; the Windows build does not automatically import WSL credentials.

Settings are stored via QSettings in your OS's standard per-user location
(`~/.config/UsageFloat/UsageFloat.conf` on Linux). No usage history is uploaded. The optional Claude bridge persists its latest
allowance snapshot and response markers locally; session logs belong to Codex.
Background provider polling defaults to every 15 minutes. A failed
request keeps the last reading and marks it stale; missing limits never become
zero usage. Claude rate-limit responses respect cooldowns, including manual
refresh clicks. Reading percentages does not guarantee the provider will accept
a new task, and reset times remain estimates supplied by that provider.

## Tests and visual QA

Tests cover missing/null allowances versus genuine zero usage, Codex bucket
deduplication and labels, scoped Claude limits, clamping, reset countdowns,
independent provider failures, stale-data retention, subprocess startup errors,
and suppression of raw RPC error messages. Hybrid tests cover partial local log
records, multiple Codex buckets, missing/expired windows, timestamp ordering,
interval migration, and Claude bridge installation/redraw behavior. Lifecycle tests use a fake app-server
and an empty credential directory; they never touch your actual accounts.
Window tests repeatedly expand/collapse both capsule widths and check that details
fit the content when screen space permits, while capsules return to 44px tall.
CTest runs these offscreen; `QT_QPA_PLATFORM=wayland ./build/window-tests` also
checks the same transitions against a real Wayland compositor.
Demo mode never accesses account credentials and uses a separate settings name.
To capture a deterministic UI sample:

```sh
./scripts/run.sh --demo --screenshot /tmp/usage-float.png
```

The initial design deliberately uses a static background and animates meter
changes only; it does not keep a continuous animation running while idle.

## Releases

Pushes and pull requests build all four desktop packages and upload archives as
workflow artifacts. A matching `vX.Y.Z` tag additionally publishes a GitHub Release
with `.tar.gz`, `.zip`, `.dmg`, and SHA-256 files after every matrix job passes.
The workflow rejects tags that do not match the project version plus the optional
`USAGE_FLOAT_PRERELEASE` suffix in `CMakeLists.txt`. Tags such as
`v0.2.0-rc.2` publish a GitHub prerelease and do not replace the latest stable release.

To cut a release, update the project version, `USAGE_FLOAT_PRERELEASE` (empty for
stable releases), and `packaging/RELEASE_NOTES.md`,
commit, push, and verify the branch workflow. Then tag that exact commit:

```sh
git tag -a v0.2.0-rc.2 -m "Usage Float v0.2.0-rc.2"
git push origin v0.2.0-rc.2
```

`scripts/package.py` creates the same download archives locally from a CMake
install tree. `scripts/smoke-installed.py` verifies startup with Qt SDK paths
removed from the runtime environment. Platform icons are generated from
`packaging/io.usagefloat.UsageFloat.svg`; PNG, ICO, and ICNS assets are checked in
so builders do not need image conversion tools.
