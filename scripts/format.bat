@echo off
setlocal
set "DCAPP_CLANG_FORMAT_VERSION=21.1.8"
if not defined DCAPP_CLANG_FORMAT set "DCAPP_CLANG_FORMAT=clang-format"

"%DCAPP_CLANG_FORMAT%" --version 2>nul | findstr /C:"version %DCAPP_CLANG_FORMAT_VERSION%" >nul
if errorlevel 1 (
    echo clang-format %DCAPP_CLANG_FORMAT_VERSION% not found.
    echo Install with: winget install --id LLVM.LLVM --version %DCAPP_CLANG_FORMAT_VERSION% --exact
    exit /b 1
)

for /f "delims=" %%F in ('git -C "%~dp0.." ls-files -- "*.c" "*.h" "*.cc" "*.cpp" "*.cxx" "*.hh" "*.hpp" "*.hxx" "*.m" "*.mm" ":(exclude)tools/terrain/pl_icons.h"') do (
    "%DCAPP_CLANG_FORMAT%" -i --style=file -- "%~dp0..\%%F"
    if errorlevel 1 exit /b 1
)
