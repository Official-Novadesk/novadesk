# Check for admin privileges (optional/recommended for some build actions, but maybe not strictly required if just building)
# Write-Host "Starting Build Process..." -ForegroundColor Cyan

# Function to find MSBuild
function Get-MSBuildPath {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
        if ($path) { return $path }
    }
    
    # Fallback to common paths if vswhere fails or isn't found
    $paths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    )
    
    foreach ($p in $paths) {
        if (Test-Path $p) { return $p }
    }
    
    return "msbuild" # Hope it's in PATH
}

$MSBuild = Get-MSBuildPath
Write-Host "Using MSBuild: $MSBuild" -ForegroundColor Gray

# Build Novadesk (x64 Release)
Write-Host "Building Novadesk (Release|x64)..." -ForegroundColor Yellow
$buildArgsNovadesk = @(
    "Novadesk.vcxproj",
    "/p:Configuration=Release",
    "/p:Platform=x64",
    "/t:Rebuild",
    "/v:m"
)
& $MSBuild $buildArgsNovadesk
if ($LASTEXITCODE -ne 0) { Write-Error "Novadesk build failed!"; exit 1 }

# Build nwm (x64 Release)
Write-Host "Building nwm (Release|x64)..." -ForegroundColor Yellow
$buildArgsNwm = @(
    "nwm\nwm.vcxproj",
    "/p:Configuration=Release",
    "/p:Platform=x64",
    "/t:Rebuild",
    "/v:m"
)
& $MSBuild $buildArgsNwm
if ($LASTEXITCODE -ne 0) { Write-Error "nwm build failed!"; exit 1 }

# Check for NSIS
$makensis = "C:\Program Files (x86)\NSIS\makensis.exe"
if (-not (Test-Path $makensis)) {
    # Check if in path
    if (Get-Command makensis -ErrorAction SilentlyContinue) {
        $makensis = "makensis"
    } else {
        Write-Error "NSIS (makensis.exe) not found! Please install NSIS."
        exit 1
    }
}

# Ensure dist_output directory exists
$distDir = "installer\dist_output"
if (-not (Test-Path $distDir)) {
    New-Item -ItemType Directory -Force -Path $distDir | Out-Null
}

Write-Host "Creating Installer..." -ForegroundColor Yellow
& $makensis "installer\setup.nsi"
if ($LASTEXITCODE -ne 0) { Write-Error "Installer creation failed!"; exit 1 }

Write-Host "Build and Setup Complete!" -ForegroundColor Green
