@echo off
setlocal

where cmake >nul 2>nul
if %errorlevel%==0 (
    cmake -S . -B build -A x64
    if errorlevel 1 exit /b 1
    cmake --build build --config Release
    exit /b %errorlevel%
)

where cl >nul 2>nul
if %errorlevel%==0 goto build_with_cl

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
)

if defined VSINSTALL (
    call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
    goto build_with_cl
)

echo Could not find CMake or MSVC cl.exe. Install Visual Studio Build Tools with "Desktop development with C++".
exit /b 1

:build_with_cl
if not exist build mkdir build
rc /nologo /fo build\app.res resources\app.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /utf-8 /DUNICODE /D_UNICODE /O2 /Fo:build\ /Fe:build\AttendanceApp.exe src\main.cpp build\app.res /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib ole32.lib uxtheme.lib dwmapi.lib
exit /b %errorlevel%
