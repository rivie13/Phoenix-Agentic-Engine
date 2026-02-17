$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$envFile = Join-Path $repoRoot ".env.local"

if (-not (Test-Path $envFile)) {
    Write-Host ".env.local not found in engine repo." -ForegroundColor Yellow
    exit 0
}

$public = ""
foreach ($line in Get-Content -Path $envFile) {
    if ($line -match '^\s*PHOENIX_PUBLIC_GATEWAY_URL=(.*)$') {
        $public = $matches[1].Trim()
    }
}

Write-Host "Engine .env.local values:" -ForegroundColor Cyan
Write-Host "PHOENIX_PUBLIC_GATEWAY_URL=$public"
