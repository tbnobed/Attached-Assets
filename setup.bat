@echo off
setlocal EnableDelayedExpansion

echo.
echo =============================================
echo   Zoom ISO Capture - Setup Launcher
echo =============================================
echo.

where node >nul 2>&1
if %ERRORLEVEL% neq 0 goto :nonode

if "%~1"=="" goto :noparam
set "SDK_PATH=%~1"
goto :runsetup

:noparam
echo The path should point to the x64 folder inside the extracted Zoom SDK.
echo.
set /p SDK_PATH="Enter Zoom SDK x64 path, or press Enter to skip: "
if "!SDK_PATH!"=="" goto :runsetup_nosdk
goto :runsetup

:runsetup_nosdk
echo.
echo Running setup without Zoom SDK...
node scripts/setup.js
goto :checklaunch

:runsetup
echo.
echo Running setup with SDK at: !SDK_PATH!
node scripts/setup.js "!SDK_PATH!"
goto :checklaunch

:checklaunch
if %ERRORLEVEL% neq 0 goto :done
echo.
set /p LAUNCH="Launch the app now? [Y/N]: "
if /i "!LAUNCH!"=="Y" goto :launch
goto :done

:launch
echo.
echo Starting Zoom ISO Capture...
cd /d "%~dp0"
npx electron . --no-sandbox --disable-gpu-sandbox
goto :done

:nonode
echo [FAIL] Node.js is not installed or not on PATH.
echo        Download from https://nodejs.org
goto :done

:done
echo.
pause
