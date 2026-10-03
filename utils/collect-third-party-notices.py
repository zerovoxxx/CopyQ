#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Include the matching Qt SBOM and dependency source notices in a release."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, required=True)
parser.add_argument('--qt', type=Path, required=True)
parser.add_argument('--dependencies', type=Path, required=True)
args = parser.parse_args()
destination = args.root / 'licenses'
destination.mkdir(parents=True, exist_ok=True)
copied = []
for name in ('sbom', 'licenses', 'LICENSES'):
    source = args.qt / name
    if source.is_dir():
        target = destination / ('Qt-' + name)
        shutil.copytree(source, target, dirs_exist_ok=True)
        copied.append(str(target.relative_to(args.root)))
if not (args.qt / 'sbom').is_dir():
    raise SystemExit('The Qt 6.10 SDK SBOM is missing; do not publish an incomplete notice inventory.')
for component in sorted(args.dependencies.iterdir()):
    if not component.is_dir():
        continue
    for path in component.rglob('*'):
        if not path.is_file():
            continue
        if 'LICENSES' not in path.parts and not path.name.upper().startswith(('LICENSE', 'COPYING', 'NOTICE', 'AUTHORS')):
            continue
        relative = path.relative_to(component)
        target = destination / 'dependencies' / component.name / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        copied.append(str(target.relative_to(args.root)))
inventory = {
    'product': 'QClip',
    'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
    'versions': {name: os.environ.get(name) for name in ('QT_VERSION', 'KF_VERSION', 'KF_PATCH', 'QCA_VERSION', 'QTKEYCHAIN_VERSION', 'SNORETOAST_VERSION', 'OPENSSL_VERSION')},
    'notices': copied,
    'source_references': 'THIRD-PARTY-NOTICES.txt',
}
(args.root / 'THIRD-PARTY-INVENTORY.json').write_text(json.dumps(inventory, indent=2) + '\n', encoding='utf-8')
print(f'Collected Qt SBOM and {len(copied)} dependency notice paths.')
