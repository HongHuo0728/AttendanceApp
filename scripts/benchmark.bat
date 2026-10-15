@echo off
setlocal
set "NORMALIZED_PATH=%Path%"
set "PATH="
set "Path="
set "Path=%NORMALIZED_PATH%"
cmake -S . -B build -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release --target AttendanceBaseline AttendanceTests
if errorlevel 1 exit /b 1
build\tests\Release\AttendanceBaseline.exe build\baseline-test-data > build\baseline-benchmark.txt
if errorlevel 1 exit /b 1
build\tests\Release\AttendanceTests.exe build\test-data > build\ultra-benchmark.txt
if errorlevel 1 exit /b 1
powershell.exe -NoProfile -Command "$a=Get-Content build\baseline-benchmark.txt | Where-Object {$_ -like 'translation *'}; $b=Get-Content build\ultra-benchmark.txt | Where-Object {$_ -like 'translation *'}; if (Compare-Object $a $b) { throw 'Translation equivalence failed' }"
exit /b %errorlevel%
