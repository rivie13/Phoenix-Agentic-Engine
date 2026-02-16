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

$sourceDir = Join-Path $EngineRoot "modules\ultimate_ai\external\bfxr2-mcp-server"
$destDir = Join-Path $EngineRoot "bin\addons\bfxr2-mcp-server"
$bridgeSource = Join-Path $EngineRoot "modules\ultimate_ai\tools\phoenix_bridge.js"

if (!(Test-Path $sourceDir)) {
    throw "[bfxr] Source path not found: $sourceDir"
}

if (!(Test-Path $bridgeSource)) {
    throw "[bfxr] Missing bridge script: $bridgeSource"
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
    throw "[bfxr] robocopy failed with exit code $robocopyExit"
}

Copy-Item -Force $bridgeSource (Join-Path $destDir "phoenix_bridge.js")

$requiredFiles = @(
    (Join-Path $destDir "mcp-server.js"),
    (Join-Path $destDir "package.json"),
    (Join-Path $destDir "phoenix_bridge.js")
)

foreach ($requiredFile in $requiredFiles) {
    if (!(Test-Path $requiredFile)) {
        throw "[bfxr] Missing staged file: $requiredFile"
    }
}

Write-Host "[bfxr] Staged runtime payload for platform: $Platform" -ForegroundColor Green
$global:LASTEXITCODE = 0
