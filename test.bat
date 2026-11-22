@echo off
rem ============================================================
rem  test.bat - builds and runs all six test programs.
rem  The test programs use only the core code (no Menu / Loading).
rem ============================================================
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo ERROR: g++ was not found. Install MinGW-w64 and add its "bin" folder to PATH.
    pause
    exit /b 1
)

if not exist tests\bin mkdir tests\bin

set LIB=src\Utils.cpp src\Graph.cpp src\Queue.cpp src\Algorithms.cpp src\FileManager.cpp src\Security.cpp
set FAILED=0

for %%T in (test_graph test_queue test_algorithms test_files test_security test_validation) do (
    echo.
    echo ===== %%T =====
    g++ -std=c++11 -Wall -Wextra -Isrc -o tests\bin\%%T.exe tests\%%T.cpp %LIB%
    if errorlevel 1 (
        echo COMPILE FAILED: %%T
        set FAILED=1
    ) else (
        tests\bin\%%T.exe
        if errorlevel 1 set FAILED=1
    )
)

echo.
if "%FAILED%"=="0" (
    echo ALL TEST PROGRAMS PASSED
) else (
    echo SOME TESTS FAILED - see the messages above
)
pause
exit /b %FAILED%
