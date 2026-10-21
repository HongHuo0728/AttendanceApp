Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$sourceRoot = Join-Path $root 'src'
$files = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -File | Where-Object Extension -in '.cpp', '.h')
$graph = @{}
$includeCount = 0
foreach ($file in $files) {
    $dependencies = @()
    $content = [IO.File]::ReadAllText($file.FullName)
    foreach ($match in [regex]::Matches($content, '(?m)^\s*#\s*include\s+"([^"]+)"')) {
        ++$includeCount
        $include = $match.Groups[1].Value
        if ($include.EndsWith('.cpp')) { throw "Implementation include in $($file.FullName): $include" }
        $target = @((Join-Path $file.DirectoryName $include), (Join-Path $sourceRoot $include), (Join-Path (Join-Path $root 'include') $include), (Join-Path (Join-Path $root 'build\generated') $include)) |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
        if (-not $target) { throw "Missing local include in $($file.FullName): $include" }
        $dependencies += [IO.Path]::GetFullPath($target)
    }
    $graph[$file.FullName] = $dependencies
}
if ($includeCount -eq 0) { throw 'No local includes were found; dependency validation did not run.' }
$visiting = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$visited = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
function Visit-Dependency([string]$path) {
    if ($visited.Contains($path) -or -not $graph.ContainsKey($path)) { return }
    if (-not $visiting.Add($path)) { throw "Header include cycle at $path" }
    foreach ($dependency in $graph[$path]) { Visit-Dependency $dependency }
    [void]$visiting.Remove($path)
    [void]$visited.Add($path)
}
foreach ($file in $files) { Visit-Dependency $file.FullName }
$sourceCount = @($files | Where-Object Extension -eq '.cpp').Count
$headerCount = @($files | Where-Object Extension -eq '.h').Count
Write-Output "Verified $sourceCount independent implementation files, $headerCount headers and $includeCount local includes; local includes resolve and the header graph is acyclic."
