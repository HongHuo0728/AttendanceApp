@echo off
setlocal

set "ROOT=%~dp0"
set "EXE=%ROOT%build\AttendanceApp.exe"
set "ICON=%ROOT%assets\AttdFile.ico"

if not exist "%EXE%" (
    echo AttendanceApp.exe was not found. Run build.bat first.
    exit /b 1
)

if not exist "%ICON%" (
    echo AttdFile.ico was not found.
    exit /b 1
)

reg add "HKCU\Software\Classes\.attd" /ve /d "AttendanceApp.attd" /f
reg add "HKCU\Software\Classes\AttendanceApp.attd" /ve /d "Attendance File" /f
reg add "HKCU\Software\Classes\AttendanceApp.attd\DefaultIcon" /ve /d "\"%ICON%\"" /f
reg add "HKCU\Software\Classes\AttendanceApp.attd\shell\open\command" /ve /d "\"%EXE%\" \"%%1\"" /f

ie4uinit.exe -show
echo .attd file icon registration complete.
