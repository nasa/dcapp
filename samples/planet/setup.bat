@echo off
setlocal enabledelayedexpansion

REM Resolve directories relative to this script (works from anywhere)
set "SAMPLE_DIR=%~dp0"
set "SAMPLE_DIR=%SAMPLE_DIR:~0,-1%"
pushd "%SAMPLE_DIR%\..\.."
set "DCAPP_HOME=%CD%"
popd

set "CACHE_DIR=%DCAPP_HOME%\cache"
set "IMG_URL=https://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_45S_100M.IMG"
set "LBL_URL=https://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_45S_100M.LBL"
set "IMG_FILE=%CACHE_DIR%\LDEM_45S_100M.IMG"
set "LBL_FILE=%CACHE_DIR%\LDEM_45S_100M.LBL"

echo ========================================
echo Planet Sample Setup
echo ========================================

if not exist "%CACHE_DIR%" mkdir "%CACHE_DIR%"

if not exist "%IMG_FILE%" (
    echo Downloading LDEM_45S_100M.IMG...
    curl -L -o "%IMG_FILE%" "%IMG_URL%"
    if errorlevel 1 ( echo ERROR: Failed to download IMG & exit /b 1 )
) else (
    echo LDEM_45S_100M.IMG already downloaded, skipping.
)

if not exist "%LBL_FILE%" (
    echo Downloading LDEM_45S_100M.LBL...
    curl -L -o "%LBL_FILE%" "%LBL_URL%"
    if errorlevel 1 ( echo ERROR: Failed to download LBL & exit /b 1 )
) else (
    echo LDEM_45S_100M.LBL already downloaded, skipping.
)

echo.
echo Running chunkgen...
call "%DCAPP_HOME%\bin\dcapp-planet-chunkgen.bat" "%LBL_FILE%" "%CACHE_DIR%" --radius 1737400
