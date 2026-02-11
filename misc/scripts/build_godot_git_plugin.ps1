param(
    [string]$Platform = "windows",
    [string]$EngineRoot = "",
    [switch]$KeepBuild,
    [switch]$ReuseBuild,
    [switch]$ForceClean
)

$ErrorActionPreference = "Stop"

Write-Host "[git-plugin] Build start" -ForegroundColor Cyan

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineRoot = (Resolve-Path "$PSScriptRoot/../..").Path
} else {
    $EngineRoot = (Resolve-Path $EngineRoot).Path
}

$apiFile = Join-Path $EngineRoot "extension_api.json"
if (!(Test-Path $apiFile)) {
    throw "Missing extension_api.json at $apiFile"
}
$apiJson = Get-Content $apiFile -Raw | ConvertFrom-Json
$versionStatus = $apiJson.header.version_status
$godotCppBranch = "master"
if ($versionStatus -eq "stable") {
    $godotCppBranch = "godot-$($apiJson.header.version_major).$($apiJson.header.version_minor)-stable"
}

$tempRoot = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { $env:TEMP }
$buildRoot = Join-Path $tempRoot "phoenix-git-plugin-build"

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

$cloneArgs = @("-c", "core.longpaths=true", "clone", "--depth", "1", "https://github.com/rivie13/godot-git-plugin.git", $buildRoot)
$git = Get-Command git -ErrorAction Stop
$perl = Get-Command perl -ErrorAction SilentlyContinue
if (-not $perl) {
    $strawberryPerl = "C:\Strawberry\perl\bin\perl.exe"
    if (Test-Path $strawberryPerl) {
        $env:PERL = $strawberryPerl
        $env:Path = "$(Split-Path -Parent $strawberryPerl);$env:Path"
        $perl = Get-Command perl -ErrorAction SilentlyContinue
    }
}
if (-not $perl) {
    throw "Missing perl in PATH. Install Strawberry Perl (or equivalent) and retry."
}
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    throw "Missing cmake in PATH. Install CMake and retry."
}
$nasm = Get-Command nasm -ErrorAction SilentlyContinue
if (-not $nasm) {
    $nasmCandidates = @(
        "C:\Program Files\NASM\nasm.exe",
        "C:\Program Files (x86)\NASM\nasm.exe",
        "C:\Strawberry\c\bin\nasm.exe"
    )
    foreach ($nasmPath in $nasmCandidates) {
        if (Test-Path $nasmPath) {
            $env:Path = "$(Split-Path -Parent $nasmPath);$env:Path"
            $nasm = Get-Command nasm -ErrorAction SilentlyContinue
            if ($nasm) {
                break
            }
        }
    }
}
if (-not $nasm) {
    throw "Missing nasm in PATH. Install NASM and retry."
}

function Ensure-ThirdPartyRepo {
    param(
        [string]$RepoUrl,
        [string]$DestPath,
        [string]$Commit
    )
    if (!(Test-Path $DestPath)) {
        $parent = Split-Path -Parent $DestPath
        if (!(Test-Path $parent)) {
            New-Item -ItemType Directory -Force -Path $parent | Out-Null
        }
        & $git -c core.longpaths=true clone --depth 1 $RepoUrl $DestPath
    }
    Push-Location $DestPath
    & $git -c core.longpaths=true fetch --depth 1 origin $Commit
    & $git -c core.longpaths=true checkout $Commit
    Pop-Location
}

if (!(Test-Path $buildRoot)) {
    & $git @cloneArgs
}

Push-Location $buildRoot
& $git -c core.longpaths=true fetch --depth 1 origin
& $git -c core.longpaths=true reset --hard origin/master
& $git -c core.longpaths=true submodule update --init --recursive --depth 1

$thirdPartyRoot = Join-Path $buildRoot "thirdparty"
Ensure-ThirdPartyRepo -RepoUrl "https://github.com/openssl/openssl.git" -DestPath (Join-Path $thirdPartyRoot "openssl") -Commit "26baecb28ce461696966dac9ac889629db0b3b96"
Ensure-ThirdPartyRepo -RepoUrl "https://github.com/libgit2/libgit2.git" -DestPath (Join-Path $thirdPartyRoot "git2\libgit2") -Commit "b7bad55e4bb0a285b073ba5e02b01d3f522fc95d"
Ensure-ThirdPartyRepo -RepoUrl "https://github.com/libssh2/libssh2.git" -DestPath (Join-Path $thirdPartyRoot "ssh2\libssh2") -Commit "635caa90787220ac3773c1d5ba11f1236c22eae8"

$localPluginRoot = Join-Path $EngineRoot "modules/ultimate_ai/external/godot-git-plugin"
$localPluginSource = Join-Path $localPluginRoot "godot-git-plugin/src/git_plugin.cpp"
$tempPluginSource = Join-Path $buildRoot "godot-git-plugin/src/git_plugin.cpp"
if (Test-Path $localPluginSource) {
    Copy-Item -Force $localPluginSource $tempPluginSource
}
Push-Location (Join-Path $buildRoot "godot-cpp")
$desiredBranch = $godotCppBranch
& $git fetch --depth 1 origin $desiredBranch
if ($LASTEXITCODE -ne 0) {
    $desiredBranch = "master"
    & $git fetch --depth 1 origin $desiredBranch
}
& $git checkout -B $desiredBranch "origin/$desiredBranch"
Write-Host "[git-plugin] godot-cpp branch: $desiredBranch" -ForegroundColor Cyan
Write-Host "[git-plugin] godot-cpp commit: $(& $git rev-parse HEAD)" -ForegroundColor Cyan
Pop-Location
$tempApiFile = Join-Path $tempRoot "extension_api.json"
Copy-Item -Force $apiFile $tempApiFile
if (!(Test-Path $tempApiFile)) {
    throw "Failed to stage extension_api.json at $tempApiFile"
}
$buildLog = Join-Path $buildRoot "phoenix_git_plugin_build.log"
$previousErrorAction = $ErrorActionPreference
$ErrorActionPreference = "Continue"
Write-Host "[git-plugin] SCons build running..." -ForegroundColor Cyan
& C:\Python313\python.exe -m SCons platform=$Platform target=editor dev_build=yes generate_bindings=yes custom_api_file="extension_api.json" 2>&1 | Tee-Object -FilePath $buildLog
$ErrorActionPreference = $previousErrorAction
$sconsExit = $LASTEXITCODE
if ($sconsExit -ne 0) {
    if (Test-Path $buildLog) {
        Write-Host "SCons failed. Last 200 lines:" -ForegroundColor Red
        Get-Content $buildLog -Tail 200
    }
    Write-Host "[git-plugin] Build failed" -ForegroundColor Red
    throw "SCons failed with exit code $sconsExit"
}
Write-Host "[git-plugin] SCons build completed" -ForegroundColor Green
Pop-Location

$dst = Join-Path $EngineRoot "bin/addons/godot-git-plugin"
if (Test-Path $dst) {
    Remove-Item -Recurse -Force $dst
}
New-Item -ItemType Directory -Force -Path $dst | Out-Null
Copy-Item -Recurse -Force (Join-Path $buildRoot "addons/godot-git-plugin/*") $dst

Write-Host "[git-plugin] Build finished" -ForegroundColor Green

if (-not $KeepBuild -and -not $reuseEffective) {
    Remove-Item -Recurse -Force $buildRoot
}
