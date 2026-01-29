@echo off
setlocal enabledelayedexpansion

set "DCAPP_HOME=%~dp0"
set "DCAPP_HOME=%DCAPP_HOME:~0,-1%"
set "RUN_DIR=%DCAPP_HOME%\pilotlight\out"

if "%~1"=="" (
    echo Usage: validate.bat ^<config.xml^> [CONSTANT=value ...]
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

REM Use PowerShell to compute relative path
for /f "delims=" %%i in ('powershell -Command "[System.IO.Path]::GetRelativePath('%RUN_DIR%', '%CONFIG%')"') do set "CONFIG_REL=%%i"

cd /d "%RUN_DIR%"
set "cmd=dcapp-validate.exe %CONFIG_REL%%ARGS%"
echo %cmd%
%cmd%
