@echo off
title PharmaCare C++ Interactive Terminal Menu
color 0F
echo ========================================================
echo   Starting PharmaCare C++ Console Application
echo ========================================================
echo.
cd /d "%~dp0Cpp Files"
if not exist "main.exe" (
    echo [INFO] Compiling main.cpp...
    g++ -std=c++11 main.cpp -o main.exe
)
.\main.exe
pause
