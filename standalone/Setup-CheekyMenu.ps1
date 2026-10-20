# Run from the extracted Standalone upgrade beside the game executable.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$folder = Join-Path $PSScriptRoot 'CheekyFoveatedDLSS'
$destination = Join-Path $folder 'CheekyOverlay.ini'
$template = Join-Path $PSScriptRoot 'defaults\CheekyOverlay.ini'
if (!(Test-Path -LiteralPath (Join-Path $folder 'CheekyFoveatedDLSSHost.dll') -PathType Leaf)) {
    throw 'Extract the Standalone upgrade beside the game executable first. Host DLL not found.'
}
if (Test-Path -LiteralPath $destination) {
    Write-Output "Preserved existing menu settings: $destination"
    Write-Output 'Use its MenuKey value, or change the binding in Cheeky -> General.'
    return
}
# Copy without overwrite also protects a file created after the existence check.
[IO.File]::Copy($template, $destination, $false)
Write-Output "Created: $destination"
Write-Output 'Press F11 with the game window focused. General -> Change menu key changes the binding.'
