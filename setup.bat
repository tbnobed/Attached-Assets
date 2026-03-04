@echo off
setlocal

echo.
echo =============================================
echo   Zoom ISO Capture - Setup Launcher
echo =============================================
echo.

where node >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [FAIL] Node.js is not installed or not on PATH.
    echo        Download from https://nodejs.org
    pause
    exit /b 1
)

if "%~1"=="" (
    echo Usage:
    echo   setup.bat "C:\path\to\zoom-sdk\x64"
    echo.
    echo The path should point to the x64 folder inside the extracted Zoom SDK,
    echo containing h\, lib\, and bin\ subfolders.
    echo.
    set /p SDK_PATH="Enter Zoom SDK x64 path (or press Enter to skip): "
) else (
    set "SDK_PATH=%~1"
)

if "%SDK_PATH%"=="" (
    echo.
    echo Running setup without Zoom SDK (stub mode)...
    powershell -ExecutionPolicy Bypass -File "%~dp0setup.ps1"
) else (
    echo.
    echo Running setup with SDK at: %SDK_PATH%
    powershell -ExecutionPolicy Bypass -File "%~dp0setup.ps1" -ZoomSdkPath "%SDK_PATH%"
)

if %ERRORLEVEL% equ 0 (
    echo.
    set /p LAUNCH="Launch the app now? (Y/N): "
    if /i "%LAUNCH%"=="Y" (
        echo.
        echo Starting Zoom ISO Capture...
        cd /d "%~dp0"
        npx electron . --no-sandbox --disable-gpu-sandbox
    )
)

pause
