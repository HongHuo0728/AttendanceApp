. (Join-Path $PSScriptRoot 'storage.ps1')
$version=([IO.File]::ReadAllText((Join-Path $script:ProjectRoot 'VERSION'))).Trim()
if($version -notmatch '^\d+\.\d+\.\d+$'){throw 'Invalid VERSION.'}
$destination=Join-Path $script:ProjectRoot 'build\generated';[void][IO.Directory]::CreateDirectory($destination)
$tokens=@{'@ATTENDANCE_VERSION@'=$version;'@ATTENDANCE_VERSION_COMMAS@'=$version.Replace('.',',');'@ATTENDANCE_SOURCE_DIR@'=$script:ProjectRoot.Replace('\','/');'@ATTENDANCE_MANIFEST_PATH@'=(Join-Path $destination 'app.manifest').Replace('\','/')}
foreach($item in @(@('include\version.h.in','version.h'),@('resources\app.rc.in','app.rc'),@('resources\app.manifest.in','app.manifest'))){$content=[IO.File]::ReadAllText((Join-Path $script:ProjectRoot $item[0]));foreach($key in $tokens.Keys){$content=$content.Replace($key,$tokens[$key])};[IO.File]::WriteAllText((Join-Path $destination $item[1]),$content,[Text.UTF8Encoding]::new($false))}
