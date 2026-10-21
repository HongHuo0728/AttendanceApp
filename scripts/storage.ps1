Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:ProjectRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
function Get-TreeSnapshot([string]$Path) {
 $files = @(); if(Test-Path -LiteralPath $Path){$files=@(Get-ChildItem -LiteralPath $Path -Recurse -Force -File -ErrorAction Stop)}
 $bytes=0L;foreach($file in $files){$bytes+=$file.Length}
 [pscustomobject]@{Bytes=$bytes;Files=$files.Count;Largest=@($files|Sort-Object Length -Descending|Select-Object -First 5 @{n='Path';e={$_.FullName}},Length)}
}
function Assert-ManagedPath([string]$Path) {
 $full=[IO.Path]::GetFullPath($Path)
 if(-not $full.StartsWith($script:ProjectRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw "Path outside project: $full"}
 if((Get-Item -LiteralPath $script:ProjectRoot -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Project root is a reparse point.'}
 $relative=$full.Substring($script:ProjectRoot.Length+1);$cursor=$script:ProjectRoot
 foreach($part in $relative.Split('\')){$cursor=Join-Path $cursor $part;if(Test-Path -LiteralPath $cursor){if((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Refusing linked path: $cursor"}}}
 $pending=[Collections.Generic.Stack[string]]::new();if(Test-Path -LiteralPath $full -PathType Container){$pending.Push($full)}
 while($pending.Count){foreach($item in Get-ChildItem -LiteralPath $pending.Pop() -Force){if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Refusing linked entry: $($item.FullName)"};if($item.PSIsContainer){$pending.Push($item.FullName)}}}
 return $full
}
function Remove-ManagedTree([string]$Path){$full=Assert-ManagedPath $Path;if(Test-Path -LiteralPath $full){Remove-Item -LiteralPath $full -Recurse -Force}}
function Get-GeneratedBytes {
 $total=0L;foreach($name in @('build','.vs','build-analysis','build-dev5-check')){$total+=(Get-TreeSnapshot (Join-Path $script:ProjectRoot $name)).Bytes};return $total
}
function Assert-TestBudget($Snapshot,[long]$TotalLimit=20MB,[long]$SingleLimit=5MB){
 if($Snapshot.Bytes -gt $TotalLimit -or @($Snapshot.Largest|Where-Object Length -gt $SingleLimit).Count){throw 'Test output exceeded the configured total or per-file budget.'}
}
function Invoke-BoundedProcess {
 param([string]$File,[string[]]$Arguments,[string]$LogPrefix,[string]$TestRoot='')
 if((Get-GeneratedBytes) -ge 512MB){throw 'Generated artifacts reached the 512 MiB budget.'}
 $argumentsQuoted=@($Arguments|ForEach-Object{'"'+$_+'"'})
 $process=Start-Process -FilePath $File -ArgumentList $argumentsQuoted -WorkingDirectory $script:ProjectRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput ($LogPrefix+'.out.log') -RedirectStandardError ($LogPrefix+'.err.log')
 $processHandle=$process.Handle
 $peak=0L
 try {
  do {
   $size=Get-GeneratedBytes;$peak=[Math]::Max($peak,$size)
   if($size -ge 512MB){throw 'Generated artifacts reached the 512 MiB budget.'}
   if($TestRoot){$test=Get-TreeSnapshot $TestRoot;Assert-TestBudget $test}
  } while(-not $process.WaitForExit(500))
  $process.WaitForExit()
  if($process.ExitCode -ne 0){$failureOutput=@(Get-Content -LiteralPath ($LogPrefix+'.out.log') -Tail 20;Get-Content -LiteralPath ($LogPrefix+'.err.log') -Tail 20) -join [Environment]::NewLine;throw "Process failed: $File ($($process.ExitCode))$([Environment]::NewLine)$failureOutput"}
  return $peak
 } finally {
  if(-not $process.HasExited){& taskkill.exe /PID $process.Id /T /F | Out-Null;$process.WaitForExit()}
  $process.Dispose()
 }
}
