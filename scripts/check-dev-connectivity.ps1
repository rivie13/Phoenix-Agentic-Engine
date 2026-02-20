param(
    [string]$EnvFile = ".env.local",
    [int]$TimeoutSeconds = 15
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$resolvedEnvFile = if ([System.IO.Path]::IsPathRooted($EnvFile)) { $EnvFile } else { Join-Path $repoRoot $EnvFile }

function Import-EnvFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "Environment file not found: $Path"
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

function Invoke-Request {
    param(
        [ValidateSet("GET", "POST")]
        [string]$Method,
        [string]$Url,
        [hashtable]$Headers = @{},
        [string]$Body = "{}"
    )

    $statusCode = 0
    $rawBody = ""

    try {
        if ($Method -eq "GET") {
            $response = Invoke-WebRequest -Method GET -Uri $Url -Headers $Headers -UseBasicParsing -TimeoutSec $TimeoutSeconds -ErrorAction Stop
        }
        else {
            $response = Invoke-WebRequest -Method POST -Uri $Url -Headers $Headers -Body $Body -UseBasicParsing -TimeoutSec $TimeoutSeconds -ErrorAction Stop
        }

        $statusCode = [int]$response.StatusCode
        $rawBody = [string]$response.Content
    }
    catch [System.Net.WebException] {
        $httpResponse = $_.Exception.Response
        if ($null -eq $httpResponse) {
            throw
        }

        $statusCode = [int]$httpResponse.StatusCode
        $stream = $httpResponse.GetResponseStream()
        if ($null -ne $stream) {
            $reader = New-Object System.IO.StreamReader($stream)
            $rawBody = $reader.ReadToEnd()
            $reader.Dispose()
            $stream.Dispose()
        }
    }

    $json = $null
    if (-not [string]::IsNullOrWhiteSpace($rawBody)) {
        try {
            $json = $rawBody | ConvertFrom-Json -ErrorAction Stop
        }
        catch {
            $json = $null
        }
    }

    return [PSCustomObject]@{
        StatusCode = $statusCode
        RawBody = $rawBody
        Json = $json
    }
}

function Mask-Token {
    param([string]$Token)

    if ([string]::IsNullOrWhiteSpace($Token)) {
        return "<empty>"
    }

    if ($Token.Length -le 8) {
        return "********"
    }

    return "$($Token.Substring(0, 4))...$($Token.Substring($Token.Length - 4))"
}

Import-EnvFile -Path $resolvedEnvFile

$baseUrl = $env:PHOENIX_PUBLIC_GATEWAY_URL
if ([string]::IsNullOrWhiteSpace($baseUrl)) {
    $baseUrl = $env:PHOENIX_GATEWAY_BASE_URL
}
if ([string]::IsNullOrWhiteSpace($baseUrl)) {
    $baseUrl = $env:PHOENIX_TEST_BASE_URL
}

if ([string]::IsNullOrWhiteSpace($baseUrl)) {
    throw "Missing base URL. Set PHOENIX_PUBLIC_GATEWAY_URL in $resolvedEnvFile."
}

if (-not ($baseUrl -match '^https?://')) {
    throw "Invalid base URL '$baseUrl'. It must start with http:// or https://"
}

$authMode = $env:PHOENIX_AUTH_MODE
if ([string]::IsNullOrWhiteSpace($authMode)) {
    $authMode = "managed"
}

$token = $env:PHOENIX_API_TOKEN
if ([string]::IsNullOrWhiteSpace($token)) {
    $token = $env:PHOENIX_TEST_TOKEN
}

if (-not [string]::Equals($authMode, "offline", [System.StringComparison]::OrdinalIgnoreCase) -and [string]::IsNullOrWhiteSpace($token)) {
    throw "Missing token for auth mode '$authMode'. Set PHOENIX_API_TOKEN (or PHOENIX_TEST_TOKEN) in $resolvedEnvFile."
}

$configSource = $env:PHOENIX_DEV_CONFIG_SOURCE
if ([string]::IsNullOrWhiteSpace($configSource)) {
    $configSource = $resolvedEnvFile
}

Write-Host "Resolved base URL : $baseUrl" -ForegroundColor Cyan
Write-Host "Resolved auth mode: $authMode" -ForegroundColor Cyan
Write-Host "Resolved source   : $configSource" -ForegroundColor Cyan
Write-Host "Resolved token    : $(Mask-Token -Token $token)" -ForegroundColor DarkGray

$health = Invoke-Request -Method "GET" -Url ($baseUrl.TrimEnd('/') + "/health")
if ($health.StatusCode -ne 200) {
    if ($health.StatusCode -eq 403) {
        throw "Reachability check failed with HTTP 403. Your current IP is likely not allowlisted on the dev Azure gateway. Run backend command: .\scripts\azure-gateway.ps1 allowlist-my-ip"
    }

    throw "Reachability check failed at $baseUrl/health (HTTP $($health.StatusCode)). Body: $($health.RawBody)"
}

$headers = @{
    "Content-Type" = "application/json"
    "x-phoenix-auth-mode" = $authMode
}

if (-not [string]::IsNullOrWhiteSpace($token)) {
    $headers["Authorization"] = "Bearer $token"
}

$handshake = Invoke-Request -Method "POST" -Url ($baseUrl.TrimEnd('/') + "/api/v1/auth/handshake") -Headers $headers -Body "{}"
if ($handshake.StatusCode -ne 200) {
    if ($handshake.StatusCode -eq 403) {
        if ($handshake.RawBody -match "static client token auth is disabled") {
            throw "Auth sanity failed: static token auth is disabled in this environment. Static phx_ tokens are dev-only."
        }

        throw "Auth sanity failed with HTTP 403. If reachability worked, token mode/config is invalid for this environment. Body: $($handshake.RawBody)"
    }

    if ($handshake.StatusCode -eq 401) {
        throw "Auth sanity failed with HTTP 401. Token is missing/invalid. Refresh PHOENIX_API_TOKEN from backend .env.local or Key Vault sync."
    }

    throw "Auth sanity failed at /api/v1/auth/handshake (HTTP $($handshake.StatusCode)). Body: $($handshake.RawBody)"
}

Write-Host "Reachability check: OK" -ForegroundColor Green
Write-Host "Auth handshake   : OK" -ForegroundColor Green
if ($null -ne $handshake.Json) {
    Write-Host "Handshake mode   : $($handshake.Json.mode)" -ForegroundColor DarkGreen
    Write-Host "Actor ID         : $($handshake.Json.actor_id)" -ForegroundColor DarkGreen
}
