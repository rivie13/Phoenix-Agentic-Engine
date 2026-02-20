param(
    [Parameter(Mandatory = $true)]
    [string]$Url,
    [string]$ApiToken = "",
    [string]$AuthMode = "",
    [string]$ConfigSource = ""
)

$ErrorActionPreference = "Stop"

if (-not ($Url -match '^https?://')) {
    throw "Url must start with http:// or https://"
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$envFile = Join-Path $repoRoot ".env.local"

$lines = @()
if (Test-Path $envFile) {
    $rawLines = Get-Content -Path $envFile
    if ($null -eq $rawLines) {
        $lines = @()
    }
    elseif ($rawLines -is [string]) {
        if ([string]::IsNullOrWhiteSpace($rawLines)) {
            $lines = @()
        }
        else {
            $lines = @($rawLines)
        }
    }
    else {
        $lines = @($rawLines)
    }
}

function Set-Or-ReplaceLine {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [AllowEmptyString()]
        [string[]]$InputLines,
        [Parameter(Mandatory = $true)]
        [string]$Key,
        [Parameter(Mandatory = $true)]
        [AllowEmptyString()]
        [string]$Value
    )

    if ($null -eq $InputLines) {
        $InputLines = @()
    }

    $updatedLines = New-Object System.Collections.Generic.List[string]
    $matched = $false

    foreach ($line in $InputLines) {
        if ($line -match "^\s*$([Regex]::Escape($Key))=") {
            $updatedLines.Add("$Key=$Value")
            $matched = $true
        }
        else {
            $updatedLines.Add($line)
        }
    }

    if (-not $matched) {
        $updatedLines.Add("$Key=$Value")
    }

    return $updatedLines.ToArray()
}

$lines = Set-Or-ReplaceLine -InputLines $lines -Key "PHOENIX_PUBLIC_GATEWAY_URL" -Value $Url
$lines = Set-Or-ReplaceLine -InputLines $lines -Key "PHOENIX_TEST_BASE_URL" -Value $Url

if (-not [string]::IsNullOrWhiteSpace($ApiToken)) {
    $lines = Set-Or-ReplaceLine -InputLines $lines -Key "PHOENIX_API_TOKEN" -Value $ApiToken
    $lines = Set-Or-ReplaceLine -InputLines $lines -Key "PHOENIX_TEST_TOKEN" -Value $ApiToken
}

if (-not [string]::IsNullOrWhiteSpace($AuthMode)) {
    $lines = Set-Or-ReplaceLine -InputLines $lines -Key "PHOENIX_AUTH_MODE" -Value $AuthMode
}

if (-not [string]::IsNullOrWhiteSpace($ConfigSource)) {
    $lines = Set-Or-ReplaceLine -InputLines $lines -Key "PHOENIX_DEV_CONFIG_SOURCE" -Value $ConfigSource
}

Set-Content -Path $envFile -Value $lines -Encoding UTF8
Write-Host "Updated $envFile" -ForegroundColor Green
Write-Host "PHOENIX_PUBLIC_GATEWAY_URL=$Url" -ForegroundColor Yellow
Write-Host "PHOENIX_TEST_BASE_URL=$Url" -ForegroundColor Yellow
if (-not [string]::IsNullOrWhiteSpace($ApiToken)) {
    Write-Host "PHOENIX_API_TOKEN=<updated>" -ForegroundColor Yellow
    Write-Host "PHOENIX_TEST_TOKEN=<updated>" -ForegroundColor Yellow
}
if (-not [string]::IsNullOrWhiteSpace($AuthMode)) {
    Write-Host "PHOENIX_AUTH_MODE=$AuthMode" -ForegroundColor Yellow
}
if (-not [string]::IsNullOrWhiteSpace($ConfigSource)) {
    Write-Host "PHOENIX_DEV_CONFIG_SOURCE=$ConfigSource" -ForegroundColor Yellow
}
