#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Audit a QClip installed layout without starting clipboard monitoring."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import subprocess
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--platform", choices=["macos", "windows", "linux"], required=True)
parser.add_argument("--root", type=Path, required=True)
parser.add_argument("--min-macos", default="13.0")
parser.add_argument("--source", type=Path)
parser.add_argument("--standalone", action="store_true", help="Require a bundled Linux Qt/QML runtime")
parser.add_argument("--output", type=Path)
args = parser.parse_args()
root = args.root.resolve()
errors = []
checks = []

def require(condition, message):
    (checks if condition else errors).append(message)

def within(path):
    return path.resolve().is_relative_to(root)

def command(argv):
    return subprocess.check_output(argv, text=True, stderr=subprocess.STDOUT)

require(root.is_dir(), "Installed directory exists")
files = list(root.rglob("*")) if root.is_dir() else []
for path in files:
    if path.is_symlink():
        require(path.exists() and within(path), "Internal valid symlink: " + str(path.relative_to(root)))

if args.platform == "macos":
    contents = root / "Contents"
    executable = contents / "MacOS/QClip"
    plugins = contents / "PlugIns/copyq"
    licenses = contents / "Resources"
    try:
        info = plistlib.loads((contents / "Info.plist").read_bytes())
        require(info.get("CFBundleIdentifier") == "io.github.zerovoxxx.QClip", "Distinct QClip bundle ID")
        require(info.get("CFBundleExecutable") == "QClip", "QClip bundle executable")
    except (OSError, plistlib.InvalidFileException) as exc:
        errors.append(str(exc))
    require((contents / "Resources/qml/QtQuick/Controls/Basic/qmldir").is_file(), "Qt Quick Basic QML metadata")
    require((contents / "PlugIns/platforms/libqcocoa.dylib").is_file(), "Cocoa platform plugin")
    def os_version(value):
        return tuple((list(map(int, value.split("."))) + [0, 0])[:3])
    maximum = os_version(args.min_macos)
    for path in files:
        if not path.is_file() or path.is_symlink():
            continue
        with path.open("rb") as handle:
            magic = handle.read(4)
        if magic not in (b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf"):
            continue
        loads = command(["otool", "-l", str(path)])
        versions = re.findall(r"\bminos ([0-9.]+)", loads) or re.findall(r"cmd LC_VERSION_MIN_MACOSX\s+cmdsize \d+\s+version ([0-9.]+)", loads)
        for version in versions:
            require(os_version(version) <= maximum,
                    f"Minimum macOS {version} <= {args.min_macos}: {path.relative_to(root)}")
        rpaths = re.findall(r"cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset", loads)
        rpaths.append("@executable_path/../Frameworks")
        def expand(value):
            return Path(value.replace("@executable_path", str(executable.parent)).replace("@loader_path", str(path.parent)))
        refs = re.findall(r"^\s+([^\s]+) \(compatibility version", command(["otool", "-L", str(path)]), re.M)
        for ref in refs:
            if ref.startswith(("/System/Library/", "/usr/lib/")):
                continue
            candidates = [expand(prefix + "/" + ref[7:]) for prefix in rpaths] if ref.startswith("@rpath/") else [expand(ref)]
            require(any(candidate.exists() and within(candidate) for candidate in candidates), f"Bundled dependency {ref}: {path.relative_to(root)}")
    require(subprocess.run(["codesign", "--verify", "--deep", "--strict", str(root)], capture_output=True).returncode == 0, "Bundle signature verifies")
elif args.platform == "windows":
    executable = root / "qclip.exe"
    plugins = root / "plugins"
    licenses = root
    require((root / "Qt6Core.dll").is_file(), "Qt Core DLL")
    require((root / "platforms/qwindows.dll").is_file(), "Windows platform plugin")
    require((root / "qml/QtQuick/Controls/Basic/qmldir").is_file(), "Qt Quick Basic QML metadata")
    system_directory = Path(os.environ.get("WINDIR", "C:/Windows")) / "System32"
    system = re.compile(r"^((api-ms-|ext-ms-)[\w-]+|kernel32|user32|advapi32|gdi32|gdi32full|shell32|ole32|oleaut32|ntdll|comdlg32|comctl32|imm32|icu|ws2_32|msvcrt|ucrtbase|bcrypt|crypt32|secur32|rpcrt4|dwmapi|winmm|version|shlwapi|uxtheme|netapi32|userenv|wtsapi32|iphlpapi|dnsapi|d3d\d+|dxgi|dxcore|opengl32|glu32|normaliz|winhttp|powrprof|propsys|cfgmgr32|setupapi)\.dll$", re.I)
    for path in files:
        if path.suffix.lower() not in (".exe", ".dll"):
            continue
        for ref in re.findall(r"^\s+([\w.-]+\.dll)\s*$", command(["dumpbin", "/DEPENDENTS", str(path)]), re.M | re.I):
            require((bool(system.match(ref)) or (not ref.lower().startswith(("msvcp", "vcruntime")) and (system_directory / ref).is_file())) or any(candidate.name.lower() == ref.lower() for candidate in files), f"Bundled or system DLL {ref}: {path.relative_to(root)}")
else:
    executable = root / "usr/bin/qclip" if (root / "usr/bin/qclip").exists() else root / "bin/qclip"
    prefix = executable.parent.parent
    licenses = prefix / "share/doc/qclip"
    plugin_dirs = [path for path in files if path.is_dir() and path.name == "plugins" and "qclip" in path.parts]
    plugins = plugin_dirs[0] if plugin_dirs else prefix / "lib/qclip/plugins"
    desktop = prefix / "share/applications/io.github.zerovoxxx.QClip.desktop"
    require(desktop.is_file() and "Exec=qclip" in desktop.read_text(), "QClip desktop entry")
    if args.standalone:
        require(any(path.name.startswith("libQt6Core.so") for path in files), "Bundled Qt Core library")
        require(any(path.name == "libqxcb.so" for path in files), "Bundled XCB platform plugin")
        require(any(path.name == "qmldir" and "QtQuick/Controls/Basic" in path.as_posix() for path in files), "Bundled Qt Quick Basic QML metadata")
    library_dirs = {str(path.parent) for path in files if path.is_file() and ".so" in path.name}
    dependency_env = dict(os.environ, LD_LIBRARY_PATH=os.pathsep.join(sorted(library_dirs))) if args.standalone else None
    host_libraries = re.compile(r"^(linux-vdso|ld-linux|lib(c|m|dl|pthread|rt|resolv|gcc_s|stdc\+\+|X11|Xext|Xrender|Xfixes|Xrandr|Xcursor|Xi|Xau|Xdmcp|xcb[^ ]*|GL[^ ]*|EGL[^ ]*|OpenGL|drm[^ ]*|gbm|wayland[^ ]*|xkbcommon[^ ]*|z))[-.]")
    for path in files:
        if not path.is_file() or path.is_symlink():
            continue
        with path.open("rb") as handle:
            magic = handle.read(4)
        if magic == b"\x7fELF":
            dependencies = subprocess.check_output(["ldd", str(path)], text=True, stderr=subprocess.STDOUT, env=dependency_env)
            require("not found" not in dependencies, "ELF dependencies resolve: " + str(path.relative_to(root)))
            if args.standalone:
                for name, dependency in re.findall(r"^\s*(\S+) => (/\S+)", dependencies, re.M):
                    require(within(Path(dependency)) or bool(host_libraries.match(name)), f"Bundled ELF dependency {name}: {path.relative_to(root)}")

require(executable.is_file(), "QClip executable exists")
for name in ("itemencrypted", "itemfakevim", "itemimage", "itemnotes", "itempinned", "itemsync", "itemtags", "itemtext"):
    require(any(name in path.name and path.is_file() for path in plugins.glob("*")), "Plugin " + name)
for name in ("LICENSE", "AUTHORS", "README.md", "USER_GUIDE.md", "RELEASE.md", "THIRD-PARTY-NOTICES.txt"):
    require((licenses / name).is_file(), "Package attribution: " + name)
if args.platform in ("windows", "macos"):
    require((licenses / "THIRD-PARTY-INVENTORY.json").is_file(), "Matching dependency inventory")
    require((licenses / "licenses/Qt-sbom").is_dir(), "Matching Qt SDK SBOM")
for name in ("Qt-LGPL-3.0.txt", "QCA-COPYING.txt", "QtKeychain-COPYING.txt", "OpenSSL-LICENSE.txt", "ICU-LICENSE.txt", "Font-Awesome.txt", "miniaudio.txt", "LibQxt-COPYING.txt", "FakeVim-LGPL.txt"):
    require((licenses / "licenses" / name).is_file(), "Dependency license text: " + name)

source_hash = None
if args.source:
    source_hash = hashlib.sha256(args.source.read_bytes()).hexdigest()
    with tarfile.open(args.source) as archive:
        names = archive.getnames()
        for name in ("LICENSE", "CMakeLists.txt", "src/gui/mainwindow_snippets.cpp", "src/item/snippetstore.cpp", "src/platform/platforminput.h"):
            require(any(path.endswith("/" + name) for path in names), "Corresponding source: " + name)
        manifests = [member for member in archive if member.name.endswith("/source-manifest.json")]
        for member in manifests:
            manifest = json.load(archive.extractfile(member))
            prefix = member.name.rsplit("/", 1)[0] + "/"
            for name, record in manifest["files"].items():
                entry = archive.getmember(prefix + name)
                if "sha256" in record:
                    require(hashlib.sha256(archive.extractfile(entry).read()).hexdigest() == record["sha256"], "Source hash: " + name)
                else:
                    require(entry.issym() and entry.linkname == record["link"], "Source link: " + name)
result = {"dependency_profile": "host-resolved ELF" if args.platform == "linux" and not args.standalone else "bundled runtime", "platform": args.platform, "root": str(root), "checks": len(checks), "errors": errors,
          "bytes": sum(path.stat().st_size for path in files if path.is_file() and not path.is_symlink()),
          "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest() if executable.is_file() else None, "source_sha256": source_hash}
output = json.dumps(result, indent=2)
if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output + "\n")
print(output)
raise SystemExit(bool(errors))
