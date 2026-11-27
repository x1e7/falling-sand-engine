@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   Deploying Sand2D
echo ========================================
echo.

set "BUILD_DIR=%~dp0build\release"

if not exist "%BUILD_DIR%" (
    echo ERROR: Build directory not found! Run 'cmake --preset release' first.
    pause
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config Release --target Sand2D
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

set "SRC=%BUILD_DIR%\src"
set "DST=%~dp0package"

if not exist "%SRC%\Sand2D.exe" (
    if exist "%BUILD_DIR%\Sand2D.exe" (
        set "SRC=%BUILD_DIR%"
    ) else (
        echo ERROR: Sand2D.exe not found in build output!
        pause
        exit /b 1
    )
)

if exist "%DST%" rmdir /s /q "%DST%"
mkdir "%DST%"

echo [1/4] Copying executable...
copy "%SRC%\Sand2D.exe" "%DST%\" >nul

echo [2/4] Copying DLLs...
for %%F in ("%SRC%\*.dll") do (
    echo   %%~nxF
    copy "%%F" "%DST%\" >nul
)
for %%F in ("%BUILD_DIR%\vcpkg_installed\x64-mingw-dynamic\bin\*.dll") do (
    if not exist "%DST%\%%~nxF" (
        echo   %%~nxF (from vcpkg)
        copy "%%F" "%DST%\" >nul
    )
)

echo [3/4] Copying assets...
if exist "%~dp0assets" (
    xcopy /E /I /Y "%~dp0assets" "%DST%\assets" >nul
    echo   assets\ copied
) else (
    echo   WARNING: assets folder not found next to deploy.bat
)

echo [4/4] Done!
echo ========================================
echo   Package ready in: package\
echo ========================================
pause
