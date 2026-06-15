@echo off
setlocal

set "DCAPP_HOME=%~dp0.."
set "SNAPSHOT=%DCAPP_HOME%\bin\dcapp-planet-snapshot.bat"
set "PLANET_DATA=%DCAPP_HOME%\data\LDEM_45S_100M.planet.json"
set "OUT_DIR=%DCAPP_HOME%\data"

if "%~1"=="1" set "EXAMPLE=1"
if "%~1"=="2" set "EXAMPLE=2"
if "%~1"=="3" set "EXAMPLE=3"
if defined EXAMPLE goto checkdata

echo Usage: %~nx0 1^|2^|3
echo   1  geodetic Clavius
echo   2  geodetic Shackleton with elevation shader
echo   3  cartesian Clavius oblique
exit /b 1

:checkdata
if not exist "%PLANET_DATA%" (
    echo Missing planet data: "%PLANET_DATA%"
    echo Run scripts\download-planet-data.bat first, then rebuild if needed.
    exit /b 1
)

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

if "%EXAMPLE%"=="1" goto example1
if "%EXAMPLE%"=="2" goto example2
if "%EXAMPLE%"=="3" goto example3

:example1
call "%SNAPSHOT%" ^
    --planet-data "%PLANET_DATA%" ^
    --crs geodetic ^
    --attitude-frame local-ned ^
    --lat -58.62 --lon 345.27 --elevation 2000000 ^
    --yaw 0 --pitch 0 --roll 0 ^
    --width 1280 --height 720 ^
    --fov 60 ^
    --output "%OUT_DIR%\clavius-geodetic.png"
exit /b %errorlevel%

:example2
call "%SNAPSHOT%" ^
    --planet-data "%PLANET_DATA%" ^
    --crs geodetic ^
    --attitude-frame local-ned ^
    --lat -89.67 --lon 129.78 --elevation 1400000 ^
    --yaw 0 --pitch 0 --roll 0 ^
    --width 1024 --height 1024 ^
    --fov 55 ^
    --fragment-shader "%DCAPP_HOME%\samples\planet\shaders\planet_elevation.frag" ^
    --output "%OUT_DIR%\shackleton-elevation.png"
exit /b %errorlevel%

:example3
call "%SNAPSHOT%" ^
    --planet-data "%PLANET_DATA%" ^
    --crs cartesian ^
    --attitude-frame cartesian-rpy ^
    --x -494826 --y -3190740 --z 1882148 ^
    --roll 0 --pitch 52 --yaw 158 ^
    --width 1280 --height 720 ^
    --fov 60 ^
    --output "%OUT_DIR%\cartesian-oblique.png"
exit /b %errorlevel%
