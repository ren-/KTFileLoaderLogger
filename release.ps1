# One zip per profile: dist\<profile>\ktfl.asi + README + LICENSE.
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$ver = (Select-String -Path src\dllmain.cpp -Pattern 'KTFL_VERSION "([^"]+)"').Matches[0].Groups[1].Value
New-Item -ItemType Directory -Force release | Out-Null
foreach ($asi in Get-ChildItem dist\*\ktfl.asi) {
    $name = $asi.Directory.Name
    $dir = "release\$name"
    Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force $dir | Out-Null
    Copy-Item $asi, README.md, LICENSE $dir
    $zip = "release\KTFileLoaderLogger-$name-v$ver.zip"
    Remove-Item $zip -ErrorAction SilentlyContinue
    Compress-Archive -Path "$dir\*" -DestinationPath $zip
    Write-Host "wrote $zip"
}
