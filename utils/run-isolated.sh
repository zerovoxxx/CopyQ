#!/usr/bin/env bash
# Run a native app or a selected test with disposable CopyQ data.
set -euo pipefail
if (( $# < 1 )); then
    echo 'Usage: utils/run-isolated.sh EXECUTABLE [ARGUMENTS...]' >&2
    exit 2
fi
repo=$(cd "$(dirname "$0")/.." && pwd)
# Keep Unix-domain socket paths below macOS's sockaddr_un limit.
profile=$(mktemp -d /tmp/qclip.XXXXXX)
trap 'rm -rf "$profile"' EXIT
export COPYQ_SESSION_NAME=test
export COPYQ_SETTINGS_PATH="$profile/config"
export COPYQ_ITEM_DATA_PATH="$profile/items"
export COPYQ_STATE_PATH="$profile/state"
export COPYQ_PLUGINS="${COPYQ_PLUGINS:-}"
export COPYQ_DEFAULT_ICON=1
export COPYQ_SESSION_COLOR='#f90'
export COPYQ_THEME_PREFIX="$repo/shared/themes"
export COPYQ_PASSWORD=TEST123
export COPYQ_LOG_LEVEL=DEBUG
export QT_LOGGING_RULES='*.debug=true;qt.*.debug=false'
export QSG_RENDER_LOOP=basic
export QTEST_FUNCTION_TIMEOUT=60000
case $(uname -s) in
    Darwin) export QT_QPA_PLATFORM=cocoa ;;
    Linux)
        # The caller owns Xvfb/openbox or a Wayland compositor.
        if [[ -z "${XDG_RUNTIME_DIR:-}" ]]; then
            export XDG_RUNTIME_DIR="$profile/runtime"
            mkdir -m 700 "$XDG_RUNTIME_DIR"
        fi
        export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
        if [[ "$QT_QPA_PLATFORM" == xcb ]]; then
            : "${DISPLAY:?Start Xvfb and openbox, then set DISPLAY before running}"
            pgrep -x openbox >/dev/null || { echo 'openbox is required for the isolated X11 session' >&2; exit 2; }
        else
            : "${WAYLAND_DISPLAY:?Start a Wayland compositor first}"
        fi
        ;;
    *) echo 'Use run-isolated.ps1 on Windows' >&2; exit 2 ;;
esac
if [[ "$(basename "$1")" == copyq-tests && -z "${COPYQ_TESTS_EXECUTABLE:-}" ]]; then
    native_app="$(cd "$(dirname "$1")" && pwd)/CopyQ.app/Contents/MacOS/CopyQ"
    if [[ -x "$native_app" ]]; then
        export COPYQ_TESTS_EXECUTABLE="$native_app"
    fi
fi
"$@"
