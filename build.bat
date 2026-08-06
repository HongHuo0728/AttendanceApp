@echo off
setlocal

rem Some launchers can provide both Path and PATH. MSBuild treats those as
rem duplicate environment keys and refuses to start CL.exe.
set "ATTENDANCE_BUILD_PATH=%Path%"
set "PATH="
set "Path="
set "Path=%ATTENDANCE_BUILD_PATH%"
set "ATTENDANCE_BUILD_PATH="

where cmake >nul 2>nul
if %errorlevel%==0 (
    if exist build\Release\AttendanceApp.exe del /q build\Release\AttendanceApp.exe
    cmake -S . -B build -A x64
    if errorlevel 1 goto build_failed
    cmake --build build --config Release
    if errorlevel 1 goto build_failed
    if not exist build\Release\AttendanceApp.exe goto build_failed
    powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$v=(Get-Item -LiteralPath 'build\Release\AttendanceApp.exe').VersionInfo; if ($v.FileVersion -ne '2.0.0' -or $v.ProductVersion -ne '2.0.0') { exit 1 }"
    if errorlevel 1 goto build_failed
    goto build_succeeded
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
goto build_failed

:build_with_cl
if not exist build mkdir build
if exist build\AttendanceApp.exe del /q build\AttendanceApp.exe
rc /nologo /fo build\app.res resources\app.rc
if errorlevel 1 goto build_failed
cl /nologo /std:c++17 /EHsc /utf-8 /DUNICODE /D_UNICODE /O2 /Fo:build\ /Fe:build\AttendanceApp.exe src\main.cpp build\app.res /link /SUBSYSTEM:WINDOWS /MANIFEST:NO user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib ole32.lib uxtheme.lib dwmapi.lib msimg32.lib
if errorlevel 1 goto build_failed
if not exist build\AttendanceApp.exe goto build_failed
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$v=(Get-Item -LiteralPath 'build\AttendanceApp.exe').VersionInfo; if ($v.FileVersion -ne '2.0.0' -or $v.ProductVersion -ne '2.0.0') { exit 1 }"
if errorlevel 1 goto build_failed

:build_succeeded
exit /b 0

:build_failed
echo AttendanceApp build failed.
exit /b 1
