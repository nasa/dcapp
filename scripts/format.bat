@echo off
setlocal
if not defined DCAPP_CLANG_FORMAT set "DCAPP_CLANG_FORMAT=clang-format"

for /f "delims=" %%F in ('git -C "%~dp0.." ls-files -- "*.c" "*.h" "*.cc" "*.cpp" "*.cxx" "*.hh" "*.hpp" "*.hxx" "*.m" "*.mm" ":(exclude)tools/terrain/pl_icons.h"') do (
    "%DCAPP_CLANG_FORMAT%" -i --style=file -- "%~dp0..\%%F"
    if errorlevel 1 exit /b 1
)
