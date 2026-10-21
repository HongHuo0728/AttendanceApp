param([Parameter(Mandatory=$true)][string]$DataRoot)
Add-Type -AssemblyName System.IO.Compression.FileSystem
$file=Join-Path $DataRoot 'snapshot.pptx'
$zip=[IO.Compression.ZipFile]::OpenRead($file)
try {
 $parts=0
 foreach($entry in $zip.Entries){if($entry.FullName -notmatch '\.(xml|rels)$'){continue};if($entry.Length -gt 5MB){throw 'Report XML exceeds the test budget.'}
  $reader=[IO.StreamReader]::new($entry.Open());try{$text=$reader.ReadToEnd()}finally{$reader.Dispose()}
  $xml=[Xml.XmlDocument]::new();$xml.XmlResolver=$null;$xml.LoadXml($text);$parts++
  foreach($node in $xml.SelectNodes('//*[local-name()="ext"]')){foreach($name in @('cx','cy')){if($node.HasAttribute($name) -and [long]$node.GetAttribute($name) -lt 0){throw 'Negative report geometry.'}}}
 }
 if($parts -lt 15){throw 'Incomplete PPTX package.'}
 Write-Output "PPTX XML/relationship parts verified: $parts"
}finally{$zip.Dispose()}
