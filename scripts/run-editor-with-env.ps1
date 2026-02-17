param(
    [string]$EditorBinary = ".\\bin\\phoenix_agentic.windows.editor.dev.x86_64.exe"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repoRoot

function Import-EnvFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        return
    }

    foreach ($line in Get-Content -Path $Path) {
        $trimmed = $line.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith("#")) {
            continue
        }

        $separator = $trimmed.IndexOf("=")
        if ($separator -lt 1) {
            continue
        }

        $key = $trimmed.Substring(0, $separator).Trim()
        $value = $trimmed.Substring($separator + 1)

        if (($value.StartsWith('"') -and $value.EndsWith('"')) -or
            ($value.StartsWith("'") -and $value.EndsWith("'"))) {
            $value = $value.Substring(1, $value.Length - 2)
        }

        [Environment]::SetEnvironmentVariable($key, $value, "Process")
    }
}

Import-EnvFile -Path ".env.local"

if ([string]::IsNullOrWhiteSpace($env:PHOENIX_GATEWAY_BASE_URL) -and -not [string]::IsNullOrWhiteSpace($env:PHOENIX_PUBLIC_GATEWAY_URL)) {
    [Environment]::SetEnvironmentVariable("PHOENIX_GATEWAY_BASE_URL", $env:PHOENIX_PUBLIC_GATEWAY_URL, "Process")
}

if ([string]::IsNullOrWhiteSpace($env:PHOENIX_GATEWAY_BASE_URL)) {
    throw "PHOENIX_GATEWAY_BASE_URL or PHOENIX_PUBLIC_GATEWAY_URL must be set in .env.local before launching the editor."
}

if (-not (Test-Path $EditorBinary)) {
    throw "Editor binary not found at $EditorBinary. Build first (dev: build editor)."
}

& $EditorBinary
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
