$ErrorActionPreference='Stop'
$repo=Split-Path -Parent $PSScriptRoot
$testRoot=Join-Path $repo ('build\menu-package-test-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $testRoot 'defaults') | Out-Null
Copy-Item (Join-Path $repo 'standalone\Setup-CheekyMenu.ps1') $testRoot
Copy-Item (Join-Path $repo 'standalone\CheekyOverlay.ini') (Join-Path $testRoot 'defaults')
$setup=Join-Path $testRoot 'Setup-CheekyMenu.ps1'
$rejected=$false
try { & $setup } catch { $rejected=$true }
if(!$rejected){throw 'Missing host was not rejected'}
$folder=Join-Path $testRoot 'CheekyFoveatedDLSS'
New-Item -ItemType Directory -Path $folder | Out-Null
New-Item -ItemType File -Path (Join-Path $folder 'CheekyFoveatedDLSSHost.dll') | Out-Null
& $setup
$ini=Join-Path $folder 'CheekyOverlay.ini'
if((Get-Content -Raw $ini) -notmatch '(?m)^MenuKey=122\s*$'){throw 'F11 default missing'}
[IO.File]::WriteAllText($ini,"[Overlay]`r`nMenuKey=119`r`n; preserve custom content`r`n")
$before=(Get-FileHash $ini).Hash
& $setup
if((Get-FileHash $ini).Hash -ne $before){throw 'Existing settings changed'}
$label='menu-check-'+[guid]::NewGuid().ToString('N')
& (Join-Path $repo 'scripts\package-nr-upgrade.ps1') -Label $label
foreach($mode in @('Standalone','UEVR')) {
    $zip=[IO.Compression.ZipFile]::OpenRead((Join-Path $repo "bin\Release\Cheeky-0.5.5-$mode-Upgrade-$label.zip"))
    try {
        $names=@($zip.Entries | ForEach-Object {$_.FullName.Replace('\','/')})
        if($names -contains 'CheekyFoveatedDLSS/CheekyOverlay.ini'){throw 'Upgrade would overwrite active menu INI'}
        foreach($name in @('defaults/CheekyOverlay.ini','Setup-CheekyMenu.ps1','OVERLAY-MENU.txt','CYBERPUNK-STANDALONE.txt')){
            if(($names -contains $name) -ne ($mode -eq 'Standalone')){throw "Wrong $mode package entry: $name"}
        }
    } finally {$zip.Dispose()}
}
"PASS: missing host rejected; F11 INI created; existing INI preserved byte-for-byte; ZIP paths and hashes verified. Fixtures: $testRoot"
