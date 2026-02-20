@echo off
setlocal enabledelayedexpansion

set "DCAPP_HOME=%~dp0.."
pushd "%DCAPP_HOME%"
set "DCAPP_HOME=%CD%"
popd
set "RUN_DIR=%DCAPP_HOME%\pilotlight\out"

if "%~1"=="" (
    set "CONFIG=%DCAPP_HOME%\samples\welcome\welcome.xml"
) else (
    set "CONFIG=%~f1"
    shift
)

set "ARGS="
:argloop
if "%~1"=="" goto endargs
set "ARGS=!ARGS! %~1"
shift
goto argloop
:endargs

cd /d "%RUN_DIR%"
echo pilot_light.exe -a dcapp "%CONFIG%"%ARGS%
pilot_light.exe -a dcapp "%CONFIG%"%ARGS%
