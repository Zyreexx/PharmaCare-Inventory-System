@echo off
title PharmaCare C++ Desktop Application
color 0A
echo ========================================================
echo   Starting PharmaCare C++ Desktop Application...
echo ========================================================
echo.
echo Checking environment...
node -v >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Node.js is not installed or not in PATH.
    echo Please install Node.js from https://nodejs.org/ to launch the desktop bridge.
    pause
    exit /b
)

if not exist "Cpp Files\main.exe" (
    echo [INFO] main.exe not found in 'Cpp Files'. Compiling from main.cpp...
    pushd "Cpp Files"
    g++ -std=c++11 main.cpp -o main.exe
    if %errorlevel% neq 0 (
        echo [ERROR] Compilation failed.
        popd
        pause
        exit /b
    )
    popd
    echo [SUCCESS] main.exe compiled successfully!
)

echo Starting desktop bridge server and launching application window...
for /f "tokens=5" %%a in ('netstat -aon ^| findstr :4000 ^| findstr LISTENING') do taskkill /f /pid %%a >nul 2>&1
node desktop_ui\desktop_app.js
pause
