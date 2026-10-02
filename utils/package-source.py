#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Archive the exact development source, including non-ignored new files."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("output", type=Path)
args = parser.parse_args()
repo = Path(__file__).resolve().parent.parent
paths = subprocess.check_output(["git", "ls-files", "-z", "--cached", "--others", "--exclude-standard"], cwd=repo).decode().split("\0")
entries = {}
for name in sorted(set(filter(None, paths))):
    path = repo / name
    if path.resolve() == args.output.resolve():
        continue
    if path.is_symlink():
        entries[name] = {"link": path.readlink().as_posix()}
    elif path.is_file():
        entries[name] = {"sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
manifest = {"commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip(),
            "dirty": bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=repo)), "files": entries}
args.output.parent.mkdir(parents=True, exist_ok=True)
with tarfile.open(args.output, "w:gz", dereference=False) as archive:
    for name in entries:
        archive.add(repo / name, arcname="QClip-source/" + name, recursive=False)
    data = json.dumps(manifest, indent=2, ensure_ascii=False).encode()
    info = tarfile.TarInfo("QClip-source/source-manifest.json")
    info.size = len(data)
    archive.addfile(info, io.BytesIO(data))
print(json.dumps({"source": str(args.output), "files": len(entries),
                  "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(), "dirty": manifest["dirty"]}))
