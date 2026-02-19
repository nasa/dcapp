@echo off
setlocal enabledelayedexpansion

set "DCAPP_HOME=%~dp0.."
pushd "%DCAPP_HOME%"
set "DCAPP_HOME=%CD%"
popd
set "RUN_DIR=%DCAPP_HOME%\pilotlight\out"

if "%~1"=="" (
    "%RUN_DIR%\dcapp-planet-chunkgen.exe" --help
    exit /b 1
)

set "INPUT=%~f1"
set "OUTPUT=%~f2"
shift
shift

set "ARGS="
:argloop
if "%~1"=="" goto endargs
set "ARGS=!ARGS! %~1"
shift
goto argloop
:endargs

REM Use PowerShell to compute relative paths
for /f "delims=" %%i in ('powershell -Command "[System.IO.Path]::GetRelativePath('%RUN_DIR%', '%INPUT%')"') do set "INPUT_REL=%%i"
for /f "delims=" %%i in ('powershell -Command "[System.IO.Path]::GetRelativePath('%RUN_DIR%', '%OUTPUT%')"') do set "OUTPUT_REL=%%i"

cd /d "%RUN_DIR%"
set "cmd=dcapp-planet-chunkgen.exe %INPUT_REL% %OUTPUT_REL%%ARGS%"
echo %cmd%
%cmd%
