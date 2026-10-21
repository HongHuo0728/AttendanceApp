param([switch]$Execute,[ValidateSet('Legacy','Intermediates','All')][string]$Mode='Legacy')
. (Join-Path $PSScriptRoot 'storage.ps1')
$targets=switch($Mode){
 'Legacy'{@('.vs','.codex\backups','build-dev5-check','build-analysis','build\baseline-2.0.3-ultra','build\localization-before-packing.cpp','build\Release\AttendanceApp.v2.0.4','build\Release\AttendanceApp.v2.0.4 (Ultra Optimization).zip')}
 'Intermediates'{@('.vs','build\CMakeFiles','build\objects','build\lib','build\AttendanceRuntime.dir','build\AttendanceApp.dir','build\AttendanceTests.dir')}
 'All'{@('.vs','.codex\backups','build-dev5-check','build-analysis','build')}
}
foreach($relative in $targets){$target=Assert-ManagedPath (Join-Path $script:ProjectRoot $relative);if(Test-Path -LiteralPath $target){$size=if(Test-Path -LiteralPath $target -PathType Leaf){(Get-Item -LiteralPath $target).Length}else{(Get-TreeSnapshot $target).Bytes};Write-Output "$Mode : $relative ($size bytes)";if($Execute){Remove-ManagedTree $target}}}
if(-not $Execute){Write-Output 'Preview only. Add -Execute to remove these generated artifacts.'}
