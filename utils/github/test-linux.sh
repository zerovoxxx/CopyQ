#!/bin/bash
# Runs tests for console and X11.
set -xeuo pipefail

# Enable verbose logging.
export COPYQ_LOG_LEVEL=DEBUG
export QT_LOGGING_RULES="*.debug=true;qt.*.debug=false;qt.*.warning=true"

export COPYQ_TESTS_EXECUTABLE=${COPYQ_TESTS_EXECUTABLE:-"./qclip"}

# Start X11 and window manager.
export DISPLAY=':99.0'
Xvfb :99 -screen 0 1280x960x24 &
sleep 5
openbox &
sleep 8

runner="${GITHUB_WORKSPACE}/utils/run-isolated.sh"

# Test the installed app with disposable settings, history and state.
"$runner" "$COPYQ_TESTS_EXECUTABLE" --help
"$runner" "$COPYQ_TESTS_EXECUTABLE" --version
"$runner" "$COPYQ_TESTS_EXECUTABLE" --info
"$runner" "$COPYQ_TESTS_EXECUTABLE" --start-server exit

# Test handling Unix signals.
"$runner" "$(dirname "$0")/test-signals.sh"

# Test global shortcuts on X11.
"$runner" "$(dirname "$0")/test-linux-global-shortcuts.sh"

# Run tests.
export COPYQ_TESTS_RERUN_FAILED=1
"$runner" ./copyq-tests "$@"
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
    nativeWindowIdentification pluginEditorAndSettings
"$runner" ./copyq-tests testCore:snippetLifecycle testCore:snippetExpansion
"$runner" ./copyq-snippet-tests storeRoundTrip damagedStorage encryptedStorage storageLimit dynamicDates dynamicClipboardAndRandom richTextAndReferences keywords copyMerging copyQMigration copyQExternalMigration qmlSnippets
"$runner" ./copyq-management-tests \
    multiSelectionIdentity bulkSelectionPerformance sourceLifetimeAndDisplay transferAndDeleteProtection \
    qmlSelectionAndActions nativeDropRoundTrip themeMapping legacyStyleRules pluginPreviewBridge managementGeometry \
    settingsDraftTransaction commandDraftRoundTrip pluginSettingsDraft allPluginSettings historyTimeAndProtection historyCaptureFilters
