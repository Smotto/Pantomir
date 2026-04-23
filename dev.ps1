# dev.ps1 — Enter VS2022 dev shell and configure/build Pantomir
# Usage:
#   .\dev.ps1              # configure only
#   .\dev.ps1 build        # configure + build debug
#   .\dev.ps1 build release # configure + build release

param(
    [string]$Action = "configure",
    [string]$Config = "debug"
)

# Auto-detect VS2022 installation
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Error "vswhere not found. Install Visual Studio 2022."
    exit 1
}

$vsPath = & $vsWhere -latest -property installationPath
if (-not $vsPath) {
    Write-Error "No Visual Studio installation found."
    exit 1
}

# Enter VS dev shell (sets PATH, LIB, INCLUDE, etc.)
$devShellModule = Join-Path $vsPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
if (-not (Test-Path $devShellModule)) {
    Write-Error "VS DevShell module not found at: $devShellModule"
    exit 1
}

Import-Module $devShellModule
Enter-VsDevShell -VsInstallPath $vsPath -DevCmdArguments "-arch=x64" -SkipAutomaticLocation | Out-Null

Write-Host "VS2022 dev environment loaded from: $vsPath" -ForegroundColor Green

# Configure
Write-Host "`n--- Configuring ---" -ForegroundColor Cyan
cmake --preset default
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Build if requested
if ($Action -eq "build") {
    Write-Host "`n--- Building ($Config) ---" -ForegroundColor Cyan
    cmake --build --preset $Config
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
