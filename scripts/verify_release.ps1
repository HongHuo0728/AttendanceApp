param(
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot

function Assert-ReleaseCondition {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Read-Utf8File {
    param([string]$Path)
    return [System.IO.File]::ReadAllText($Path, [System.Text.UTF8Encoding]::new($false, $true))
}

$buildStartedAt = Get-Date
if (-not $SkipBuild) {
    & cmd.exe /d /c (Join-Path $root 'build.bat')
    Assert-ReleaseCondition ($LASTEXITCODE -eq 0) "build.bat failed with exit code $LASTEXITCODE."
}

$sourcePath = Join-Path $root 'src\main.cpp'
$hongKongPath = Join-Path $root 'include\zh_hk_strings.h'
$readmePath = Join-Path $root 'README.md'
$cmakePath = Join-Path $root 'CMakeLists.txt'
$resourcePath = Join-Path $root 'resources\app.rc'
$manifestPath = Join-Path $root 'resources\app.manifest'

$source = Read-Utf8File $sourcePath
$hongKong = Read-Utf8File $hongKongPath
$readme = Read-Utf8File $readmePath
$cmake = Read-Utf8File $cmakePath
$resource = Read-Utf8File $resourcePath
$manifest = Read-Utf8File $manifestPath

Assert-ReleaseCondition (-not [regex]::IsMatch($readme, '[\u3400-\u4DBF\u4E00-\u9FFF\uF900-\uFAFF]')) 'README.md contains Chinese characters.'
Assert-ReleaseCondition ($cmake -match 'project\(AttendanceApp VERSION 2\.0\.1 LANGUAGES CXX\)') 'CMake project version is not 2.0.1.'
Assert-ReleaseCondition ($resource -match 'FILEVERSION\s+2,0,1,0') 'FileVersion resource is not 2.0.1.'
Assert-ReleaseCondition ($resource -match 'PRODUCTVERSION\s+2,0,1,0') 'ProductVersion resource is not 2.0.1.'
Assert-ReleaseCondition ($manifest -match 'assemblyIdentity version="2\.0\.1\.0"') 'Manifest version is not 2.0.1.0.'
Assert-ReleaseCondition ($source -match 'CurrentTitleBarVisualState') 'Application title-bar colors are not defined.'
Assert-ReleaseCondition ($source -match 'state\.caption\s*=\s*COLOR_PANEL') 'The title bar does not follow the rendered application palette.'
Assert-ReleaseCondition ($source -match 'RefreshOpenAppTitleBars\(false\)') 'Theme-transition frames do not update application title bars.'
Assert-ReleaseCondition (-not ($source -match 'AppsUseLightTheme')) 'Title bars still follow the Windows color mode.'
Assert-ReleaseCondition ($source -match 'DWMWA_SYSTEMBACKDROP_TYPE') 'The system title-bar backdrop is not enabled.'
Assert-ReleaseCondition ($source -match 'RDW_FRAME\s*\|\s*RDW_NOERASE\s*\|\s*RDW_NOCHILDREN') 'Animated title-bar refresh is not limited to the non-client frame.'

$requiredKeys = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($match in [regex]::Matches($source, 'Tr\(L"((?:\\.|[^"])*)"')) {
    [void]$requiredKeys.Add($match.Groups[1].Value)
}

$translationKeys = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$duplicateKeys = [System.Collections.Generic.List[string]]::new()
$emptyKeys = [System.Collections.Generic.List[string]]::new()
foreach ($match in [regex]::Matches($hongKong, '\{L"((?:\\.|[^"])*)",\s*L"((?:\\.|[^"])*)"\}')) {
    $key = $match.Groups[1].Value
    $value = $match.Groups[2].Value
    if (-not $translationKeys.Add($key)) {
        $duplicateKeys.Add($key)
    }
    if ([string]::IsNullOrWhiteSpace($value)) {
        $emptyKeys.Add($key)
    }
}

$missingKeys = [System.Collections.Generic.List[string]]::new()
foreach ($key in $requiredKeys) {
    if (-not $translationKeys.Contains($key)) {
        $missingKeys.Add($key)
    }
}

Assert-ReleaseCondition ($duplicateKeys.Count -eq 0) ("Duplicate zh-HK translation keys: " + ($duplicateKeys -join ', '))
Assert-ReleaseCondition ($emptyKeys.Count -eq 0) ("Empty zh-HK translations: " + ($emptyKeys -join ', '))
Assert-ReleaseCondition ($missingKeys.Count -eq 0) ("Missing zh-HK translations: " + ($missingKeys -join ', '))
Assert-ReleaseCondition ($source -match 'if\s*\(g_language\s*==\s*UiLanguage::ChineseTraditionalHongKong\)[\s\S]*?TranslateHongKong') 'zh-HK is not routed through its independent dictionary.'
Assert-ReleaseCondition (-not ($source -match 'g_language\s*=\s*UiLanguage::ChineseTraditional\s*;')) 'A Traditional Chinese fallback assignment remains in the source.'

$exeCandidates = @(
    (Join-Path $root 'build\Release\AttendanceApp.exe'),
    (Join-Path $root 'build\AttendanceApp.exe')
)
$exePath = $exeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
Assert-ReleaseCondition (-not [string]::IsNullOrEmpty($exePath)) 'AttendanceApp.exe was not produced.'

$exe = Get-Item -LiteralPath $exePath
if (-not $SkipBuild) {
    Assert-ReleaseCondition ($exe.LastWriteTime -ge $buildStartedAt.AddSeconds(-2)) 'AttendanceApp.exe was not refreshed by this build.'
}
Assert-ReleaseCondition ($exe.VersionInfo.FileVersion -eq '2.0.1') 'Executable FileVersion is not 2.0.1.'
Assert-ReleaseCondition ($exe.VersionInfo.ProductVersion -eq '2.0.1') 'Executable ProductVersion is not 2.0.1.'

Write-Host "Release verification passed."
Write-Host "Executable: $exePath"
Write-Host "Hong Kong translations: $($translationKeys.Count) entries; $($requiredKeys.Count) required keys covered."
