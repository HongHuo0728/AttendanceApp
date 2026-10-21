Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    & (Join-Path $PSScriptRoot 'generate_version.ps1')
    & rc.exe /nologo /Iinclude /fo build\app.res build\generated\app.rc
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    $objectRoot = Join-Path $root 'build\objects'
    [void][IO.Directory]::CreateDirectory($objectRoot)
    $objects = @()
    foreach ($source in (Get-ChildItem -LiteralPath (Join-Path $root 'src') -Filter '*.cpp' -Recurse | Sort-Object FullName)) {
        $relative = $source.FullName.Substring($root.Length + 1)
        $object = Join-Path $objectRoot (($relative -replace '[\\/.]', '_') + '.obj')
        & cl.exe /nologo /c /std:c++17 /EHsc /MT /utf-8 /W4 /DUNICODE /D_UNICODE /O2 /Os /Gy /Gw /GL /Iinclude /Isrc /Ibuild/generated "/Fo$object" $source.FullName
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $objects += '"' + $object + '"'
    }
    $response = Join-Path $objectRoot 'link.rsp'
    $lines = $objects + @('build\app.res', '/OUT:build\AttendanceApp.exe', '/SUBSYSTEM:WINDOWS', '/MANIFEST:NO', '/LTCG', '/OPT:REF', '/OPT:ICF', '/INCREMENTAL:NO', 'user32.lib', 'gdi32.lib', 'comdlg32.lib', 'comctl32.lib', 'shell32.lib', 'ole32.lib', 'uxtheme.lib', 'dwmapi.lib', 'msimg32.lib')
    [IO.File]::WriteAllLines($response, $lines, [Text.UnicodeEncoding]::new($false, $true))
    & link.exe "@$response"
    exit $LASTEXITCODE
} finally { Pop-Location }
