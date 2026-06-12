@echo off
setlocal enabledelayedexpansion

set "DCAPP_HOME=%~dp0.."
pushd "%DCAPP_HOME%"
set "DCAPP_HOME=%CD%"
popd
set "RUN_DIR=%DCAPP_HOME%\pilotlight\out"

set "ARGS="
:argloop
if "%~1"=="" goto endargs
if "%~1"=="-h" (
    set "ARGS=!ARGS! --snapshot-help"
    shift
    goto argloop
)
if "%~1"=="--help" (
    set "ARGS=!ARGS! --snapshot-help"
    shift
    goto argloop
)
if "%~1"=="--planet-data" goto patharg
if "%~1"=="--vertex-shader" goto patharg
if "%~1"=="--fragment-shader" goto patharg
if "%~1"=="--output" goto outputarg
set "ARGS=!ARGS! %~1"
shift
goto argloop

:patharg
set "KEY=%~1"
shift
set "ARGS=!ARGS! %KEY% %~f1"
shift
goto argloop

:outputarg
shift
for %%I in ("%~f1") do if not exist "%%~dpI" mkdir "%%~dpI"
set "ARGS=!ARGS! --output %~f1"
shift
goto argloop

:endargs

cd /d "%RUN_DIR%"
echo pilot_light.exe -a dcapp-planet-snapshot%ARGS%
pilot_light.exe -a dcapp-planet-snapshot%ARGS%
