param([Parameter(Mandatory=$true)][string]$DataRoot)
. (Join-Path $PSScriptRoot 'storage.ps1')
$DataRoot=Assert-ManagedPath ([IO.Path]::GetFullPath($DataRoot))
function Assert-Rejected([scriptblock]$Action,[string]$Label){$rejected=$false;try{& $Action}catch{$rejected=$true};if(-not $rejected){throw "Safety check did not reject: $Label"}}
Assert-Rejected {Assert-ManagedPath (Join-Path $script:ProjectRoot '..\outside-project')|Out-Null} 'path traversal'
$folder=Join-Path $DataRoot 'safety';[void][IO.Directory]::CreateDirectory($folder)
[IO.File]::WriteAllText((Join-Path $folder 'tiny.txt'),('X'*32))
$snapshot=Get-TreeSnapshot $folder
Assert-Rejected {Assert-TestBudget $snapshot -TotalLimit 16 -SingleLimit 64} 'combined budget'
Assert-Rejected {Assert-TestBudget $snapshot -TotalLimit 64 -SingleLimit 16} 'single-file budget'
$target=Join-Path $folder 'target';[void][IO.Directory]::CreateDirectory($target)
$proof=Join-Path $target 'proof.txt';[IO.File]::WriteAllText($proof,'preserved')
$link=Join-Path $folder 'junction'
try {
 New-Item -ItemType Junction -Path $link -Target $target | Out-Null
 Assert-Rejected {Assert-ManagedPath $folder|Out-Null} 'junction traversal'
}finally {
 if(Test-Path -LiteralPath $link){$item=Get-Item -LiteralPath $link -Force;if(-not ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Unexpected non-link safety fixture.'};[IO.Directory]::Delete($link)}
}
if([IO.File]::ReadAllText($proof) -ne 'preserved'){throw 'Link target was modified.'}
Remove-ManagedTree $folder
Write-Output 'Storage safety: traversal, junction and tiny injected budget failures passed.'
