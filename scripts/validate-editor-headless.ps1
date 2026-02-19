param(
    [ValidateSet("version", "startup", "doctool-temp", "all", "all-full")]
    [string]$Action = "all"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

function Resolve-EditorBinary {
    $candidates = @(
        ".\\bin\\phoenix_agentic.windows.editor.dev.x86_64.console.exe",
        ".\\bin\\phoenix_agentic.windows.editor.x86_64.console.exe",
        ".\\bin\\phoenix_agentic.windows.editor.dev.x86_64.exe",
        ".\\bin\\phoenix_agentic.windows.editor.x86_64.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "No Phoenix editor binary found under .\\bin. Run task 'dev: build editor' first."
}

function Invoke-EditorCheck {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [string]$EditorBinary,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    Write-Host ("Running {0}: {1} {2}" -f $Name, $EditorBinary, ($Arguments -join " ")) -ForegroundColor Cyan
    & $EditorBinary @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw ("{0} failed with exit code {1}." -f $Name, $LASTEXITCODE)
    }
}

$editorBinary = Resolve-EditorBinary

switch ($Action) {
    "version" {
        Invoke-EditorCheck -Name "headless version" -EditorBinary $editorBinary -Arguments @("--headless", "--version")
    }
    "startup" {
        Invoke-EditorCheck -Name "headless startup" -EditorBinary $editorBinary -Arguments @("--headless", "--quit")
    }
    "doctool-temp" {
        $doctoolOutDir = Join-Path $env:TEMP "phoenix-doctool-smoke"
        if (Test-Path $doctoolOutDir) {
            Remove-Item -Path $doctoolOutDir -Recurse -Force
        }
        New-Item -ItemType Directory -Path $doctoolOutDir -Force | Out-Null

        try {
            Invoke-EditorCheck -Name "doctool headless" -EditorBinary $editorBinary -Arguments @("--doctool", $doctoolOutDir, "--headless")
        }
        finally {
            if (Test-Path $doctoolOutDir) {
                Remove-Item -Path $doctoolOutDir -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
    }
    "all" {
        Invoke-EditorCheck -Name "headless version" -EditorBinary $editorBinary -Arguments @("--headless", "--version")
        Invoke-EditorCheck -Name "headless startup" -EditorBinary $editorBinary -Arguments @("--headless", "--quit")
    }
    "all-full" {
        Invoke-EditorCheck -Name "headless version" -EditorBinary $editorBinary -Arguments @("--headless", "--version")
        Invoke-EditorCheck -Name "headless startup" -EditorBinary $editorBinary -Arguments @("--headless", "--quit")

        $doctoolOutDir = Join-Path $env:TEMP "phoenix-doctool-smoke"
        if (Test-Path $doctoolOutDir) {
            Remove-Item -Path $doctoolOutDir -Recurse -Force
        }
        New-Item -ItemType Directory -Path $doctoolOutDir -Force | Out-Null

        try {
            Invoke-EditorCheck -Name "doctool headless" -EditorBinary $editorBinary -Arguments @("--doctool", $doctoolOutDir, "--headless")
        }
        finally {
            if (Test-Path $doctoolOutDir) {
                Remove-Item -Path $doctoolOutDir -Recurse -Force -ErrorAction SilentlyContinue
            }
        }
    }
}

Write-Host "Validation task completed successfully." -ForegroundColor Green
