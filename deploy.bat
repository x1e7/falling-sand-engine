@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   Deploying Sand2D
echo ========================================
echo.

set "SRC=build\release\src"
set "DST=package"

if not exist "%SRC%\Sand2D.exe" (
    echo ERROR: %SRC%\Sand2D.exe not found!
    echo Build first: cmake --build --preset release
    pause
    exit /b 1
)

if not exist "%DST%" mkdir "%DST%"

echo [1/3] Copying executable...
copy "%SRC%\Sand2D.exe" "%DST%\" >nul

echo [2/3] Copying all DLLs from %SRC%...
for %%F in ("%SRC%\*.dll") do (
    echo   %%~nxF
    copy "%%F" "%DST%\" >nul
)

echo [3/3] Copying assets...
if exist "assets" (
    xcopy /E /I /Y "assets" "%DST%\assets" >nul
    echo   assets\
) else (
    echo   WARNING: assets\ not found, font will be missing
)

echo [4/4] Copying licenses...
if exist "THIRD_PARTY_NOTICES.txt" (
    copy "THIRD_PARTY_NOTICES.txt" "%DST%\" >nul
)
if exist "LICENSE" (
    copy "LICENSE" "%DST%\" >nul
)

echo.
echo ========================================
echo   Deployment complete!
echo   Output: %DST%\
echo ========================================
echo.

dir "%DST%"

echo.
echo To run: %DST%\Sand2D.exe
pause
