param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(ValueFromRemainingArguments=$true)][string[]]$Arguments
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$profile = Join-Path ([IO.Path]::GetTempPath()) ('qclip-' + [guid]::NewGuid().ToString('N'))
$names = @('COPYQ_SESSION_NAME','COPYQ_SETTINGS_PATH','COPYQ_ITEM_DATA_PATH','COPYQ_STATE_PATH',
    'COPYQ_PLUGINS','COPYQ_DEFAULT_ICON','COPYQ_SESSION_COLOR','COPYQ_THEME_PREFIX',
    'COPYQ_PASSWORD','COPYQ_LOG_LEVEL','QT_LOGGING_RULES','QT_QPA_PLATFORM','QSG_RENDER_LOOP',
    'QTEST_FUNCTION_TIMEOUT','COPYQ_TESTS_EXECUTABLE','XDG_RUNTIME_DIR')
$previous = @{}
foreach ($name in $names) { $previous[$name] = [Environment]::GetEnvironmentVariable($name) }
try {
    New-Item -ItemType Directory -Path $profile -Force | Out-Null
    $env:COPYQ_SESSION_NAME = 'test'
    $env:COPYQ_SETTINGS_PATH = Join-Path $profile 'config'
    $env:COPYQ_ITEM_DATA_PATH = Join-Path $profile 'items'
    $env:COPYQ_STATE_PATH = Join-Path $profile 'state'
    if (-not $env:COPYQ_PLUGINS) { $env:COPYQ_PLUGINS = '' }
    $env:COPYQ_DEFAULT_ICON = '1'
    $env:COPYQ_SESSION_COLOR = '#f90'
    $env:COPYQ_THEME_PREFIX = Join-Path $repo 'shared/themes'
    $env:COPYQ_PASSWORD = 'TEST123'
    $env:COPYQ_LOG_LEVEL = 'DEBUG'
    $env:QT_LOGGING_RULES = '*.debug=true;qt.*.debug=false'
    $env:QT_QPA_PLATFORM = if ($IsWindows) { 'windows' } elseif ($IsMacOS) { 'cocoa' } elseif ($previous['QT_QPA_PLATFORM']) { $previous['QT_QPA_PLATFORM'] } else { 'xcb' }
    $env:QSG_RENDER_LOOP = 'basic'
    $env:QTEST_FUNCTION_TIMEOUT = '60000'
    if ($IsLinux) {
        if ($env:QT_QPA_PLATFORM -eq 'xcb' -and
            (-not $env:DISPLAY -or -not (Get-Process openbox -ErrorAction SilentlyContinue))) {
            throw 'Start Xvfb and openbox, then set DISPLAY before running'
        }
        if ($env:QT_QPA_PLATFORM -eq 'wayland' -and -not $env:WAYLAND_DISPLAY) {
            throw 'Start a Wayland compositor and set WAYLAND_DISPLAY before running'
        }
        if (-not $env:XDG_RUNTIME_DIR) {
            $env:XDG_RUNTIME_DIR = Join-Path $profile 'runtime'
            New-Item -ItemType Directory -Path $env:XDG_RUNTIME_DIR | Out-Null
            & chmod 700 $env:XDG_RUNTIME_DIR
        }
    }
    if ($IsMacOS -and (Split-Path -Leaf $Executable) -eq 'copyq-tests' -and -not $env:COPYQ_TESTS_EXECUTABLE) {
        $nativeApp = Join-Path (Split-Path -Parent (Resolve-Path $Executable)) 'CopyQ.app/Contents/MacOS/CopyQ'
        if (Test-Path -LiteralPath $nativeApp) { $env:COPYQ_TESTS_EXECUTABLE = $nativeApp }
    }
    & $Executable @Arguments
    $result = $LASTEXITCODE
} finally {
    foreach ($name in $names) { [Environment]::SetEnvironmentVariable($name, $previous[$name]) }
    if (Test-Path -LiteralPath $profile) { Remove-Item -LiteralPath $profile -Recurse -Force }
}
exit $result
