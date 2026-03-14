param(
    [string]$QtPath = "F:\qt\5.15.2\msvc2019_64",
    [string]$VcpkgRoot = "F:\packages\vcpkg",
    [string]$PreferredVsVersion = "[17.0,19.0)"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-VsWherePath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "vswhere.exe not found at $vswhere"
    }
    return $vswhere
}

function Get-VcEnvironmentScriptPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$VersionRange
    )

    $vswhere = Get-VsWherePath
    $args = @("-products", "*", "-version", $VersionRange, "-format", "json")
    $instanceJson = & $vswhere $args
    $instances = $instanceJson | ConvertFrom-Json

    if ($instances -is [array]) {
        $candidates = $instances |
            Where-Object { $_.isComplete -and $_.isLaunchable } |
            Sort-Object installationVersion -Descending
    } elseif ($instances) {
        $candidates = @($instances)
    } else {
        $candidates = @()
    }

    if (-not $candidates) {
        throw "No Visual Studio instance with C++ tools found for version range $VersionRange"
    }

    foreach ($instance in $candidates) {
        $vcVars = Join-Path $instance.installationPath "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcVars) {
            return $vcVars
        }

        $devCmd = Join-Path $instance.installationPath "Common7\Tools\VsDevCmd.bat"
        if (Test-Path $devCmd) {
            return $devCmd
        }
    }

    throw "Neither vcvars64.bat nor VsDevCmd.bat was found in any Visual Studio installation matching $VersionRange"
}

function Import-BatchEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BatchFile,
        [string[]]$Arguments = @()
    )

    $argString = $Arguments -join " "
    $tempFile = Join-Path $env:TEMP ("plotjuggler-msvc-" + [guid]::NewGuid().ToString() + ".cmd")
    $batchContent = "@echo off`r`ncall `"$BatchFile`" $argString`r`nset`r`n"
    Set-Content -Path $tempFile -Value $batchContent -Encoding ASCII

    try {
        $output = & cmd.exe /s /c $tempFile
    }
    finally {
        if (Test-Path $tempFile) {
            Remove-Item -Force $tempFile
        }
    }

    foreach ($line in $output) {
        if ($line -match "^(.*?)=(.*)$") {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }
}

function Remove-PathEntries {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Patterns
    )

    $entries = ($env:PATH -split ";") | Where-Object { $_ }
    $filtered = foreach ($entry in $entries) {
        $skip = $false
        foreach ($pattern in $Patterns) {
            if ($entry -like $pattern) {
                $skip = $true
                break
            }
        }
        if (-not $skip) {
            $entry
        }
    }
    $env:PATH = ($filtered | Select-Object -Unique) -join ";"
}

function Prepend-PathEntries {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Entries
    )

    $current = ($env:PATH -split ";") | Where-Object { $_ }
    $combined = @($Entries + $current) | Where-Object { $_ } | Select-Object -Unique
    $env:PATH = $combined -join ";"
}

if (-not (Test-Path $QtPath)) {
    throw "Qt path not found: $QtPath"
}

if (-not (Test-Path $VcpkgRoot)) {
    throw "vcpkg root not found: $VcpkgRoot"
}

$vcEnvScript = Get-VcEnvironmentScriptPath -VersionRange $PreferredVsVersion
$vcEnvArgs = @()
if ($vcEnvScript -like "*VsDevCmd.bat") {
    $vcEnvArgs = @("-arch=x64", "-host_arch=x64")
}
Import-BatchEnvironment -BatchFile $vcEnvScript -Arguments $vcEnvArgs

Remove-PathEntries -Patterns @(
    "*\WinLibs\*",
    "*\mingw*\bin*",
    "*\msys64\*",
    "*\cygwin*\bin*"
)

$qtBin = Join-Path $QtPath "bin"
$vsInstallRoot = Split-Path -Parent (Split-Path -Parent $vcEnvScript)
if ($vcEnvScript -like "*vcvars64.bat") {
    $vsInstallRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $vcEnvScript)))
}
$vsCMake = Join-Path $vsInstallRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"

Prepend-PathEntries -Entries @(
    $vsCMake,
    $qtBin
)

$env:VCPKG_ROOT = $VcpkgRoot
$env:CMAKE_TOOLCHAIN_FILE = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
$env:CMAKE_PREFIX_PATH = $QtPath
$env:Qt5_DIR = Join-Path $QtPath "lib\cmake\Qt5"
$env:PKG_CONFIG_PATH = ""

Write-Host "Using: $vcEnvScript"
Write-Host "MSVC environment loaded for PlotJuggler."
Write-Host "Qt: $QtPath"
Write-Host "vcpkg: $VcpkgRoot"
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    throw "MSVC compiler was not loaded correctly."
}
Write-Host "cl: $((Get-Command cl).Source)"
Write-Host "cmake: $((Get-Command cmake).Source)"
