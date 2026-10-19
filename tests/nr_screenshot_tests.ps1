$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$repo=Split-Path -Parent $PSScriptRoot
$root=Join-Path $repo ('build\screenshot-test-'+[guid]::NewGuid().ToString('N'))
$raw=Join-Path $root 'raw'
New-Item -ItemType Directory -Path $raw | Out-Null
$settings=@{NrEnabled=$true;NrFoveated=$true;NrSecondPass=$true;NrThirdPass=$false;NrProcessingOrder=0;NrPreset=2;NrStyle=0;NrWorkingScale=0.5;NrIntensity=0.7;NrPass2Preset=1;NrPass2Style=2;NrPass2WorkingScale=1.0;NrPass2Intensity=0.6}
@{settings=$settings} | ConvertTo-Json | Set-Content (Join-Path $root 'settings.json')
$image=New-Object Drawing.Bitmap(1024,128)
$graphics=[Drawing.Graphics]::FromImage($image)
$graphics.Clear([Drawing.Color]::FromArgb(255,20,40,80));$graphics.Dispose()
foreach($eye in @('left','right')){$image.Save((Join-Path $raw ($eye+'.bmp')),[Drawing.Imaging.ImageFormat]::Bmp)}
$image.Dispose()
$before=(Get-FileHash (Join-Path $raw 'left.bmp')).Hash
& (Join-Path $repo 'standalone\Capture-NR-Screenshot.ps1') -CaptureRoot $root -LabelOnly -PicturesRoot (Join-Path $root 'pictures-test')
foreach($eye in @('left','right')) {
    $path=Join-Path $root ($eye+'-labelled.png')
    $label=[Drawing.Bitmap]::FromFile($path)
    try {
        if($label.Width -ne 1024 -or $label.Height -le 128){throw 'Missing header or resized image'}
        $header=$label.Height-128
        for($y=$header;$y -lt $label.Height;$y++){for($x=0;$x -lt 1024;$x++){
            if($label.GetPixel($x,$y).ToArgb() -ne [Drawing.Color]::FromArgb(255,20,40,80).ToArgb()){throw 'Capture pixels modified'}
        }}
        $green=$false
        for($y=0;$y -lt $header;$y++){for($x=0;$x -lt 1024;$x++){
            $p=$label.GetPixel($x,$y);if($p.G -gt 100 -and $p.R -lt 50 -and $p.B -lt 50){$green=$true;break}
        };if($green){break}}
        if(!$green){throw 'Green labels missing'}
    } finally {$label.Dispose()}
    $copy=Join-Path $root ('pictures-test\CheekyFoveatedDLSS\'+(Split-Path -Leaf $root)+'\'+$eye+'-labelled.png')
    if((Get-FileHash $path).Hash -ne (Get-FileHash $copy).Hash){throw 'Pictures copy mismatch'}
}
if((Get-FileHash (Join-Path $raw 'left.bmp')).Hash -ne $before){throw 'Raw original changed'}
'PASS: green labels, untouched image pixels/raw BMPs, both eyes and Pictures copies'
$bad=Join-Path $root 'near-black'
New-Item -ItemType Directory -Path (Join-Path $bad 'raw') | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'settings.json') -Destination $bad
$image=New-Object Drawing.Bitmap(1024,128)
$graphics=[Drawing.Graphics]::FromImage($image);$graphics.Clear([Drawing.Color]::Black);$graphics.Dispose()
$image.SetPixel(100,100,[Drawing.Color]::Red)
foreach($eye in @('left','right')){$image.Save((Join-Path $bad ('raw\'+$eye+'.bmp')),[Drawing.Imaging.ImageFormat]::Bmp)}
$image.Dispose()
& "$PSHOME\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repo 'standalone\Capture-NR-Screenshot.ps1') -CaptureRoot $bad -LabelOnly -PicturesRoot (Join-Path $bad 'pictures-test')
if($LASTEXITCODE -ne 1){throw 'Near-black mirrors must fail'}
if(Test-Path (Join-Path $bad 'pictures-test')){throw 'Near-black capture was published to Pictures'}
if(!(Get-Content (Join-Path $bad 'capture-error.txt') -Raw).Contains('Near-black eye mirror')){throw 'Missing near-black explanation'}
'PASS: near-black HUD-only capture rejected; raw originals retained, no Pictures output'
exit 0
