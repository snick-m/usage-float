#!/usr/bin/env python3
"""Export only Claude allowance fields locally, preserving an existing status line.

Run with --install to copy this bridge into CLAUDE_CONFIG_DIR and configure it.
No network calls; requires Python 3.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import time


def atomic_json(path, value):
    fd, temporary = tempfile.mkstemp(prefix=path.name + '.', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as stream:
            json.dump(value, stream)
            stream.write('\n')
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def load(path):
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except (OSError, ValueError):
        return {}


def command_for(path):
    if os.name == 'nt':
        return subprocess.list2cmdline([sys.executable, str(path)])
    return shlex.join([sys.executable, str(path)])


def install(folder):
    folder.mkdir(parents=True, exist_ok=True)
    settings_path = folder / 'settings.json'
    # Invalid existing settings must fail rather than silently overwrite them.
    settings = json.loads(settings_path.read_text(encoding='utf-8')) if settings_path.exists() else {}
    bridge = folder / 'usage-float-statusline.py'
    command = command_for(bridge)
    current = settings.get('statusLine', {})
    if current.get('command') != command:
        atomic_json(folder / 'usage-float-statusline-previous.json', current)
        if settings_path.exists():
            shutil.copy2(settings_path, folder / ('settings.json.before-usage-float-' + str(time.time_ns())))
    if Path(__file__).resolve() != bridge.resolve():
        shutil.copy2(__file__, bridge)
    # Retain padding, refresh interval, and other display preferences.
    settings['statusLine'] = {**current, 'type': 'command', 'command': command}
    atomic_json(settings_path, settings)
    print('Installed Claude local usage bridge. Restart Claude Code sessions to activate it.')


def export(folder, data):
    limits = {}
    now = int(time.time())
    for key in ('five_hour', 'seven_day'):
        window = data.get('rate_limits', {}).get(key) or {}
        percent, reset = window.get('used_percentage'), window.get('resets_at')
        if (isinstance(percent, (int, float)) and not isinstance(percent, bool)
                and math.isfinite(percent) and isinstance(reset, (int, float))
                and math.isfinite(reset) and now < reset < 1e12):
            limits[key] = {'used_percentage': max(0, min(100, percent)), 'resets_at': reset}
    if not limits:
        return
    target = folder / 'usage-float.json'
    session = hashlib.sha256(str(data.get('session_id', 'default')).encode()).hexdigest()
    state_folder = folder / 'usage-float-statusline-state'
    state_folder.mkdir(parents=True, exist_ok=True)
    state = state_folder / (session + '.json')
    previous = load(state)
    # Status-line redraws are not fresh server observations. API duration changes
    # after responses; identical redraws must not keep old percentages alive.
    marker = [data.get('session_id'), data.get('cost', {}).get('total_api_duration_ms'), limits]
    if previous.get('response_marker') == marker:
        return
    folder.mkdir(parents=True, exist_ok=True)
    atomic_json(state, {'response_marker': marker})
    atomic_json(target, {'observed_at': now, 'rate_limits': limits})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    folder = Path(os.environ.get('CLAUDE_CONFIG_DIR', Path.home() / '.claude')).expanduser()
    if args.install:
        install(folder)
        return
    raw = sys.stdin.buffer.read()
    try:
        export(folder, json.loads(raw))
    except (OSError, ValueError, TypeError, AttributeError):
        pass  # Cache failures must never break the terminal status line.
    previous = load(folder / 'usage-float-statusline-previous.json')
    if previous.get('command'):
        result = subprocess.run(previous['command'], shell=True, input=raw, check=False)
        sys.exit(result.returncode)
    # A useful default when there was no status line before installation.
    try:
        data = json.loads(raw)
        parts = [data.get('model', {}).get('display_name', 'Claude')]
        for key, label in (('five_hour', '5h'), ('seven_day', '7d')):
            percent = (data.get('rate_limits', {}).get(key) or {}).get('used_percentage')
            if isinstance(percent, (int, float)):
                parts.append(f'{label}: {percent:.0f}%')
        print(' | '.join(parts))
    except (ValueError, TypeError, AttributeError):
        pass


if __name__ == '__main__':
    main()
