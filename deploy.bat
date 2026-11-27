@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   Deploying Sand2D
echo ========================================
echo.

cmake --build --preset release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

set "SRC=build\release\src"
set "DST=package"

if not exist "%SRC%\Sand2D.exe" (
    if exist "%SRC%\Release\Sand2D.exe" (
        set "SRC=%SRC%\Release"
    ) else (
        echo ERROR: Sand2D.exe not found in %SRC% or %SRC%\Release!
        echo Please check your build output directory.
        pause
        exit /b 1
    )
)

if not exist "%DST%" mkdir "%DST%"

echo [1/3] Copying executable...
copy "%SRC%\Sand2D.exe" "%DST%\" >nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to copy Sand2D.exe!
    pause
    exit /b 1
)

echo [2/3] Copying all DLLs from %SRC%...
set "DLL_COUNT=0"
for %%F in ("%SRC%\*.dll") do (
    echo   %%~nxF
    copy "%%F" "%DST%\" >nul
    set /a DLL_COUNT+=1
)
if !DLL_COUNT! EQU 0 (
    echo   WARNING: No DLLs found in %SRC%. If the app crashes on startup, check runtime dependencies.
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
