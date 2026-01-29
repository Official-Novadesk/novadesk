# Novadesk Launch Script
# Use: .\run.ps1 -Script "path/to/script.js"

param(
    [string]$Script = "D:\GITHUB\Novadesk\Widgets\index.js"
)

$exePath = "D:\GITHUB\x64\Debug\Novadesk.exe"

if (-not (Test-Path $exePath)) {
    Write-Host "Error: Novadesk.exe not found at $exePath" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $Script)) {
    Write-Host "Error: Script file not found at $Script" -ForegroundColor Red
    exit 1
}

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host " Launching Novadesk " -ForegroundColor Cyan
Write-Host " Script: $Script" -ForegroundColor Gray
Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop Novadesk." -ForegroundColor DarkGray
Write-Host ""

$process = $null

try {
    # Start the process and pass the script path
    # Using -PassThru to get the process object
    # PowerShell handles the quoting of arguments with spaces automatically
    $process = Start-Process -FilePath $exePath -ArgumentList $Script -PassThru -NoNewWindow

    # Wait for the process to exit
    # This also allows the terminal to receive the stdout from the attached process
    Wait-Process -Id $process.Id
}
catch {
    Write-Host "`nError during execution: $_" -ForegroundColor Red
}
finally {
    # Ensure Novadesk is killed if the script is interrupted (Ctrl+C)
    if ($process -and -not $process.HasExited) {
        Write-Host "`nInterrupted. Stopping Novadesk..." -ForegroundColor Yellow
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ""
Write-Host "Novadesk has exited." -ForegroundColor Cyan
