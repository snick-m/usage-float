#!/usr/bin/env python3
"""Verify the installed binary starts with SDK paths removed from its environment."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--prefix', type=Path, required=True)
parser.add_argument('--sdk', type=Path, required=True)
args = parser.parse_args()
prefix = args.prefix.resolve(strict=True)
sdk = str(args.sdk.resolve()).lower()
env = os.environ.copy()
env['PATH'] = os.pathsep.join(p for p in env.get('PATH', '').split(os.pathsep) if sdk not in p.lower())
env['QT_QPA_PLATFORM'] = 'offscreen'
env['QT_QUICK_BACKEND'] = 'software'
env['QT_FORCE_STDERR_LOGGING'] = '1'
env['QT_LOGGING_TO_CONSOLE'] = '1'
if os.name == 'nt':
    import ctypes
    # A loader failure should fail CI, not open a blocking native error dialog.
    ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
    env['QT_QPA_PLATFORM'] = 'windows'
    exe = prefix / 'bin/usage-float.exe'
    for name in ['vcruntime140.dll', 'msvcp140.dll']:
        if not (prefix / 'bin' / name).exists():
            raise SystemExit(f'Windows C++ runtime missing: {name}')
    env['QT_PLUGIN_PATH'] = str(prefix / 'plugins')
    env['QML_IMPORT_PATH'] = str(prefix / 'qml')
elif (prefix / 'usage-float.app').exists():
    env['QT_QPA_PLATFORM'] = 'cocoa'
    exe = prefix / 'usage-float.app/Contents/MacOS/usage-float'
    if not (prefix / 'usage-float.app/Contents/Resources/UsageFloat.icns').exists():
        raise SystemExit('macOS bundle icon missing')
else:
    exe = prefix / 'bin/usage-float'
    for name in ['libqwayland-generic.so', 'libqwayland-egl.so']:
        if not (prefix / 'plugins/platforms' / name).exists():
            raise SystemExit(f'Native Wayland plugin missing: {name}')
    if not (prefix / 'share/icons/hicolor/128x128/apps/io.usagefloat.UsageFloat.png').exists():
        raise SystemExit('Linux launcher icon missing')
    env['LD_LIBRARY_PATH'] = str(prefix / 'lib')
    env['QT_PLUGIN_PATH'] = str(prefix / 'plugins')
    env['QML_IMPORT_PATH'] = str(prefix / 'qml')
with tempfile.TemporaryDirectory(prefix='usage-float-smoke-') as directory:
    env['XDG_CONFIG_HOME'] = directory
    screenshot = Path(directory) / 'window.png'
    subprocess.run([str(exe), '--demo', '--wide', '--screenshot', str(screenshot)], env=env, timeout=30, check=True)
    if not screenshot.is_file() or screenshot.stat().st_size < 100:
        raise SystemExit('Installed app did not render its window')
print('Installed application started and rendered successfully.')
