@echo off

:: keep environment variables modifications local
@setlocal

:: make script directory CWD
@pushd %~dp0

:: app artifacts
@if exist "pilotlight\out" rmdir /s /q "pilotlight\out"
@if exist "pilotlight\out-temp" rmdir /s /q "pilotlight\out-temp"
@if exist "pilotlight\shader-temp" rmdir /s /q "pilotlight\shader-temp"
@if exist "pilotlight\cache" rmdir /s /q "pilotlight\cache"

:: sample artifacts
@for /d %%d in ("samples\*") do @(
    @if exist "%%d\logic\logic.dll" del "%%d\logic\logic.dll"
    @if exist "%%d\logic\logic_*.pdb" del "%%d\logic\logic_*.pdb"
)

@popd
@endlocal
