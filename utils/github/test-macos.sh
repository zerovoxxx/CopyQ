#!/bin/bash
set -xeuo pipefail

hdiutil attach QClip*.dmg
ls -Rl /Volumes
app_bundle_path=$(echo /Volumes/QClip-*/QClip.app)
executable="$app_bundle_path/Contents/MacOS/QClip"
runner="${GITHUB_WORKSPACE}/utils/run-isolated.sh"
test "$(lipo -archs "$executable")" = arm64
python3 - <<'PY'
import hashlib, json, os
from pathlib import Path
dmg = next(Path('.').glob('QClip-*.dmg'))
metadata = {'commit': os.environ['GITHUB_SHA'], 'files': {dmg.name: {'sha256': hashlib.sha256(dmg.read_bytes()).hexdigest(), 'bytes': dmg.stat().st_size}}}
Path('release-metadata-macos.json').write_text(json.dumps(metadata, indent=2) + '\n')
print(json.dumps(metadata))
PY
python3 "${GITHUB_WORKSPACE}/utils/package-source.py" QClip-source.tar.gz
python3 "${GITHUB_WORKSPACE}/utils/check-release.py" --platform macos --root "$app_bundle_path" --source QClip-source.tar.gz --output release-audit.json

# Test the app before deployment.
"$runner" "$executable" --help
"$runner" "$executable" --version
"$runner" "$executable" --info

# Test paths and features.
ls "$("$runner" "$executable" info plugins)/"
ls "$("$runner" "$executable" info themes)/"
ls "$("$runner" "$executable" info translations)/"
test "$("$runner" "$executable" info has-global-shortcuts)" -eq "1"

# Disable animations for tests
defaults write -g NSAutomaticWindowAnimationsEnabled -bool false
defaults write -g NSWindowResizeTime -float 0.001

# Run tests (retry once on error).
export COPYQ_TESTS_RERUN_FAILED=1
export COPYQ_TESTS_SKIP_COMMAND_EDIT=1
export COPYQ_TESTS_SKIP_CONFIG_MOVE=1
export COPYQ_TESTS_SKIP_DRAG_AND_DROP=1
export COPYQ_TESTS_SKIP_SLOW_CLIPBOARD=1
export COPYQ_TESTS_EXECUTABLE="$executable"
"$runner" ./copyq-tests "testCore:configPath" "testCore:searchItemsAndCopy" "testCore:keysAndFocusing" \
    "testCore:clipboardUriList" "testCore:paletteSearchAndCopy" "testCore:paletteCommands" "testCore:paletteClipboardFailure" \
    "testCore:paletteEditor" "testCore:palettePaste" "testCore:paletteMimeAndDisplayCommands" \
    "testItemFakeVim:createItem" "testItemFakeVim:paletteEditor"
"$runner" ./copyq-tests \
    testCore:managementActions testCore:managementTabs testCore:managementGroups testCore:managementCommands testCore:managementEditor testCore:managementHistory testCore:managementHistoryCapture testCore:managementDialogs
"$runner" ./copyq-tests \
    testCore:importExportTab testCore:commandConfig testCore:commandLoadTheme testCore:displayCommand \
    testCore:commandDialogFitsContents testCore:commandNotification testCore:commandScreenshot \
    testItemImage:savePng testItemTags:searchTags testItemPinned:keepPinnedIfMaxItemsChanges \
    testItemEncrypted:encryptDecryptData testItemEncrypted:encryptDecryptItems \
    testItemSync:itemsToFiles testItemSync:filesToItems
"$runner" ./copyq-palette-tests \
    modelIdentity queryChanges displayCopiesAndPreview sourceDestructionAndReset \
    qmlKeyboardAndIme actionsAndCancellation explicitCommands standardPreviews \
    nativeWindowIdentification pluginEditorAndSettings nativeInputListeningAndReplacement
"$runner" ./copyq-tests testCore:snippetLifecycle testCore:snippetExpansion
"$runner" ./copyq-snippet-tests storeRoundTrip damagedStorage encryptedStorage storageLimit dynamicDates dynamicClipboardAndRandom richTextAndReferences keywords copyMerging copyQMigration copyQExternalMigration qmlSnippets
"$runner" ./copyq-management-tests \
    multiSelectionIdentity bulkSelectionPerformance sourceLifetimeAndDisplay transferAndDeleteProtection \
    qmlSelectionAndActions nativeDropRoundTrip themeMapping legacyStyleRules pluginPreviewBridge managementGeometry \
    settingsDraftTransaction commandDraftRoundTrip pluginSettingsDraft allPluginSettings historyTimeAndProtection historyCaptureFilters

# Verify the bundle is self-contained: every @rpath reference resolves to a
# library that is actually present in the Frameworks directory.
echo '--- Checking bundle for unresolved @rpath references ---'
frameworks_dir="$app_bundle_path/Contents/Frameworks"
unresolved=$(
    find "$app_bundle_path" -type f \( -name '*.dylib' -o -name '*.so' -o -perm /111 \) -print0 |
    xargs -0 otool -L 2>/dev/null |
    grep -o '@rpath/[^ ]*' |
    sort -u |
    while read -r ref; do
        rel=${ref#@rpath/}
        if [[ ! -e "$frameworks_dir/$rel" ]]; then
            echo "$ref"
        fi
    done || true
)
if [[ -n "$unresolved" ]]; then
    echo 'ERROR: Unresolved @rpath references in bundle:'
    echo "$unresolved"
    exit 1
fi
echo 'OK: All @rpath references resolve within the bundle.'

external=$(
    find "$app_bundle_path" -type f \( -name '*.dylib' -o -name '*.so' -o -perm /111 \) -print0 |
    xargs -0 otool -L 2>/dev/null |
    awk '/^[ \t]+\// {print $1}' |
    sort -u |
    while read -r ref; do
        case "$ref" in
            /System/Library/*|/usr/lib/*) ;;
            *) printf '%s\n' "$ref" ;;
        esac
    done || true
)
if [[ -n "$external" ]]; then
    echo 'ERROR: Bundle depends on external development libraries:'
    echo "$external"
    exit 1
fi
python3 - "$app_bundle_path" <<'PY'
from pathlib import Path
import sys
bundle = Path(sys.argv[1]).resolve()
bad = [str(path.relative_to(bundle)) for path in bundle.rglob('*')
       if path.is_symlink() and (not path.exists() or not path.resolve().is_relative_to(bundle))]
if bad:
    raise SystemExit('Broken or external bundle symlinks: ' + ', '.join(bad))
PY
codesign --verify --deep --strict "$app_bundle_path"

# Verify minimum macOS deployment target is at most 13.0.
echo '--- Checking minimum macOS version ---'
min_version=$(otool -l "$executable" | awk '/LC_BUILD_VERSION/{found=1} found && /minos/{print $2; exit}')
if [[ -z "$min_version" ]]; then
    # Fallback: try LC_VERSION_MIN_MACOSX
    min_version=$(otool -l "$executable" | awk '/LC_VERSION_MIN_MACOSX/{found=1} found && /version/{print $2; exit}')
fi
if [[ -z "$min_version" ]]; then
    echo 'ERROR: Could not determine minimum macOS version from binary.'
    exit 1
fi
echo "Minimum macOS version: $min_version"
major=${min_version%%.*}
if [[ "$major" -gt 13 ]]; then
    echo "ERROR: Minimum macOS version $min_version exceeds 13.x"
    exit 1
fi
echo 'OK: Minimum macOS version is acceptable.'
