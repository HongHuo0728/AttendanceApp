@echo off
call "%~dp0..\build.bat" -Analyze
exit /b %errorlevel%
