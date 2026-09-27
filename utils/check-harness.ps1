# Verify the local Windows adaptation of the astack document scaffold.
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$canonicalPath = Join-Path $repoRoot 'CLAUDE.md'
$agentPath = Join-Path $repoRoot 'AGENTS.md'
$indexPath = Join-Path $repoRoot 'docs/astack/INDEX.md'
$versionPath = Join-Path $repoRoot 'docs/astack/version'
$planPath = Join-Path $repoRoot 'docs/astack/plan'

foreach ($path in @($canonicalPath, $agentPath, $indexPath, $versionPath, $planPath)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing harness path: $path" }
}
if ((Get-Item -LiteralPath $canonicalPath).LinkType -eq 'SymbolicLink') {
    throw 'CLAUDE.md must be the canonical regular file.'
}
$entry = Get-Item -LiteralPath $agentPath
if ($entry.LinkType -eq 'SymbolicLink') {
    $destination = $entry.ResolveLinkTarget($true)
    if ($destination.FullName -ne $canonicalPath) { throw 'AGENTS.md must link to CLAUDE.md.' }
    $linkMode = 'symbolic link'
} elseif ($IsWindows -and $entry.LinkType -eq 'HardLink') {
    $links = @(& fsutil.exe hardlink list $canonicalPath)
    if ($LASTEXITCODE -ne 0) { throw 'Unable to verify NTFS hard-link identity.' }
    $volumeRoot = [IO.Path]::GetPathRoot($canonicalPath)
    $expected = '\' + $agentPath.Substring($volumeRoot.Length)
    $links = @($links | ForEach-Object { $_.Trim() })
    if ($links -notcontains $expected) { throw 'AGENTS.md and CLAUDE.md must share the same NTFS file.' }
    $linkMode = 'NTFS hard link (documented Windows adaptation)'
} else {
    throw 'AGENTS.md must use the standard symbolic link or the documented Windows hard link.'
}
if ((Get-FileHash -LiteralPath $canonicalPath).Hash -ne (Get-FileHash -LiteralPath $agentPath).Hash) {
    throw 'Governance entry contents differ.'
}
Write-Output "PASS: canonical entry and $linkMode"

$index = Get-Content -LiteralPath $indexPath -Raw
if ($index -notmatch '\|\s*迭代\s*\|.*标题.*状态.*文档.*创建日期') {
    throw 'INDEX.md is missing the iteration table.'
}
$specs = @(Get-ChildItem -LiteralPath $versionPath -Filter 'Iteration*_SPEC.md' -File)
if ($specs.Count -eq 0) { throw 'No iteration SPEC is registered.' }
foreach ($spec in $specs) {
    $content = Get-Content -LiteralPath $spec.FullName -Raw
    $match = [regex]::Match($content, '(?m)^>\s*\|\s*文档状态\s*\|\s*(待实施|开发中|已完成|阻塞)\s*\|')
    if (-not $match.Success) { throw "Missing or invalid SPEC status: $($spec.Name)" }
    $row = @($index -split '\r?\n' | Where-Object { $_.StartsWith('|') -and $_.Contains($spec.Name) })
    if ($row.Count -ne 1 -or $row[0] -notmatch ('\|\s*' + $match.Groups[1].Value + '\s*\|')) {
        throw "INDEX/SPEC state mismatch: $($spec.Name)"
    }
}
Write-Output "PASS: INDEX and $($specs.Count) SPEC status entries"

$documents = @($canonicalPath, $indexPath) + @($specs.FullName)
$documents += @(Get-ChildItem -LiteralPath $planPath -Filter '*.md' -File | ForEach-Object { $_.FullName })
foreach ($document in $documents) {
    $content = Get-Content -LiteralPath $document -Raw
    if ($content -match '(?m)[\t ]+\r?$') { throw "Trailing whitespace: $document" }
    if (-not $content.EndsWith("`n")) { throw "Missing final newline: $document" }
    if ($content -match '\{\{PROJECT_(NAME|DESC)\}\}|（待补充：') {
        throw "Unresolved template text: $document"
    }
    foreach ($match in [regex]::Matches($content, '\]\(([^)]+)\)')) {
        $target = $match.Groups[1].Value
        if ($target -match '^[a-zA-Z][a-zA-Z0-9+.-]*:|^#') { continue }
        $target = ($target -split '#', 2)[0]
        $resolved = Join-Path (Split-Path -Parent $document) $target
        if (-not (Test-Path -LiteralPath $resolved)) { throw "Broken link in ${document}: $target" }
    }
}
Write-Output "PASS: local document links and template migration ($($documents.Count) documents)"

$settings = Get-Content -LiteralPath (Join-Path $repoRoot '.claude/settings.json') -Raw | ConvertFrom-Json
if ($settings.env.COPYQ_STATE_PATH -ne 'build/copyq-test-conf' -or $settings.env.QT_QPA_PLATFORM -ne 'xcb') {
    throw 'Claude test environment conflicts with the inherited isolated X11 profile.'
}
Write-Output 'PASS: isolated test environment configuration'
Write-Output 'Harness verification passed. Application build and GUI behavior are separate gates.'
