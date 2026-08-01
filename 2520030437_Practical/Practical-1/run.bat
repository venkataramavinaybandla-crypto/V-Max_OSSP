@echo off
echo Compiling Case01.C using GCC in WSL...
wsl gcc -o Practical-1/Case01 Practical-1/Case01.C
if %ERRORLEVEL% neq 0 (
    echo Compilation failed!
    pause
    exit /b %ERRORLEVEL%
)
echo.
echo Compilation successful! Running Case01 in WSL...
echo (Type a command like 'ls' or 'uname -a' when prompted)
echo.
wsl ./Practical-1/Case01
echo.
echo Program finished.
pause
