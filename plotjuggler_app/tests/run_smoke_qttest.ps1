param(
    [Parameter(Mandatory = $true)]
    [string]$ExePath,
    [Parameter(Mandatory = $true)]
    [string]$BuildBinDir,
    [Parameter(Mandatory = $true)]
    [string]$VcpkgBinDir,
    [string]$QtPath = "F:\qt\5.15.2\msvc2019_64"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path $ExePath)) {
    throw "Smoke test executable not found: $ExePath"
}

if (-not (Test-Path $BuildBinDir)) {
    throw "Build bin directory not found: $BuildBinDir"
}

if (-not (Test-Path $VcpkgBinDir)) {
    throw "vcpkg bin directory not found: $VcpkgBinDir"
}

$qtBin = Join-Path $QtPath "bin"
$vcpkgRoot = Split-Path -Parent $VcpkgBinDir
$vcpkgPlugins = Join-Path $vcpkgRoot "plugins"
$localPlugins = Join-Path $BuildBinDir "plugins"
$platformDir = Join-Path $localPlugins "platforms"
$qoffscreenDll = Join-Path (Join-Path $vcpkgPlugins "platforms") "qoffscreen.dll"

if (-not (Test-Path $platformDir)) {
    New-Item -ItemType Directory -Force -Path $platformDir | Out-Null
}

if (Test-Path $qoffscreenDll) {
    Copy-Item $qoffscreenDll -Destination (Join-Path $platformDir "qoffscreen.dll") -Force
}

$env:PATH = @(
    $BuildBinDir
    $VcpkgBinDir
    $qtBin
    $env:PATH
) -join ";"

$env:QT_PLUGIN_PATH = @(
    $localPlugins
    $vcpkgPlugins
) -join ";"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = $platformDir

& $ExePath -platform offscreen
exit $LASTEXITCODE
