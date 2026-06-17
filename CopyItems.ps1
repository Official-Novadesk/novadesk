param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $RepoRoot) {
    $RepoRoot = $PSScriptRoot
}

function Assert-PathExists {
    param(
        [string]$PathValue,
        [string]$Label
    )

    if (-not (Test-Path $PathValue)) {
        throw "$Label not found: $PathValue"
    }
}

function Copy-DirectoryContent {
    param(
        [string]$SourceDir,
        [string]$DestinationDir
    )

    if (Test-Path $DestinationDir) {
        Remove-Item -Recurse -Force $DestinationDir
    }
    New-Item -ItemType Directory -Path $DestinationDir -Force | Out-Null
    Copy-Item -Path (Join-Path $SourceDir "*") -Destination $DestinationDir -Recurse -Force
}

try {
    $distDir = Join-Path $RepoRoot "dist"
    $distNwmDir = Join-Path $distDir "nwm"
    $distWidgetsDir = Join-Path $distDir "Widgets"
    $distImagesDir = Join-Path $distDir "images"
    $distAddonsDir = Join-Path $distDir "Addons"
    $distNwmTemplateDir = Join-Path $distNwmDir "template"

    $novadeskExeSrc = Join-Path $RepoRoot "src\apps\x64\$Configuration\novadesk\novadesk.exe"
    $widgetsSrc = Join-Path $RepoRoot "src\Widgets\builtin\Widgets"
    $imagesSrc = Join-Path $RepoRoot "src\apps\assets\images"
    $nwmExeSrc = Join-Path $RepoRoot "src\apps\x64\$Configuration\nwm\nwm.exe"
    $nwmTemplateSrc = Join-Path $RepoRoot "src\Widgets\template"
    $installerStubExeSrc = Join-Path $RepoRoot "src\apps\x64\$Configuration\installer_stub\installer_stub.exe"
    $manageExeSrc = Join-Path $RepoRoot "src\apps\x64\$Configuration\manage_novadesk\manage_novadesk.exe"
    $restartExeSrc = Join-Path $RepoRoot "src\apps\x64\$Configuration\restart_novadesk\restart_novadesk.exe"
    $ndpkgInstallerExeSrc = Join-Path $RepoRoot "src\apps\x64\$Configuration\ndpkg_installer\ndpkg_installer.exe"
    $addonsBuildRoot = Join-Path $RepoRoot "src\addons\dist\$Platform\$Configuration"
    $addonProjectNames = @("AppVolume", "AudioLevel", "Brightness", "Hotkey", "NowPlaying", "InputBox", "BlurBehind")

    Assert-PathExists -PathValue $novadeskExeSrc -Label "novadesk.exe"
    Assert-PathExists -PathValue $widgetsSrc -Label "Widgets source"
    Assert-PathExists -PathValue $imagesSrc -Label "images source"
    Assert-PathExists -PathValue $nwmExeSrc -Label "nwm.exe"
    Assert-PathExists -PathValue $nwmTemplateSrc -Label "nwm template source"
    Assert-PathExists -PathValue $installerStubExeSrc -Label "installer_stub.exe"
    Assert-PathExists -PathValue $manageExeSrc -Label "manage_novadesk.exe"
    Assert-PathExists -PathValue $restartExeSrc -Label "restart_novadesk.exe"
    Assert-PathExists -PathValue $ndpkgInstallerExeSrc -Label "ndpkg_installer.exe"
    Assert-PathExists -PathValue $addonsBuildRoot -Label "addons build root"

    New-Item -ItemType Directory -Path $distDir -Force | Out-Null
    New-Item -ItemType Directory -Path $distNwmDir -Force | Out-Null

    Write-Host "Copying build outputs to dist..." -ForegroundColor Cyan
    Copy-Item -Path $novadeskExeSrc -Destination (Join-Path $distDir "novadesk.exe") -Force
    Copy-Item -Path $manageExeSrc -Destination (Join-Path $distDir "manage_novadesk.exe") -Force
    Copy-Item -Path $restartExeSrc -Destination (Join-Path $distDir "restart_novadesk.exe") -Force
    Copy-Item -Path $ndpkgInstallerExeSrc -Destination (Join-Path $distDir "ndpkg_installer.exe") -Force
    Copy-DirectoryContent -SourceDir $widgetsSrc -DestinationDir $distWidgetsDir
    Copy-DirectoryContent -SourceDir $imagesSrc -DestinationDir $distImagesDir
    Copy-Item -Path $nwmExeSrc -Destination (Join-Path $distNwmDir "nwm.exe") -Force
    Copy-DirectoryContent -SourceDir $nwmTemplateSrc -DestinationDir $distNwmTemplateDir
    Copy-Item -Path $installerStubExeSrc -Destination (Join-Path $distNwmDir "installer_stub.exe") -Force

    if (Test-Path $distAddonsDir) {
        Remove-Item -Recurse -Force $distAddonsDir
    }
    New-Item -ItemType Directory -Path $distAddonsDir -Force | Out-Null

    foreach ($addonName in $addonProjectNames) {
        $addonOutDir = Join-Path $addonsBuildRoot $addonName
        Assert-PathExists -PathValue $addonOutDir -Label "addon output directory for $addonName"

        $addonDll = Join-Path $addonOutDir "$addonName.dll"
        if (-not (Test-Path $addonDll)) {
            $fallbackDll = Get-ChildItem -Path $addonOutDir -Filter *.dll -File -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($fallbackDll) {
                $addonDll = $fallbackDll.FullName
            }
        }
        Assert-PathExists -PathValue $addonDll -Label "addon dll for $addonName"
        Copy-Item -Path $addonDll -Destination (Join-Path $distAddonsDir ([System.IO.Path]::GetFileName($addonDll))) -Force
    }

    Write-Host "Copy completed successfully." -ForegroundColor Green
}
catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}