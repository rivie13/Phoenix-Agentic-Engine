param(
    [string]$Platform = "windows",
    [string]$EngineRoot = "",
    [string]$ApiFile = "",
    [string]$SConsCachePath = "",
    [switch]$KeepBuild,
    [switch]$ReuseBuild,
    [switch]$ForceClean
)

$ErrorActionPreference = "Stop"

Write-Host "[gdterm] Build start" -ForegroundColor Cyan

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineRoot = (Resolve-Path "$PSScriptRoot/../..").Path
} else {
    $EngineRoot = (Resolve-Path $EngineRoot).Path
}

$apiFilePath = $ApiFile
if ([string]::IsNullOrWhiteSpace($apiFilePath)) {
    $apiFilePath = Join-Path $EngineRoot "extension_api.json"
}
if (!(Test-Path $apiFilePath)) {
    throw "Missing extension_api.json at $apiFilePath"
}

$apiJson = Get-Content $apiFilePath -Raw | ConvertFrom-Json
$godotCppBranch = "$($apiJson.header.version_major).$($apiJson.header.version_minor)"
if ($apiJson.header.version_status -eq "stable") {
    $godotCppBranch = "godot-$($apiJson.header.version_major).$($apiJson.header.version_minor)-stable"
}

$tempRoot = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { $env:TEMP }
$buildRoot = Join-Path $tempRoot "phoenix-gdterm-build"

$reuseEffective = $ReuseBuild -or -not $ForceClean
if (Test-Path $buildRoot) {
    if ($reuseEffective) {
        if (!(Test-Path (Join-Path $buildRoot ".git"))) {
            Remove-Item -Recurse -Force $buildRoot
        }
    } else {
        Remove-Item -Recurse -Force $buildRoot
    }
}

$git = Get-Command git -ErrorAction Stop
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command python3 -ErrorAction SilentlyContinue
}
if (-not $python) {
    throw "Python executable not found in PATH (expected 'python' or 'python3')."
}

if (!(Test-Path $buildRoot)) {
    & $git -c core.longpaths=true clone --depth 1 https://github.com/rivie13/gdterm.git $buildRoot
}

Push-Location $buildRoot
& $git -c core.longpaths=true fetch --depth 1 origin main
& $git -c core.longpaths=true reset --hard origin/main

$localSubmodulePath = Join-Path $EngineRoot "modules/ultimate_ai/external/gdterm"
if (Test-Path (Join-Path $localSubmodulePath ".git")) {
    $localSha = (& $git -C $localSubmodulePath rev-parse HEAD).Trim()
    if (-not [string]::IsNullOrWhiteSpace($localSha)) {
        & $git -c core.longpaths=true fetch --depth 1 origin $localSha
        & $git -c core.longpaths=true checkout --detach $localSha
    }
}

& $git -c core.longpaths=true submodule update --init --depth 1 godot-cpp src/gdterm/pty/thirdparty/libtmt

Push-Location (Join-Path $buildRoot "godot-cpp")
$desiredBranch = $godotCppBranch
& $git fetch --depth 1 origin $desiredBranch
if ($LASTEXITCODE -ne 0) {
    $desiredBranch = "master"
    & $git fetch --depth 1 origin $desiredBranch
}
& $git checkout -B $desiredBranch "origin/$desiredBranch"
Write-Host "[gdterm] godot-cpp branch: $desiredBranch" -ForegroundColor Cyan
Write-Host "[gdterm] godot-cpp commit: $(& $git rev-parse HEAD)" -ForegroundColor Cyan
Pop-Location

$customApiFile = (Resolve-Path $apiFilePath).Path
$stagedApiRoot = Join-Path $buildRoot "extension_api.json"
$stagedApiGodotCpp = Join-Path $buildRoot "godot-cpp\gdextension\extension_api.json"
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $stagedApiGodotCpp) | Out-Null
Copy-Item -Force $customApiFile $stagedApiRoot
Copy-Item -Force $customApiFile $stagedApiGodotCpp

& $python.Source (Join-Path $EngineRoot "misc/scripts/sanitize_extension_api.py") $stagedApiRoot
& $python.Source (Join-Path $EngineRoot "misc/scripts/sanitize_extension_api.py") $stagedApiGodotCpp

$sconsCommonArgs = @(
    "platform=$Platform",
    "generate_bindings=yes",
    "custom_api_file=$stagedApiRoot"
)
if ($Platform -eq "windows") {
    $sconsCommonArgs += "arch=x86_64"
}
if ($Platform -eq "linuxbsd") {
    $sconsCommonArgs[0] = "platform=linux"
    $sconsCommonArgs += "arch=x86_64"
}
if ($Platform -eq "linux") {
    $sconsCommonArgs += "arch=x86_64"
}
if (-not [string]::IsNullOrWhiteSpace($SConsCachePath)) {
    $sconsCommonArgs += "cache_path=$SConsCachePath"
    Write-Host "[gdterm] SCons cache path: $SConsCachePath" -ForegroundColor Cyan
}

$buildLog = Join-Path $buildRoot "phoenix_gdterm_build.log"
$previousErrorAction = $ErrorActionPreference
$ErrorActionPreference = "Continue"
Write-Host "[gdterm] SCons build running..." -ForegroundColor Cyan
& $python.Source -m SCons @sconsCommonArgs "target=template_debug" 2>&1 | Tee-Object -FilePath $buildLog
$firstExit = $LASTEXITCODE
& $python.Source -m SCons @sconsCommonArgs "target=template_release" 2>&1 | Tee-Object -FilePath $buildLog -Append
$secondExit = $LASTEXITCODE
$ErrorActionPreference = $previousErrorAction

if ($firstExit -ne 0 -or $secondExit -ne 0) {
    if (Test-Path $buildLog) {
        Write-Host "SCons failed. Last 200 lines:" -ForegroundColor Red
        Get-Content $buildLog -Tail 200
    }
    throw "SCons failed (template_debug exit=$firstExit, template_release exit=$secondExit)"
}
Pop-Location

$dst = Join-Path $EngineRoot "bin/addons/gdterm"
if (Test-Path $dst) {
    Remove-Item -Recurse -Force $dst
}
New-Item -ItemType Directory -Force -Path $dst | Out-Null
Copy-Item -Recurse -Force (Join-Path $buildRoot "addons/gdterm/*") $dst

Write-Host "[gdterm] Build finished" -ForegroundColor Green

if (-not $KeepBuild -and -not $reuseEffective) {
    Remove-Item -Recurse -Force $buildRoot
}
