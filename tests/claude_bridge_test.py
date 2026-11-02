"""Bridge tests use temporary Claude settings and never touch real accounts."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

BRIDGE = Path(__file__).resolve().parents[1] / 'scripts' / 'claude-statusline.py'


class BridgeTest(unittest.TestCase):
    def test_install_preserves_settings_and_redraw_does_not_refresh_data(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            env = {**os.environ, 'CLAUDE_CONFIG_DIR': temporary}
            original = {'model': 'sonnet', 'statusLine': {
                'type': 'command', 'command': 'echo existing-status', 'padding': 2}}
            (root / 'settings.json').write_text(json.dumps(original))
            for _ in range(2):
                subprocess.run([sys.executable, str(BRIDGE), '--install'], env=env,
                               check=True, capture_output=True)
            settings = json.loads((root / 'settings.json').read_text())
            self.assertEqual(settings['model'], 'sonnet')
            self.assertEqual(settings['statusLine']['padding'], 2)
            self.assertEqual(json.loads((root / 'usage-float-statusline-previous.json').read_text()), original['statusLine'])
            data = {'session_id': 'test', 'cost': {'total_api_duration_ms': 100},
                    'rate_limits': {'five_hour': {'used_percentage': 0, 'resets_at': int(time.time()) + 3600}},
                    'transcript_path': 'PRIVATE_NOT_EXPORTED', 'other': 'PRIVATE_NOT_EXPORTED'}
            def run(value):
                return subprocess.run([sys.executable, str(root / 'usage-float-statusline.py')],
                                      input=json.dumps(value), text=True, capture_output=True, env=env, check=True)
            self.assertEqual(run(data).stdout.strip(), 'existing-status')
            cache = root / 'usage-float.json'
            self.assertNotIn('PRIVATE_NOT_EXPORTED', cache.read_text())
            first = cache.stat().st_mtime_ns
            run(data)
            self.assertEqual(cache.stat().st_mtime_ns, first)
            data['cost']['total_api_duration_ms'] = 200
            run(data)
            self.assertNotEqual(cache.stat().st_mtime_ns, first)
            first = cache.read_text()
            # Another session's export must not make this session's redraw fresh.
            other = {**data, 'session_id': 'other'}
            run(other)
            second = cache.stat().st_mtime_ns
            run(data)
            self.assertEqual(cache.stat().st_mtime_ns, second)
            first = cache.read_text()
            run({'rate_limits': {}})
            self.assertEqual(cache.read_text(), first)

    def test_invalid_settings_are_not_overwritten(self):
        with tempfile.TemporaryDirectory() as temporary:
            settings = Path(temporary) / 'settings.json'
            settings.write_text('{broken')
            result = subprocess.run([sys.executable, str(BRIDGE), '--install'],
                                    env={**os.environ, 'CLAUDE_CONFIG_DIR': temporary}, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(settings.read_text(), '{broken')


if __name__ == '__main__':
    unittest.main()
