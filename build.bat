@echo off
rem ============================================================
rem  build.bat - compiles the Social Network Analyzer into sna.exe
rem  Double-click it, or run it from a terminal in this folder.
rem  (run.bat calls this file with the word "nopause")
rem ============================================================
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo ERROR: g++ was not found. Install MinGW-w64 and add its "bin" folder to PATH.
    if not "%1"=="nopause" pause
    exit /b 1
)

echo Building sna.exe ...
g++ -std=c++11 -Wall -Wextra -Isrc -o sna.exe src\Utils.cpp src\Graph.cpp src\Queue.cpp src\Algorithms.cpp src\FileManager.cpp src\Security.cpp src\Menu.cpp src\Loading.cpp src\main.cpp
if errorlevel 1 (
    echo.
    echo BUILD FAILED - read the error messages above.
    if not "%1"=="nopause" pause
    exit /b 1
)

echo Build OK: sna.exe
if not "%1"=="nopause" pause
exit /b 0
