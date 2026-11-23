@echo off
rem ============================================================
rem  run.bat - builds the program (if needed) and starts it.
rem  Rebuilds when sna.exe is missing, or when you pass "rebuild".
rem ============================================================
cd /d "%~dp0"

if "%1"=="rebuild" goto build
if not exist sna.exe goto build
goto start

:build
call build.bat nopause
if errorlevel 1 (
    pause
    exit /b 1
)

:start
sna.exe
echo.
pause
