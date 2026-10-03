#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Prepare a QClip Windows/macOS release from successful native CI builds.
set -euo pipefail

repo=zerovoxxx/QClip
version="${1:?Usage: draft-release.sh VERSION [WORKDIR]}"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo 'Invalid version'; exit 1; }
tag="v$version"
repo_root="$(git rev-parse --show-toplevel)"
workdir="${2:-$repo_root/release-$version}"
mkdir -p "$workdir"
workdir="$(cd "$workdir" && pwd)"
sha="$(git -C "$repo_root" rev-parse "$tag^{commit}")"
[[ "$(head -1 "$repo_root/CHANGES.md")" = "# $version" ]] || { echo 'Update CHANGES.md first'; exit 1; }

assets=("qclip-$version-setup.exe" "qclip-$version.zip"
        "QClip-$version-macos-13-m1.dmg")
for workflow in build-windows.yml build-macos.yml; do
    run_id="$(gh run list --repo "$repo" --workflow "$workflow" --commit "$sha" --status success --limit 1 --json databaseId --jq '.[0].databaseId // empty')"
    [[ -n "$run_id" ]] || { echo "No successful build for $workflow ($sha)"; exit 1; }
    result="$(gh run view "$run_id" --repo "$repo" --json conclusion --jq .conclusion)"
    [[ "$result" = success ]] || { echo "$workflow is not successful: $result"; exit 1; }
    if [[ "$workflow" = build-windows.yml ]]; then
        gh run download "$run_id" --repo "$repo" --name release-evidence-windows --dir "$workdir/evidence-windows"
        gh run download "$run_id" --repo "$repo" --name release-metadata-windows --dir "$workdir/evidence-windows"
    else
        gh run download "$run_id" --repo "$repo" --name release-evidence-macos-13-m1 --dir "$workdir/evidence-macos"
    fi
    for asset in "${assets[@]}"; do
        case "$workflow:$asset" in
            build-windows.yml:qclip-*|build-macos.yml:QClip-*.dmg)
                if [[ ! -s "$workdir/$asset" ]]; then
                    gh run download "$run_id" --repo "$repo" --name "$asset" --dir "$workdir"
                fi
                ;;
        esac
    done
done
for asset in "${assets[@]}"; do
    [[ -s "$workdir/$asset" ]] || { echo "Missing asset: $asset"; exit 1; }
done

source="$workdir/QClip-$version.tar.gz"
git -C "$repo_root" archive --format=tar.gz --prefix="QClip-$version/" --output="$source" "$tag"
assets+=("QClip-$version.tar.gz")
python3 "$repo_root/utils/verify-release-assets.py" "$version" "$sha" "$workdir"
if command -v sha256sum >/dev/null; then
    (cd "$workdir" && sha256sum "${assets[@]}" > checksums-sha256.txt)
else
    (cd "$workdir" && shasum -a 256 "${assets[@]}" > checksums-sha256.txt)
fi

notes="$workdir/release-notes.md"
awk '/^# / {if (seen) exit; seen=1; next} seen {print}' "$repo_root/CHANGES.md" > "$notes"
python3 - "$notes" "$repo" "$tag" <<'PY'
from pathlib import Path
import sys
path = Path(sys.argv[1])
text = path.read_text()
text = text.replace('(docs/UPSTREAM-CHANGES.md)', f'(https://github.com/{sys.argv[2]}/blob/{sys.argv[3]}/docs/UPSTREAM-CHANGES.md)')
path.write_text(text)
PY
printf '\n使用说明：[QClip 用户手册](https://github.com/%s/blob/%s/docs/USER_GUIDE.md)\n' "$repo" "$tag" >> "$notes"
if gh release view "$tag" --repo "$repo" >/dev/null 2>&1; then
    draft="$(gh release view "$tag" --repo "$repo" --json isDraft --jq .isDraft)"
    [[ "$draft" = true ]] || { echo 'Already public: refuse to replace release assets'; exit 1; }
    gh release edit "$tag" --repo "$repo" --title "QClip $version" --notes-file "$notes"
else
    gh release create "$tag" --repo "$repo" --verify-tag --target "$sha" --draft --title "QClip $version" --notes-file "$notes"
fi
paths=()
for asset in "${assets[@]}" checksums-sha256.txt; do paths+=("$workdir/$asset"); done
gh release upload "$tag" --repo "$repo" "${paths[@]}" --clobber
gh release view "$tag" --repo "$repo" --json url,assets,isDraft
# Publication is a separate explicit step after asset and checksum verification.
