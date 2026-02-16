param(
    [string]$Platform = "windows",
    [string]$EngineRoot = "",
    [string]$NodeVersion = "20.19.0"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineRoot = (Resolve-Path "$PSScriptRoot/../..").Path
} else {
    $EngineRoot = (Resolve-Path $EngineRoot).Path
}

if ($Platform -ne "windows") {
    throw "[node-runtime] Unsupported platform for PowerShell staging: $Platform"
}

$destRoot = Join-Path $EngineRoot "bin\tools\node"
$destDir = Join-Path $destRoot "windows"
$tmpRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("phoenix-node-runtime-" + [System.Guid]::NewGuid().ToString("N"))
$archivePath = Join-Path $tmpRoot "node.zip"
$extractRoot = Join-Path $tmpRoot "extract"

New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null

try {
    New-Item -ItemType Directory -Force -Path $destRoot | Out-Null
    if (Test-Path $destDir) {
        Remove-Item -Recurse -Force $destDir
    }

    $archiveName = "node-v$NodeVersion-win-x64.zip"
    $url = "https://nodejs.org/dist/v$NodeVersion/$archiveName"

    Write-Host "[node-runtime] Downloading $url" -ForegroundColor Cyan
    Invoke-WebRequest -Uri $url -OutFile $archivePath

    Expand-Archive -Path $archivePath -DestinationPath $extractRoot -Force

    $srcNode = Join-Path $extractRoot "node-v$NodeVersion-win-x64\node.exe"
    if (!(Test-Path $srcNode)) {
        throw "[node-runtime] Expected node.exe not found at $srcNode"
    }

    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    Copy-Item -Force $srcNode (Join-Path $destDir "node.exe")

    $stagedNode = Join-Path $destDir "node.exe"
    if (!(Test-Path $stagedNode)) {
        throw "[node-runtime] Failed to stage node.exe at $stagedNode"
    }

    Write-Host "[node-runtime] Staged Node v$NodeVersion runtime for windows" -ForegroundColor Green
}
finally {
    if (Test-Path $tmpRoot) {
        Remove-Item -Recurse -Force $tmpRoot
    }
}
