$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$backendRepo = Join-Path (Split-Path $repoRoot -Parent) "Phoenix-Agentic-Engine-Backend"
$backendEnv = Join-Path $backendRepo ".env.local"
$engineEnv = Join-Path $repoRoot ".env.local"

if (-not (Test-Path $backendEnv)) {
    throw "Backend .env.local not found at $backendEnv"
}

$url = ""
$baseUrl = ""
$apiToken = ""
$authMode = ""
foreach ($line in Get-Content -Path $backendEnv) {
    if ($line -match '^\s*PHOENIX_PUBLIC_GATEWAY_URL=(.*)$') {
        $url = $matches[1].Trim()
    }
    elseif ($line -match '^\s*PHOENIX_GATEWAY_BASE_URL=(.*)$') {
        $baseUrl = $matches[1].Trim()
    }
    elseif ($line -match '^\s*PHOENIX_API_TOKEN=(.*)$') {
        $apiToken = $matches[1].Trim()
    }
    elseif ($line -match '^\s*PHOENIX_AUTH_MODE=(.*)$') {
        $authMode = $matches[1].Trim()
    }
}

if ([string]::IsNullOrWhiteSpace($url)) {
    throw "PHOENIX_PUBLIC_GATEWAY_URL is not set in backend .env.local"
}

if ([string]::IsNullOrWhiteSpace($authMode)) {
    $authMode = "managed"
}

$setArgs = @("-Url", $url, "-AuthMode", $authMode, "-ConfigSource", "backend:.env.local")
if (-not [string]::IsNullOrWhiteSpace($apiToken)) {
    $setArgs += @("-ApiToken", $apiToken)
}

& (Join-Path $repoRoot "scripts\set-public-gateway-url.ps1") @setArgs

if (-not [string]::IsNullOrWhiteSpace($baseUrl)) {
    $lines = @()
    if (Test-Path $engineEnv) {
        $lines = Get-Content -Path $engineEnv
    }

    $updated = New-Object System.Collections.Generic.List[string]
    $matched = $false
    foreach ($line in $lines) {
        if ($line -match '^\s*PHOENIX_GATEWAY_BASE_URL=') {
            $updated.Add("PHOENIX_GATEWAY_BASE_URL=$baseUrl")
            $matched = $true
        }
        else {
            $updated.Add($line)
        }
    }

    if (-not $matched) {
        $updated.Add("PHOENIX_GATEWAY_BASE_URL=$baseUrl")
    }

    Set-Content -Path $engineEnv -Value $updated.ToArray() -Encoding UTF8
}

Write-Host "Synced engine PHOENIX_PUBLIC_GATEWAY_URL from backend .env.local" -ForegroundColor Green
