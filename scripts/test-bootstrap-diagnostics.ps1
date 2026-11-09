$ErrorActionPreference='Stop'
$bin=Join-Path (Split-Path -Parent $PSScriptRoot) 'bin\Release'
foreach($mode in @('proxy','version','asi','missing','chain','broken-chain','loop-chain','device-chain')) {
 $dll=if($mode -eq 'version'){Join-Path $bin 'version-loader\version.dll'}else{Join-Path $bin 'standalone-loader\dxgi.dll'}
 Write-Host "TEST $mode"
 & (Join-Path $bin 'CheekyBootstrapTests.exe') $mode $dll (Join-Path $bin 'CheekyFoveatedDLSS.asi') (Join-Path $bin 'CheekyBootstrapFakeHost.dll')
 if($LASTEXITCODE -ne 0){throw "Failed: $mode"}
}
