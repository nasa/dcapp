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
set "FORCE=0"

:: Parse command line arguments
:ParseArgs
if "%~1"=="" goto DoneArgs
if "%~1"=="-c" (
    set "CONFIG=%~2"
    shift
    shift
    goto ParseArgs
)
if "%~1"=="-f" (
    set "FORCE=1"
    shift
    goto ParseArgs
)
if "%~1"=="-h" goto ShowHelp
if "%~1"=="--help" goto ShowHelp
echo Unknown option: %~1
exit /b 1

:ShowHelp
echo Usage: %~nx0 [-c ^<config^>] [-f]
echo   -c ^<config^>  Build configuration (debug, release)
echo                Default: release
echo   -f           Force all build stages
exit /b 0

:DoneArgs

set "PILOTLIGHT_CONFIG=%CONFIG%_experimental"
set "PILOTLIGHT_OUT=%DCAPP_HOME%\pilotlight\out"
set "PILOTLIGHT_STAMP=%PILOTLIGHT_OUT%\.dcapp-pilotlight-win32-%PILOTLIGHT_CONFIG%.stamp"
set "DCAPP_STAMP=%PILOTLIGHT_OUT%\.dcapp-win32-%CONFIG%.stamp"
set "DCAPP_HEAD="
set "PILOTLIGHT_HEAD="
set "PILOTLIGHT_DIRTY=0"
set "CLEAN_PILOTLIGHT_OUT=0"
set "BUILD_PILOTLIGHT=0"

for /f "usebackq delims=" %%H in (`git -C "%DCAPP_HOME%" rev-parse HEAD 2^>nul`) do set "DCAPP_HEAD=%%H"
for /f "usebackq delims=" %%H in (`git -C "%DCAPP_HOME%\pilotlight" rev-parse HEAD 2^>nul`) do set "PILOTLIGHT_HEAD=%%H"
git -C "%DCAPP_HOME%\pilotlight" diff --quiet HEAD -- >nul 2>nul
if errorlevel 1 set "PILOTLIGHT_DIRTY=1"

if "%FORCE%"=="1" set "CLEAN_PILOTLIGHT_OUT=1"
if defined DCAPP_HEAD (
    set "DCAPP_STAMP_HEAD="
    if exist "%DCAPP_STAMP%" set /p DCAPP_STAMP_HEAD=<"%DCAPP_STAMP%"
    if /I not "!DCAPP_STAMP_HEAD!"=="!DCAPP_HEAD!" set "CLEAN_PILOTLIGHT_OUT=1"
)

if "%CLEAN_PILOTLIGHT_OUT%"=="1" (
    echo Cleaning pilotlight outputs...
    if exist "%PILOTLIGHT_OUT%" rmdir /s /q "%PILOTLIGHT_OUT%"
    if exist "%DCAPP_HOME%\pilotlight\out-temp" rmdir /s /q "%DCAPP_HOME%\pilotlight\out-temp"
    if exist "%DCAPP_HOME%\pilotlight\out_temp" rmdir /s /q "%DCAPP_HOME%\pilotlight\out_temp"
    mkdir "%PILOTLIGHT_OUT%"
    if defined DCAPP_HEAD >"%DCAPP_STAMP%" echo !DCAPP_HEAD!
)

if "%FORCE%"=="1" set "BUILD_PILOTLIGHT=1"
if not exist "%PILOTLIGHT_OUT%\pilot_light.exe" set "BUILD_PILOTLIGHT=1"
if not defined PILOTLIGHT_HEAD set "BUILD_PILOTLIGHT=1"
if "%PILOTLIGHT_DIRTY%"=="1" set "BUILD_PILOTLIGHT=1"
if not exist "%PILOTLIGHT_STAMP%" set "BUILD_PILOTLIGHT=1"
if "%BUILD_PILOTLIGHT%"=="0" set /p STAMP_HEAD=<"%PILOTLIGHT_STAMP%"
if "%BUILD_PILOTLIGHT%"=="0" if /I not "%STAMP_HEAD%"=="%PILOTLIGHT_HEAD%" set "BUILD_PILOTLIGHT=1"

echo ========================================
echo DCAPP Build
echo ========================================
echo Configuration: %CONFIG%
echo Platform: Windows
echo ========================================

:: Step 1: Build pilotlight with _experimental suffix
echo.
if "%BUILD_PILOTLIGHT%"=="1" (
    echo [1/3] Building pilotlight...
    pushd "%DCAPP_HOME%\pilotlight\src"
    call build_win32.bat -c %PILOTLIGHT_CONFIG%
    if errorlevel 1 (
        echo Pilotlight build failed!
        popd
        exit /b 1
    )
    popd

    if defined PILOTLIGHT_HEAD (
        if not exist "%PILOTLIGHT_OUT%" mkdir "%PILOTLIGHT_OUT%"
        >"%PILOTLIGHT_STAMP%" echo %PILOTLIGHT_HEAD%
    )
) else (
    echo [1/3] Skipping pilotlight; cached %PILOTLIGHT_CONFIG% build is current.
)

:: Step 2: Build dcapp apps
echo.
echo [2/3] Building dcapp apps...
call "%DCAPP_HOME%\scripts\internal\build-apps-win32.bat" -c %CONFIG%
if errorlevel 1 (
    echo Apps build failed!
    exit /b 1
)

:: Step 3: Build dcapp samples
echo.
echo [3/3] Building dcapp samples...
call "%DCAPP_HOME%\scripts\internal\build-samples-win32.bat" -c %CONFIG%
if errorlevel 1 (
    echo Samples build failed!
    exit /b 1
)

echo.
echo ========================================
echo DCAPP Build Complete
echo ========================================
