param(
    [string]$BuildDir = "build-mingw",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$Reconfigure,
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

function Resolve-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "C:\msys64\mingw64\bin\cmake.exe",
        "C:\msys64\ucrt64\bin\cmake.exe",
        "C:\msys64\clang64\bin\cmake.exe"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "cmake.exe was not found in PATH or common install paths. Install CMake or add it to PATH."
}

function Resolve-MingwTool {
    param(
        [string]$ExeName
    )

    $cmd = Get-Command $ExeName -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        "C:\msys64\mingw64\bin\$ExeName",
        "C:\msys64\ucrt64\bin\$ExeName",
        "C:\msys64\clang64\bin\$ExeName"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "$ExeName was not found in PATH or common MinGW locations."
}

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
    & $MSBuildPath $SolutionPath /t:Build /m /nologo /p:Configuration=$Config /p:Platform=$Plat
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed for $SolutionPath ($Config|$Plat)"
    }
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

function Get-CachedCMakeBuildType {
    param(
        [string]$CacheFilePath
    )

    if (-not (Test-Path $CacheFilePath)) {
        return $null
    }

    $line = Select-String -Path $CacheFilePath -Pattern "^CMAKE_BUILD_TYPE:STRING=" | Select-Object -First 1
    if (-not $line) {
        return $null
    }

    return ($line.Line -replace "^CMAKE_BUILD_TYPE:STRING=", "").Trim()
}

try {
    $cmake = Resolve-CMake
    $msbuild = Resolve-MSBuild
    $mingwMake = Resolve-MingwTool -ExeName "mingw32-make.exe"
    $mingwCC = Resolve-MingwTool -ExeName "gcc.exe"
    $mingwCXX = Resolve-MingwTool -ExeName "g++.exe"
    $mingwBin = Split-Path -Parent $mingwCC

    Set-Location $RepoRoot
    $env:PATH = "$mingwBin;$env:PATH"

    $unifiedSolution = Join-Path $RepoRoot "Novadesk-Project.sln"
    Build-Solution -MSBuildPath $msbuild -SolutionPath $unifiedSolution -Config $Configuration -Plat $Platform

    $cmakeBuildType = if ($Configuration -eq "Release") { "MinSizeRel" } else { "Debug" }
    $effectiveBuildDir = Join-Path $BuildDir $cmakeBuildType
    $cacheFile = Join-Path $effectiveBuildDir "CMakeCache.txt"
    $cachedBuildType = Get-CachedCMakeBuildType -CacheFilePath $cacheFile
    $needsConfigure = $Reconfigure -or -not (Test-Path $cacheFile) -or ($cachedBuildType -ne $cmakeBuildType)

    if ($needsConfigure) {
        Write-Host "Configuring MinGW build in $effectiveBuildDir..." -ForegroundColor Cyan
        & $cmake -S . -B $effectiveBuildDir -G "MinGW Makefiles" `
            -DCMAKE_MAKE_PROGRAM="$mingwMake" `
            -DCMAKE_C_COMPILER="$mingwCC" `
            -DCMAKE_CXX_COMPILER="$mingwCXX" `
            -DCMAKE_BUILD_TYPE="$cmakeBuildType"
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configure failed."
        }
    } else {
        Write-Host "Using existing CMake configuration in $effectiveBuildDir (CMAKE_BUILD_TYPE=$cachedBuildType)" -ForegroundColor DarkGray
    }

    $effectiveJobs = $Jobs
    if ($effectiveJobs -le 0) {
        $effectiveJobs = [Environment]::ProcessorCount
        if ($effectiveJobs -lt 1) {
            $effectiveJobs = 1
        }
    }
    Write-Host "Building MinGW target(s) from $effectiveBuildDir with $effectiveJobs parallel job(s)..." -ForegroundColor Cyan
    & $cmake --build $effectiveBuildDir --parallel $effectiveJobs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed."
    }

    $distDir = Join-Path $RepoRoot "dist"
    $distNwmDir = Join-Path $distDir "nwm"
    $distWidgetsDir = Join-Path $distDir "Widgets"
    $distImagesDir = Join-Path $distDir "images"
    $distAddonsDir = Join-Path $distDir "Addons"
    $distNwmTemplateDir = Join-Path $distNwmDir "template"

    $novadeskExeSrc = Join-Path $RepoRoot "$effectiveBuildDir\novadesk.exe"
    $widgetsSrc = Join-Path $RepoRoot "src\Widgets\builtin\Widgets"
    $imagesSrc = Join-Path $RepoRoot "src\apps\assets\images"
    $nwmExeSrc = Join-Path $RepoRoot "src\apps\$Platform\$Configuration\nwm\nwm.exe"
    $nwmTemplateSrc = Join-Path $RepoRoot "src\Widgets\template"
    $installerStubExeSrc = Join-Path $RepoRoot "src\apps\$Platform\$Configuration\installer_stub\installer_stub.exe"
    $manageExeSrc = Join-Path $RepoRoot "src\apps\$Platform\$Configuration\manage_novadesk\manage_novadesk.exe"
    $restartExeSrc = Join-Path $RepoRoot "src\apps\$Platform\$Configuration\restart_novadesk\restart_novadesk.exe"
    $ndpkgInstallerExeSrc = Join-Path $RepoRoot "src\apps\$Platform\$Configuration\ndpkg_installer\ndpkg_installer.exe"
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

    Write-Host "Build completed successfully." -ForegroundColor Green
}
catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
