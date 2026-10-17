@echo off
rem Launcher for push_to_github.ps1
rem -ExecutionPolicy Bypass applies to THIS process only; no system setting is changed.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0push_to_github.ps1" %*
set RC=%ERRORLEVEL%
if not "%RC%"=="0" echo. & echo [launcher] script exited with code %RC%
pause
exit /b %RC%