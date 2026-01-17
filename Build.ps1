# Novadesk Build Script
# This script finds MSBuild and builds the project solution.

param(
    [string]$Configuration = "Debug",
    [string]$Platform = "Win32"
)

$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $vswhere)) {
    Write-Host "Error: vswhere.exe not found at $vswhere" -ForegroundColor Red
    Write-Host "Visual Studio Installer is required for this script to work automatically." -ForegroundColor Yellow
    exit 1
}

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host " Novadesk Build System" -ForegroundColor Cyan
Write-Host " Configuration: $Configuration" -ForegroundColor Gray
Write-Host " Platform:      $Platform" -ForegroundColor Gray
Write-Host "====================================================" -ForegroundColor Cyan

# Find MSBuild using vswhere
Write-Host "Locating MSBuild..." -ForegroundColor DarkGray
$msbuildPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1

if (-not $msbuildPath) {
    Write-Host "Error: MSBuild.exe could not be located." -ForegroundColor Red
    exit 1
}

Write-Host "Using: $msbuildPath" -ForegroundColor DarkGray
Write-Host ""

# Run MSBuild
# Using /m for parallel build to speed up things
& $msbuildPath "Novadesk.sln" /t:Build /p:Configuration=$Configuration /p:Platform=$Platform /m /v:minimal

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host " BUILD SUCCESSFUL" -ForegroundColor Green
    Write-Host "====================================================" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "====================================================" -ForegroundColor Red
    Write-Host " BUILD FAILED (Exit Code: $LASTEXITCODE)" -ForegroundColor Red
    Write-Host "====================================================" -ForegroundColor Red
    exit $LASTEXITCODE
}
