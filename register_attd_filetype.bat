@echo off
setlocal
set "EXE=%~dp0AttendanceApp.exe"
if not exist "%EXE%" set "EXE=%~dp0build\Release\AttendanceApp.exe"
if not exist "%EXE%" set "EXE=%~dp0build\AttendanceApp.exe"
if not exist "%EXE%" (
 echo AttendanceApp.exe not found. Build or extract the full release first.
 exit /b 1
)
if /I "%~1"=="--check" (
 echo Executable: "%EXE%"
 echo Icon: embedded file icon, index 1
 exit /b 0
)
reg add "HKCU\Software\Classes\.attd" /ve /d "AttendanceApp.attd" /f
if errorlevel 1 exit /b 1
reg add "HKCU\Software\Classes\AttendanceApp.attd" /ve /d "Attendance File" /f
reg add "HKCU\Software\Classes\AttendanceApp.attd\DefaultIcon" /ve /d "\"%EXE%\",1" /f
reg add "HKCU\Software\Classes\AttendanceApp.attd\shell\open\command" /ve /d "\"%EXE%\" \"%%1\"" /f
if errorlevel 1 exit /b 1
ie4uinit.exe -show
echo File association registered for the current user.
