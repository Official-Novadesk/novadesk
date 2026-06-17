param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

function Resolve-MSBuild {
    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
        if ($found) {
            return $found
        }
    }

    throw "msbuild.exe was not found in PATH and could not be located with vswhere."
}

function Build-Solution {
    param(
        [string]$MSBuildPath,
        [string]$SolutionPath,
        [string]$Config,
        [string]$Plat
    )

    if (-not (Test-Path $SolutionPath)) {
        throw "Solution file not found: $SolutionPath"
    }

    Write-Host "Building $SolutionPath ($Config|$Plat) with MSBuild..." -ForegroundColor Cyan
    & $MSBuildPath $SolutionPath /t:Build /m /nologo /v:minimal /p:Configuration=$Config /p:Platform=$Plat
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed for $SolutionPath ($Config|$Plat)"
    }
}

try {
    $msbuild = Resolve-MSBuild

    Set-Location $RepoRoot

    $unifiedSolution = Join-Path $RepoRoot "Novadesk-Project.sln"
    Build-Solution -MSBuildPath $msbuild -SolutionPath $unifiedSolution -Config $Configuration -Plat $Platform

    # Copy build outputs to dist
    $copyScript = Join-Path $RepoRoot "CopyItems.ps1"
    & $copyScript -Configuration $Configuration -Platform $Platform

    Write-Host "Build completed successfully." -ForegroundColor Green
}
catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}