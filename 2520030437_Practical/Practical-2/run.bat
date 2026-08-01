@echo off
echo Compiling minishell using GCC in WSL...
wsl make -C 2520030437_Practical/Practical-2
if %ERRORLEVEL% neq 0 (
    echo Compilation failed!
    pause
    exit /b %ERRORLEVEL%
)
echo.
echo Compilation successful! Running minishell in WSL...
echo.
wsl ./2520030437_Practical/Practical-2/minishell
echo.
echo Program finished.
pause
