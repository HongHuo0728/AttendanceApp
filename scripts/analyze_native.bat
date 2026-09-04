@echo off
setlocal

rem Match build.bat's environment normalization so MSBuild can launch CL.exe
rem even when the parent process supplies both Path and PATH.
set "ATTENDANCE_ANALYSIS_PATH=%Path%"
set "PATH="
set "Path="
set "Path=%ATTENDANCE_ANALYSIS_PATH%"
set "ATTENDANCE_ANALYSIS_PATH="

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake is required for native code analysis.
    exit /b 1
)

cmake -S . -B build-analysis -A x64
if errorlevel 1 exit /b 1

cmake --build build-analysis --config Release -- /t:Rebuild /p:RunCodeAnalysis=true /p:EnableMicrosoftCodeAnalysis=true /m:1
if errorlevel 1 exit /b 1

exit /b 0
