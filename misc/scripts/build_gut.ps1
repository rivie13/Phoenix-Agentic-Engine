param(
    [string]$Platform = "windows",
    [string]$EngineRoot = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineRoot = (Resolve-Path "$PSScriptRoot/../..").Path
} else {
    $EngineRoot = (Resolve-Path $EngineRoot).Path
}

$sourceDir = Join-Path $EngineRoot "modules\ultimate_ai\external\gut\addons\gut"
$destDir = Join-Path $EngineRoot "bin\addons\gut"

if (!(Test-Path $sourceDir)) {
    throw "[gut] Source path not found: $sourceDir"
}

New-Item -ItemType Directory -Force -Path (Join-Path $EngineRoot "bin\addons") | Out-Null

if (Test-Path $destDir) {
    Remove-Item -Recurse -Force $destDir
}

New-Item -ItemType Directory -Force -Path $destDir | Out-Null

$robocopyArgs = @(
    $sourceDir,
    $destDir,
    "/E",
    "/R:2",
    "/W:1",
    "/NFL",
    "/NDL",
    "/NJH",
    "/NJS",
    "/XD", ".git", ".github", ".vscode",
    "/XF", ".DS_Store", ".cursorignore.txt"
)

& robocopy @robocopyArgs | Out-Null
$robocopyExit = $LASTEXITCODE
if ($robocopyExit -ge 8) {
    throw "[gut] robocopy failed with exit code $robocopyExit"
}

$requiredFiles = @(
    (Join-Path $destDir "plugin.cfg"),
    (Join-Path $destDir "gut_plugin.gd")
)

foreach ($requiredFile in $requiredFiles) {
    if (!(Test-Path $requiredFile)) {
        throw "[gut] Missing staged file: $requiredFile"
    }
}

Write-Host "[gut] Staged addon payload for platform: $Platform" -ForegroundColor Green
$global:LASTEXITCODE = 0
