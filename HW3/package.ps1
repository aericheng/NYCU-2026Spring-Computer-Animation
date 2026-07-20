# Package HW3 submission as HW3_113950011.zip
# Usage: double-click, or:  pwsh -File package.ps1

$ErrorActionPreference = "Stop"
$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

$StudentID = "113950011"
$ZipName   = "HW3_$StudentID.zip"
$ZipPath   = Join-Path $ScriptRoot $ZipName
$Staging   = Join-Path $ScriptRoot "_pkg_staging"

Write-Host "=== Packaging HW3 for $StudentID ===" -ForegroundColor Cyan

# Required source files
$KinematicsCpp = Join-Path $ScriptRoot "src\simulation\kinematics.cpp"
$KinematicsH   = Join-Path $ScriptRoot "include\simulation\kinematics.h"
$MainCpp       = Join-Path $ScriptRoot "InverseKinematics\main.cpp"
$ReportPdf     = Join-Path $ScriptRoot "report.pdf"

# Pre-flight checks
if (-not (Test-Path $KinematicsCpp)) {
    Write-Host "[X] kinematics.cpp not found at: $KinematicsCpp" -ForegroundColor Red
    exit 1
}
Write-Host "[v] kinematics.cpp found" -ForegroundColor Green

if (-not (Test-Path $KinematicsH)) {
    Write-Host "[!] kinematics.h not found (skipping)" -ForegroundColor Yellow
} else {
    Write-Host "[v] kinematics.h found (bonus: FABRIK enum + getters)" -ForegroundColor Green
}
if (-not (Test-Path $MainCpp)) {
    Write-Host "[!] main.cpp not found (skipping)" -ForegroundColor Yellow
} else {
    Write-Host "[v] main.cpp found (bonus: FABRIK selector + iter/residual text)" -ForegroundColor Green
}

if (-not (Test-Path $ReportPdf)) {
    Write-Host "[X] report.pdf not found at: $ReportPdf" -ForegroundColor Red
    Write-Host "    -> Export report.md to report.pdf first." -ForegroundColor Yellow
    exit 1
}
Write-Host "[v] report.pdf found ($([math]::Round((Get-Item $ReportPdf).Length/1KB,1)) KB)" -ForegroundColor Green

# Rebuild staging
if (Test-Path $Staging) { Remove-Item $Staging -Recurse -Force }
New-Item -ItemType Directory -Path $Staging | Out-Null

Copy-Item $KinematicsCpp (Join-Path $Staging "kinematics.cpp") -Force
Copy-Item $ReportPdf     (Join-Path $Staging "report.pdf")     -Force
if (Test-Path $KinematicsH) { Copy-Item $KinematicsH (Join-Path $Staging "kinematics.h") -Force }
if (Test-Path $MainCpp)     { Copy-Item $MainCpp     (Join-Path $Staging "main.cpp")     -Force }

# Remove old zip if exists
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }

# Create zip (compression level optimal)
Compress-Archive -Path (Join-Path $Staging "*") -DestinationPath $ZipPath -CompressionLevel Optimal

# Clean staging
Remove-Item $Staging -Recurse -Force

if (Test-Path $ZipPath) {
    $sizeKB = [math]::Round((Get-Item $ZipPath).Length/1KB, 1)
    Write-Host ""
    Write-Host "Done -> $ZipPath ($sizeKB KB)" -ForegroundColor Green
    Write-Host ""
    Write-Host "Contents:" -ForegroundColor Cyan
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::OpenRead($ZipPath).Entries | ForEach-Object {
        Write-Host ("  - {0,-25} ({1,7:N0} bytes)" -f $_.FullName, $_.Length)
    }
    Write-Host ""
    Write-Host "Next: upload $ZipName to the course system before 2026-05-18 23:59." -ForegroundColor Yellow
} else {
    Write-Host "[X] Zip not created" -ForegroundColor Red
    exit 1
}
