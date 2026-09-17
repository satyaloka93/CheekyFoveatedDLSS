[CmdletBinding()]
param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$testRoot = Join-Path (Split-Path -Parent $PSScriptRoot) "bin\$Configuration"
& (Join-Path $testRoot 'CheekyUEVRTests.exe') --addon-bridge
if ($LASTEXITCODE -ne 0) { throw 'UEVR ReShade bridge failed' }
foreach ($mode in @('--afw-core', '--d3d12-history', '--streamline-stereo', '--cyberpunk-gaze', '--present-lock')) {
    & (Join-Path $testRoot 'CheekyTests.exe') $mode
    if ($LASTEXITCODE -ne 0) { throw "Focused regression failed: $mode" }
}
Write-Host 'AFW ReShade focused regressions passed. This is not the complete standalone UEVR NR suite.'
