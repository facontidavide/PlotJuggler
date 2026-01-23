@echo off
REM Deploy Qt dependencies for PlotJuggler and all its plugins
REM This script runs windeployqt on the main executable and all plugin DLLs
REM
REM Usage: windeploy_pj.bat [path_to_windeployqt.exe]
REM   If windeployqt.exe is not in PATH, provide the full path as argument
REM   Example: windeploy_pj.bat C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe

setlocal enabledelayedexpansion

set DATA_DIR=%~dp0io.plotjuggler.application\data

REM Use provided windeployqt path or default to PATH
if "%~1"=="" (
    set WINDEPLOYQT=windeployqt.exe
) else (
    set WINDEPLOYQT=%~1
)

echo Deploying Qt dependencies for PlotJuggler...
echo Using: %WINDEPLOYQT%

REM Deploy for main executable
echo Processing: plotjuggler.exe
"%WINDEPLOYQT%" --release "%DATA_DIR%\plotjuggler.exe"

REM Deploy for all plugin DLLs
for %%f in ("%DATA_DIR%\*.dll") do (
    echo Processing: %%~nxf
    "%WINDEPLOYQT%" --release "%%f"
)

echo Done.
