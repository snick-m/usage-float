"""Run with dbus-run-session; never touches real credentials or preferences."""
import ast
import json
import os
import selectors
from pathlib import Path
import subprocess
import sys
import tempfile
import time

with tempfile.TemporaryDirectory(prefix='usage-float-panel-test-') as temporary:
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
               XDG_CONFIG_HOME=temporary, CLAUDE_CONFIG_DIR=temporary)
    process = subprocess.Popen([str(Path(sys.argv[1]).resolve()), '--demo'], env=env,
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    monitor = None
    def call(method, *args):
        return subprocess.run([
            'gdbus', 'call', '--session', '--dest', 'io.usagefloat.UsageFloat.Panel.Demo',
            '--object-path', '/io/usagefloat/UsageFloat/Panel',
            '--method', 'io.usagefloat.UsageFloat.Panel.' + method, *args,
        ], text=True, capture_output=True, timeout=5)

    def snapshot():
        result = call('GetSnapshot')
        if result.returncode:
            raise RuntimeError(result.stderr)
        return json.loads(ast.literal_eval(result.stdout)[0])

    try:
        deadline = time.monotonic() + 10
        while True:
            if process.poll() is not None:
                raise RuntimeError(process.stderr.read().decode())
            try:
                data = snapshot()
                if data['claude']['status'] == data['codex']['status'] == 'ready':
                    break
            except RuntimeError:
                pass
            if time.monotonic() > deadline:
                raise RuntimeError('Panel service did not become ready')
            time.sleep(0.05)
        assert data['claude']['rows'][0]['displayPercent'] == 32
        version = subprocess.run([str(Path(sys.argv[1]).resolve()), '--version'], env=env,
                                 text=True, capture_output=True, check=True, timeout=5)
        assert data['version'] == version.stdout.strip().split()[-1]
        monitor = subprocess.Popen([
            'gdbus', 'monitor', '--session', '--dest', 'io.usagefloat.UsageFloat.Panel.Demo',
            '--object-path', '/io/usagefloat/UsageFloat/Panel',
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        selector = selectors.DefaultSelector()
        selector.register(monitor.stdout, selectors.EVENT_READ)
        def wait_for_output(expected):
            output = b''
            deadline = time.monotonic() + 5
            while expected not in output:
                if not selector.select(max(0, deadline - time.monotonic())):
                    raise RuntimeError(f'Missing D-Bus monitor output: {expected!r}')
                chunk = os.read(monitor.stdout.fileno(), 65536)
                if not chunk:
                    raise RuntimeError('D-Bus monitor exited')
                output += chunk
            return output
        wait_for_output(b'is owned by')
        assert call('SetShowRemaining', 'true').returncode == 0
        signal = wait_for_output(b'SnapshotChanged')
        assert b'"showRemaining":true' in signal
        # A panel client's disappearing bus connection must restore the tray.
        assert call('SetPanelActive', 'true').returncode == 0
        lifecycle = wait_for_output(b'panelActiveChanged (false,')
        assert b'panelActiveChanged (true,' in lifecycle
        selector.close()
        assert snapshot()['claude']['rows'][0]['displayPercent'] == 68
        assert snapshot()['showRemaining'] is True
        for method in ['Hide', 'Show', 'Refresh']:
            assert call(method).returncode == 0, method
        # Demo data is isolated from the real panel service name.
        result = subprocess.run(['gdbus', 'call', '--session', '--dest',
                                 'org.freedesktop.DBus', '--object-path', '/org/freedesktop/DBus',
                                 '--method', 'org.freedesktop.DBus.NameHasOwner',
                                 'io.usagefloat.UsageFloat.Panel'], text=True, capture_output=True, check=True)
        assert 'false' in result.stdout
        print('Panel D-Bus integration passed')
    finally:
        if monitor:
            monitor.terminate()
            monitor.wait(timeout=5)
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
