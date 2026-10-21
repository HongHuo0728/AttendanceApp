param([switch]$SkipBuild,[switch]$Direct)
. (Join-Path $PSScriptRoot 'storage.ps1')
if(-not $SkipBuild){& (Join-Path $script:ProjectRoot 'build.bat');if($LASTEXITCODE -ne 0){throw 'Build failed.'}}
$version=([IO.File]::ReadAllText((Join-Path $script:ProjectRoot 'VERSION'))).Trim()
$relative=if($Direct){'build\AttendanceApp.exe'}else{'build\Release\AttendanceApp.exe'}
$exe=Get-Item -LiteralPath (Join-Path $script:ProjectRoot $relative)
if($exe.VersionInfo.FileVersion -ne $version -or $exe.VersionInfo.ProductVersion -ne $version){throw 'Executable version mismatch.'}
$parser=Start-Process -FilePath $exe.FullName -ArgumentList '--verify-parser' -WindowStyle Hidden -PassThru
$parserHandle=$parser.Handle;$parser.WaitForExit();$parserExit=$parser.ExitCode;$parser.Dispose()
if($parserExit -ne 0){throw 'Parser verification failed.'}
& (Join-Path $PSScriptRoot 'check_architecture.ps1')
if(Get-Command node.exe -ErrorAction SilentlyContinue){& node.exe (Join-Path $PSScriptRoot 'check_localization.mjs');if($LASTEXITCODE -ne 0){throw 'Localization check failed.'}}
$readme=[IO.File]::ReadAllText((Join-Path $script:ProjectRoot 'README.md'))
if($readme -match '[\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF]'){throw 'README must remain English.'}
Write-Output "Release $version verified; executable $($exe.Length) bytes. Size is reported, not compared with a removed baseline."
