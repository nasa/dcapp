@echo off
setlocal enabledelayedexpansion

:: DCAPP Build Script
:: Builds pilotlight (with _experimental suffix) and dcapp apps/samples

set "DCAPP_HOME=%~dp0"
set "DCAPP_HOME=%DCAPP_HOME:~0,-1%"

:: Default configuration
set "CONFIG=release"

:: Parse command line arguments
:ParseArgs
if "%~1"=="" goto DoneArgs
if "%~1"=="-c" (
    set "CONFIG=%~2"
    shift
    shift
    goto ParseArgs
)
if "%~1"=="-h" goto ShowHelp
if "%~1"=="--help" goto ShowHelp
echo Unknown option: %~1
exit /b 1

:ShowHelp
echo Usage: %~nx0 [-c ^<config^>]
echo   -c ^<config^>  Build configuration (debug, release)
echo                Default: debug
exit /b 0

:DoneArgs

echo ========================================
echo DCAPP Build
echo ========================================
echo Configuration: %CONFIG%
echo Platform: Windows
echo ========================================

:: Step 1: Build pilotlight with _experimental suffix
echo.
echo [1/3] Building pilotlight...
pushd "%DCAPP_HOME%\pilotlight\src"
call build_win32.bat -c %CONFIG%_experimental
if errorlevel 1 (
    echo Pilotlight build failed!
    popd
    exit /b 1
)
popd

:: Step 2: Build dcapp apps
echo.
echo [2/3] Building dcapp apps...
pushd "%DCAPP_HOME%\scripts"
call build_apps_win32.bat -c %CONFIG%
if errorlevel 1 (
    echo Apps build failed!
    popd
    exit /b 1
)

:: Step 3: Build dcapp samples
echo.
echo [3/3] Building dcapp samples...
call build_samples_win32.bat -c %CONFIG%
if errorlevel 1 (
    echo Samples build failed!
    popd
    exit /b 1
)
popd

echo.
echo ========================================
echo DCAPP Build Complete
echo ========================================
