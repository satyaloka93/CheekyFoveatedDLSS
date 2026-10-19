param([string]$Runtime='C:\Windows\System32\DriverStore\FileRepository\nvmdi.inf_amd64_d9379a506a4c3691\_nvngx.dll')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build\ngx-parameter-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dev=Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$cmd='call "'+$dev+'" -arch=x64 >nul && cl /nologo /std:c++20 /EHsc /O2 "'+$root+'\tests\ngx_parameter_probe.cpp" /Fe:ngx-parameter-probe.exe'
Push-Location $out
try {
    & cmd.exe /d /s /c $cmd
    if($LASTEXITCODE -ne 0){throw 'Probe build failed'}
    & '.\ngx-parameter-probe.exe' $Runtime
    if($LASTEXITCODE -ne 0){throw 'Probe failed'}
} finally {Pop-Location}
