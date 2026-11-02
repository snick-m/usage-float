#!/usr/bin/env python3
"""Create a relocatable download from CMake's already-deployed install tree."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('--prefix', type=Path, required=True)
parser.add_argument('--output', type=Path, default=Path('release-assets'))
parser.add_argument('--version', required=True)
parser.add_argument('--platform', required=True, choices=['linux-x86_64', 'windows-x86_64', 'macos-arm64'])
args = parser.parse_args()
if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*', args.version):
    parser.error('Invalid version identifier')
prefix = args.prefix.resolve(strict=True)
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
name = f'usage-float-{args.version}-{args.platform}'
source = Path(__file__).resolve().parent.parent

with tempfile.TemporaryDirectory(prefix='usage-float-package-') as temporary:
    stage = Path(temporary) / name
    shutil.copytree(prefix, stage, symlinks=True)
    shutil.copy2(source / 'README.md', stage / 'README.md')
    if (source / 'packaging/licenses').is_dir():
        shutil.copytree(source / 'packaging/licenses', stage / 'licenses')
    if args.platform.startswith('linux'):
        executable = stage / 'bin/usage-float'
        if not executable.is_file():
            raise SystemExit('Installed Linux executable is missing')
        launch = stage / 'UsageFloat'
        launch.write_text('#!/bin/sh\nset -eu\napp_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                          'export LD_LIBRARY_PATH="$app_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
                          'exec "$app_dir/bin/usage-float" "$@"\n')
        launch.chmod(0o755)
        shutil.copy2(source / 'packaging/install-linux.sh', stage / 'install.sh')
        (stage / 'install.sh').chmod(0o755)
        archive = output / (name + '.tar.gz')
        with tarfile.open(archive, 'w:gz') as tar:
            tar.add(stage, arcname=name)
    elif args.platform.startswith('windows'):
        if not (stage / 'bin/usage-float.exe').is_file():
            raise SystemExit('Installed Windows executable is missing')
        archive = output / (name + '.zip')
        with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as z:
            for path in sorted(stage.rglob('*')):
                if path.is_file():
                    z.write(path, arcname=str(Path(name) / path.relative_to(stage)))
    else:
        app = stage / 'usage-float.app'
        if not app.is_dir():
            raise SystemExit('Installed macOS app bundle is missing')
        app.rename(stage / 'Usage Float.app')
        (stage / 'Applications').symlink_to('/Applications')
        archive = output / (name + '.dmg')
        subprocess.run(['hdiutil', 'create', '-volname', 'Usage Float', '-srcfolder', str(stage),
                        '-ov', '-format', 'UDZO', str(archive)], check=True)

digest = hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest()
archive.with_name(archive.name + '.sha256').write_text(f'{digest}  {archive.name}\n')
print(archive)
