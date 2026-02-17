param(
    [Parameter(Mandatory = $true)]
    [string]$Url
)

$ErrorActionPreference = "Stop"

if (-not ($Url -match '^https?://')) {
    throw "Url must start with http:// or https://"
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$envFile = Join-Path $repoRoot ".env.local"

$lines = @()
if (Test-Path $envFile) {
    $lines = Get-Content -Path $envFile
}

$updated = New-Object System.Collections.Generic.List[string]
$matched = $false

foreach ($line in $lines) {
    if ($line -match '^\s*PHOENIX_PUBLIC_GATEWAY_URL=') {
        $updated.Add("PHOENIX_PUBLIC_GATEWAY_URL=$Url")
        $matched = $true
    }
    else {
        $updated.Add($line)
    }
}

if (-not $matched) {
    $updated.Add("PHOENIX_PUBLIC_GATEWAY_URL=$Url")
}

Set-Content -Path $envFile -Value $updated.ToArray() -Encoding UTF8
Write-Host "Updated $envFile" -ForegroundColor Green
Write-Host "PHOENIX_PUBLIC_GATEWAY_URL=$Url" -ForegroundColor Yellow
