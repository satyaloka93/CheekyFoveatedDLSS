[CmdletBinding()]
param([string]$GameBin='D:\SteamLibrary\steamapps\common\Cyberpunk 2077\bin\x64')
$ErrorActionPreference='Stop'
if(Get-Process Cyberpunk2077 -ErrorAction SilentlyContinue){throw 'Close Cyberpunk first; no files replaced.'}
$target=Join-Path $GameBin 'CheekyFoveatedDLSS'
$names=@('CheekyFoveatedDLSSHost.dll','CheekyFoveatedDLSSRuntime.dll')
foreach($n in $names){if(!(Test-Path -LiteralPath (Join-Path $target $n))){throw 'Existing standalone install required. See START-HERE.txt.'}}
$manifest=Get-Content (Join-Path $PSScriptRoot 'SHA256SUMS.txt')
foreach($n in $names){
 $src=Join-Path $PSScriptRoot "CheekyFoveatedDLSS\$n"
 $expected=@($manifest | Where-Object {$_ -like "*  CheekyFoveatedDLSS/$n"})
 if($expected.Count -ne 1 -or (Get-FileHash $src).Hash -ne $expected[0].Split(' ')[0]){throw "Payload hash failed: $n"}
}
$protected=@(Get-ChildItem $target -File | Where-Object {$_.Extension -eq '.ini' -or $_.Name -like 'nvngx*'})
foreach($n in @('dxgi.dll','dxgi2.dll','version.dll','winmm.dll')){$p=Join-Path $GameBin $n;if(Test-Path $p){$protected+=Get-Item $p}}
$hashes=@($protected | Get-FileHash)
$backup=Join-Path $GameBin ('Cheeky-backups\RC5-test-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory $backup -Force | Out-Null
$hashes | ConvertTo-Json | Set-Content (Join-Path $backup 'preserved-hashes.json')
foreach($n in $names){Copy-Item (Join-Path $target $n) (Join-Path $backup $n)}
try {
 foreach($n in $names){
  $src=Join-Path $PSScriptRoot "CheekyFoveatedDLSS\$n";$dst=Join-Path $target $n
  Copy-Item $src $dst -Force
  if((Get-FileHash $src).Hash -ne (Get-FileHash $dst).Hash){throw "Copy verification failed: $n"}
 }
 foreach($h in $hashes){if((Get-FileHash $h.Path).Hash -ne $h.Hash){throw "Protected file changed: $($h.Path)"}}
} catch {
 foreach($n in $names){Copy-Item (Join-Path $backup $n) (Join-Path $target $n) -Force}
 throw
}
Write-Output "Installed host/runtime only. Backup: $backup"
Write-Output 'Loaders, NVIDIA DLLs and settings preserved. See START-HERE.txt; headset test still required.'
