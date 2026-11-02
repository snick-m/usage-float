# Usage Float

A small desktop allowance monitor for **Claude Code** and **Codex**, built with
C++17 and Qt 6 Quick/QML. No browser engine, web server, API key, or separate
widget sign-in. Native Wayland is supported; your compositor controls window
placement and Always on Top.

## What it does

- Session and weekly usage, countdowns, and additional model-specific limits.
- Compact two-provider view, expanded detail cards, and borderless capsule by default.
- Independent asynchronous refreshes, timeouts, Claude cooldown/backoff, and
  clearly marked stale readings when a provider fails.
- Tray menu (when your desktop provides a tray), single-instance activation,
  configurable polling/background opacity, and optional startup at login.
- Local CLI logins only; credentials are never saved by the widget or logged.

## Build

Requires CMake 3.21+, a C++17 compiler, and **Qt 6.8+** with Core, Gui, Widgets,
Network, Quick, QuickControls2, Svg, and Test. Qt 6.8.3 is used on the development laptop.
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
verified on Ubuntu/Wayland so far. A GitHub Actions build/test/package matrix is
included for all three desktop platforms, but has not been run remotely yet.
Signing and notarization are not included in this initial version.

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

## How accounts work

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
(`~/.config/UsageFloat/UsageFloat.conf` on Linux). No usage history is uploaded
or persisted. Background refresh defaults to every three minutes. A failed
request keeps the last reading and marks it stale; missing limits never become
zero usage. Claude rate-limit responses respect cooldowns, including manual
refresh clicks. Reading percentages does not guarantee the provider will accept
a new task, and reset times remain estimates supplied by that provider.

## Tests and visual QA

Tests cover missing/null allowances versus genuine zero usage, Codex bucket
deduplication and labels, scoped Claude limits, clamping, reset countdowns,
independent provider failures, stale-data retention, subprocess startup errors,
and suppression of raw RPC error messages. Lifecycle tests use a fake app-server
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
