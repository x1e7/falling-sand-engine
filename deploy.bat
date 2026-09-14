@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   Deploying Sand2D
echo ========================================
echo.

if not exist package mkdir package

if exist build\src\Sand2D.exe (
    echo [1/3] Copying executable...
    copy build\src\Sand2D.exe package\ >nul
) else (
    echo ERROR: build\src\Sand2D.exe not found!
    pause
    exit /b 1
)

echo [2/3] Copying MinGW dependencies...

set "MINGW_PATH=C:\msys64\ucrt64\bin"

set "DLL1=libgcc_s_seh-1.dll"
set "DLL2=libwinpthread-1.dll"
set "DLL3=libstdc++-6.dll"

if exist "%MINGW_PATH%\%DLL1%" (
    echo   Copying %DLL1%...
    copy "%MINGW_PATH%\%DLL1%" package\ >nul
) else (
    echo   WARNING: %DLL1% not found!
)

if exist "%MINGW_PATH%\%DLL2%" (
    echo   Copying %DLL2%...
    copy "%MINGW_PATH%\%DLL2%" package\ >nul
) else (
    echo   WARNING: %DLL2% not found!
)

if exist "%MINGW_PATH%\%DLL3%" (
    echo   Copying %DLL3%...
    copy "%MINGW_PATH%\%DLL3%" package\ >nul
) else (
    echo   WARNING: %DLL3% not found!
)

echo [3/3] Copying SDL3.dll...
if exist build\src\SDL3.dll (
    copy build\src\SDL3.dll package\ >nul
    echo   Copied SDL3.dll
) else (
    echo   WARNING: SDL3.dll not found!
)

echo.
echo ========================================
echo   Deployment complete!
echo   Output: package\
echo.

dir package

echo.
echo To run: package\Sand2D.exe
pause
