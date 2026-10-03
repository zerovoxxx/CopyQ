#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify downloaded native CI assets before creating a QClip release draft."""
import hashlib
import json
from pathlib import Path
import sys
import tarfile
import zipfile

version, commit, directory = sys.argv[1:]
root = Path(directory)
checks = []

def require(condition, message):
    if not condition:
        raise SystemExit(message)
    checks.append(message)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

with tarfile.open(root / ('QClip-' + version + '.tar.gz')) as archive:
    release_sources = {item.name.split('/', 1)[1]: archive.extractfile(item).read()
                       for item in archive if item.isfile()}

def same_source(actual, expected):
    if actual == expected:
        return True
    try:
        return actual.decode('utf-8').replace('\r\n', '\n') == expected.decode('utf-8').replace('\r\n', '\n')
    except UnicodeDecodeError:
        return False

audits = {}
for platform, audit_name in [('windows', 'release-audit-windows.json'), ('macos', 'release-audit.json')]:
    folder = root / ('evidence-' + platform)
    audit = json.loads((folder / audit_name).read_text(encoding='utf-8-sig'))
    require(audit['platform'] == platform and audit['dependency_profile'] == 'bundled runtime', platform + ' native runtime audit')
    require(not audit['errors'] and audit['checks'] > 0, platform + ' package checks passed')
    source = folder / 'QClip-source.tar.gz'
    require(digest(source) == audit['source_sha256'], platform + ' audited source hash matches')
    with tarfile.open(source) as archive:
        member = next(item for item in archive if item.name.endswith('/source-manifest.json'))
        manifest = json.load(archive.extractfile(member))
        require(manifest['commit'] == commit, platform + ' source commit matches release tag')
        for name, expected in release_sources.items():
            member = archive.getmember('QClip-source/' + name)
            require(member.isfile() and same_source(archive.extractfile(member).read(), expected),
                    platform + ' audited source matches tag: ' + name)
    metadata = json.loads((folder / ('release-metadata-' + platform + '.json')).read_text(encoding='utf-8-sig'))
    require(metadata['commit'] == commit, platform + ' binary metadata commit matches release tag')
    for name, record in metadata['files'].items():
        if name == 'qclip.exe':
            require(record['sha256'] == audit['executable_sha256'], 'Windows audited executable matches metadata')
            continue
        path = root / name
        require(path.is_file() and path.stat().st_size == record['bytes'] and digest(path) == record['sha256'], 'Native asset matches: ' + name)
    if platform == 'windows':
        require(metadata['version'] == version and metadata['product'] == 'QClip', 'Windows version and product metadata')
    audits[platform] = audit

prefix = 'qclip-' + version + '/'
with zipfile.ZipFile(root / ('qclip-' + version + '.zip')) as archive:
    names = set(archive.namelist())
    require(hashlib.sha256(archive.read(prefix + 'qclip.exe')).hexdigest() == audits['windows']['executable_sha256'], 'Portable executable matches native audit')
    for name in ('LICENSE', 'AUTHORS', 'README.md', 'USER_GUIDE.md', 'RELEASE.md', 'THIRD-PARTY-NOTICES.txt', 'THIRD-PARTY-INVENTORY.json'):
        require(prefix + name in names, 'Portable attribution: ' + name)
    for name in ('itemencrypted', 'itemfakevim', 'itemimage', 'itemnotes', 'itempinned', 'itemsync', 'itemtags', 'itemtext'):
        require(any(path.startswith(prefix + 'plugins/') and Path(path).name in (name + '.dll', 'lib' + name + '.dll') for path in names), 'Portable plugin: ' + name)
    require(prefix + 'qml/QtQuick/Controls/Basic/qmldir' in names, 'Portable QML runtime')
    require(any(path.startswith(prefix + 'licenses/Qt-sbom/') for path in names), 'Portable matching Qt SBOM')
    require(not any(Path(path).name == 'itemtests.dll' or Path(path).name.endswith('-tests.exe') for path in names), 'Portable excludes test executables and plugin')

with tarfile.open(root / ('QClip-' + version + '.tar.gz')) as archive:
    names = set(archive.getnames())
    for name in ('LICENSE', 'AUTHORS', 'README.md', 'docs/USER_GUIDE.md', 'src/item/snippetstore.cpp', 'CMakeLists.txt'):
        require('QClip-' + version + '/' + name in names, 'Release source: ' + name)

result = {'commit': commit, 'version': version, 'checks': checks, 'native_audits': audits,
          'assets': {path.name: {'bytes': path.stat().st_size, 'sha256': digest(path)} for path in root.iterdir() if path.suffix in ('.exe', '.zip', '.dmg', '.gz')}}
(root / 'asset-verification.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
