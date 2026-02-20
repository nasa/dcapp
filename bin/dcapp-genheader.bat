@echo off
setlocal enabledelayedexpansion

set "DCAPP_HOME=%~dp0.."
pushd "%DCAPP_HOME%"
set "DCAPP_HOME=%CD%"
popd
set "RUN_DIR=%DCAPP_HOME%\pilotlight\out"

if "%~1"=="" (
    echo Usage: dcapp-genheader.bat ^<config.xml^> [output.h]
    exit /b 1
)

set "CONFIG=%~f1"
shift

set "ARGS="
:argloop
if "%~1"=="" goto endargs
set "ARGS=!ARGS! %~1"
shift
goto argloop
:endargs

cd /d "%RUN_DIR%"
echo dcapp-genheader.exe "%CONFIG%"%ARGS%
dcapp-genheader.exe "%CONFIG%"%ARGS%
