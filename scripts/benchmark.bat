@echo off
rem Optional small in-memory timings; no historical baseline or copied source tree.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_tests.ps1" -Runs 1
exit /b %errorlevel%
