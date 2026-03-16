param(
    [string]$BuildDir = "build-release",
    [string]$InstallDir = "install-release",
    [string]$QtPath = "F:\qt\5.15.2\msvc2019_64",
    [string]$BinaryCreatorPath = "F:\Qt\Tools\QtInstallerFramework\4.10\bin\binarycreator.exe",
    [string]$InstallerOutput = "PlotJuggler-Windows-installer.exe",
    [switch]$SkipConfigure,
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDirAbs = Join-Path $repoRoot $BuildDir
$installDirAbs = Join-Path $repoRoot $InstallDir
$installerDir = Join-Path $repoRoot "installer"
$installerDataDir = Join-Path $installerDir "io.plotjuggler.application\data"
$installedBinDir = Join-Path $installDirAbs "bin"
$installedPluginDir = Join-Path $installDirAbs "lib\plotjuggler\plugins"
$windeployScript = Join-Path $installerDir "windeploy_pj.bat"
$windeployQt = Join-Path $QtPath "bin\windeployqt.exe"
$enterMsvc = Join-Path $repoRoot "enter-msvc.ps1"
$installerOutputAbs = Join-Path $repoRoot $InstallerOutput

if (-not (Test-Path $enterMsvc)) {
    throw "Missing enter-msvc.ps1 at $enterMsvc"
}

if (-not (Test-Path $windeployQt)) {
    throw "windeployqt.exe not found at $windeployQt"
}

if (-not (Test-Path $BinaryCreatorPath)) {
    throw "binarycreator.exe not found at $BinaryCreatorPath"
}

if (-not (Test-Path $installerDataDir)) {
    New-Item -ItemType Directory -Force -Path $installerDataDir | Out-Null
}

$configureCommand = @"
. "$enterMsvc"
cmake -S "$repoRoot" -B "$buildDirAbs" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$installDirAbs" -DBUILD_TESTING=OFF
"@

$buildCommand = @"
. "$enterMsvc"
cmake --build "$buildDirAbs" --target install --parallel
"@

if (-not $SkipConfigure) {
    Write-Host "Configuring release build..."
    powershell -ExecutionPolicy Bypass -Command $configureCommand
}

if (-not $SkipBuild) {
    Write-Host "Building and installing release artifacts..."
    powershell -ExecutionPolicy Bypass -Command $buildCommand
}

Write-Host "Refreshing installer payload..."
Remove-Item -Recurse -Force (Join-Path $installerDataDir "*") -ErrorAction SilentlyContinue

if (-not (Test-Path $installedBinDir)) {
    throw "Installed bin directory not found at $installedBinDir"
}

Copy-Item (Join-Path $installedBinDir "*") $installerDataDir -Recurse -Force

if (Test-Path $installedPluginDir) {
    Write-Host "Copying plugin DLLs into installer payload..."
    Copy-Item (Join-Path $installedPluginDir "*") $installerDataDir -Recurse -Force
} else {
    Write-Warning "Installed plugin directory not found at $installedPluginDir"
}

Write-Host "Deploying Qt runtime files..."
& $windeployScript $windeployQt
if ($LASTEXITCODE -ne 0) {
    throw "windeploy_pj.bat failed with exit code $LASTEXITCODE"
}

if (Test-Path $installerOutputAbs) {
    Remove-Item -Force $installerOutputAbs
}

Write-Host "Creating installer..."
& $BinaryCreatorPath --offline-only -c (Join-Path $installerDir "config.xml") -p $installerDir $installerOutputAbs
if ($LASTEXITCODE -ne 0) {
    throw "binarycreator.exe failed with exit code $LASTEXITCODE"
}

Write-Host "Installer created at $installerOutputAbs"
