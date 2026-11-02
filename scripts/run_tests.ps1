param([switch]$SkipBuild,[ValidateRange(1,3)][int]$Runs=3)
. (Join-Path $PSScriptRoot 'storage.ps1')
if(-not $SkipBuild){& (Join-Path $script:ProjectRoot 'build.bat') -Tests;if($LASTEXITCODE -ne 0){throw 'Test build failed.'}}
$testRoot=Assert-ManagedPath (Join-Path $script:ProjectRoot 'build\test-work')
$marker=Join-Path $testRoot '.attendance-test-root'
if(Test-Path -LiteralPath $testRoot){if(-not (Test-Path -LiteralPath $marker -PathType Leaf)){throw 'Unowned test directory; refusing cleanup.'};Remove-ManagedTree $testRoot}
[void][IO.Directory]::CreateDirectory($testRoot);[IO.File]::WriteAllText($marker,'AttendanceApp isolated tests')
$results=@();$failure=$null
try {
 for($i=1;$i -le $Runs;$i++){
  $data=Join-Path $testRoot 'data';[void][IO.Directory]::CreateDirectory($data)
  & (Join-Path $PSScriptRoot 'test_storage_safety.ps1') -DataRoot $data
  $peak=Invoke-BoundedProcess (Join-Path $script:ProjectRoot 'build\tests\Release\AttendanceTests.exe') @($data) (Join-Path $testRoot 'regression') $testRoot
  & (Join-Path $PSScriptRoot 'verify_reports.ps1') -DataRoot $data
  $snapshot=Get-TreeSnapshot $testRoot
  Assert-TestBudget $snapshot
  $results+=[pscustomobject]@{Run=$i;Passed=$true;GeneratedBytes=$snapshot.Bytes;SampledBuildPeakBytes=$peak;Largest=$snapshot.Largest;Diagnostics=@([IO.File]::ReadAllLines((Join-Path $testRoot 'regression.out.log'))|Select-Object -Last 40)}
  Write-Output "Run $i passed; test bytes: $($snapshot.Bytes)."
  Remove-ManagedTree $data
 }
}catch{$failure=$_.Exception.Message;throw}finally{
 $final=Get-TreeSnapshot $testRoot
 try {
  $summaryPath=Join-Path $script:ProjectRoot 'build\test-summary.json'
  $summaryJson=[pscustomobject]@{Runs=$results;Failure=$failure;BeforeCleanup=$final;DataBudgetBytes=20MB;SingleFileBudgetBytes=5MB}|ConvertTo-Json -Depth 8
  $summaryBytes=[Text.Encoding]::UTF8.GetByteCount($summaryJson)
  if($summaryBytes -gt 1MB){throw 'Test diagnostic summary exceeded 1 MiB; refusing oversized output.'}
  $existingBytes=if(Test-Path -LiteralPath $summaryPath){(Get-Item -LiteralPath $summaryPath).Length}else{0}
  if((Get-GeneratedBytes)-$existingBytes+$summaryBytes -ge 512MB){throw 'Test summary would exceed the generated-artifact budget.'}
  [IO.File]::WriteAllText($summaryPath,$summaryJson,[Text.UTF8Encoding]::new($false))
 } finally {Remove-ManagedTree $testRoot}
}
