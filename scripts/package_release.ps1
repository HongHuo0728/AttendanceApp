param([switch]$Direct)
. (Join-Path $PSScriptRoot 'storage.ps1')
& (Join-Path $PSScriptRoot 'verify_release.ps1') -SkipBuild -Direct:$Direct
$version=([IO.File]::ReadAllText((Join-Path $script:ProjectRoot 'VERSION'))).Trim()
$exe=if($Direct){'build\AttendanceApp.exe'}else{'build\Release\AttendanceApp.exe'}
$packageRoot=Join-Path $script:ProjectRoot 'build\package'
$stage=Join-Path $packageRoot ('AttendanceApp.v'+$version)
Remove-ManagedTree $stage
[void][IO.Directory]::CreateDirectory($stage)
Copy-Item -LiteralPath (Join-Path $script:ProjectRoot $exe) -Destination (Join-Path $stage 'AttendanceApp.exe')
Copy-Item -LiteralPath (Join-Path $script:ProjectRoot 'LICENSE') -Destination $stage
$quickStart=([IO.File]::ReadAllText((Join-Path $script:ProjectRoot 'docs\USAGE.md'))).Replace('examples/sample_roster.csv','sample_roster.csv')
[IO.File]::WriteAllText((Join-Path $stage 'QUICK_START.md'),$quickStart,[Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath (Join-Path $script:ProjectRoot 'examples\sample_roster.csv') -Destination $stage
Copy-Item -LiteralPath (Join-Path $script:ProjectRoot 'register_attd_filetype.bat') -Destination $stage
$zip=Join-Path $packageRoot ('AttendanceApp.v'+$version+'.zip')
Compress-Archive -LiteralPath $stage -DestinationPath $zip -Force
if((Get-GeneratedBytes) -ge 512MB){throw 'Package exceeded generated-artifact budget.'}
Write-Output "Package: $zip"
