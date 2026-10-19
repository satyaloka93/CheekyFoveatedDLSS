$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build\nr-vit-tests'
New-Item -ItemType Directory -Force $out | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dev=Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$mh=Join-Path $root 'third_party\reshade\deps\minhook'
$objects=Join-Path $root 'build\msbuild\CheekyFoveatedDLSSRuntime\Release'
$cmd='call "'+$dev+'" -arch=x64 >nul && cl /nologo /std:c++20 /EHsc /O2 /I"'+$mh+'\include" "'+$root+'\tests\nr_vit_gpu_tests.cpp" "'+$objects+'\buffer.obj" "'+$objects+'\hook.obj" "'+$objects+'\trampoline.obj" "'+$objects+'\hde64.obj" /Fe:nr-vit-gpu-tests.exe /link d3d12.lib dxgi.lib'
Push-Location $out
try {
    & cmd.exe /d /s /c $cmd
    if($LASTEXITCODE -ne 0){throw 'F5 GPU test build failed'}
    & '.\nr-vit-gpu-tests.exe'
    if($LASTEXITCODE -ne 0){throw 'F5 GPU test failed'}
} finally {Pop-Location}
