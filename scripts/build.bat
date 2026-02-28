@echo off
setlocal enabledelayedexpansion

:: DCAPP Build Script
:: Builds pilotlight (with _experimental suffix) and dcapp apps/samples

set "DCAPP_HOME=%~dp0.."
pushd "%DCAPP_HOME%"
set "DCAPP_HOME=%CD%"
popd

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
call "%DCAPP_HOME%\scripts\internal\build_apps_win32.bat" -c %CONFIG%
if errorlevel 1 (
    echo Apps build failed!
    exit /b 1
)

:: Step 3: Build dcapp samples
echo.
echo [3/3] Building dcapp samples...
call "%DCAPP_HOME%\scripts\internal\build_samples_win32.bat" -c %CONFIG%
if errorlevel 1 (
    echo Samples build failed!
    exit /b 1
)

echo.
echo ========================================
echo DCAPP Build Complete
echo ========================================
