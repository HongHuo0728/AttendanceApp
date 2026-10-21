@echo off
setlocal
set "ATTENDANCE_BUILD_PATH=%Path%"
set "PATH="
set "Path="
set "Path=%ATTENDANCE_BUILD_PATH%"
set "ATTENDANCE_BUILD_PATH="
pushd "%~dp0"
where cl >nul 2>nul
if %errorlevel%==0 goto configured
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "ATTENDANCE_VS=%%i"
if defined ATTENDANCE_VS call "%ATTENDANCE_VS%\VC\Auxiliary\Build\vcvars64.bat"
:configured
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 %*
set "ATTENDANCE_EXIT=%errorlevel%"
popd
exit /b %ATTENDANCE_EXIT%
