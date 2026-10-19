param([Parameter(Mandatory=$true)][string]$CaptureRoot,
      [int]$GameProcessId, [switch]$LabelOnly, [string]$PicturesRoot)
$ErrorActionPreference='Stop'
try {
    $CaptureRoot=(Resolve-Path -LiteralPath $CaptureRoot).Path
    $metadata=Join-Path $CaptureRoot 'settings.json'
    $settings=(Get-Content -LiteralPath $metadata -Raw | ConvertFrom-Json).settings
    if(!$settings){throw 'Missing settings snapshot'}
    $raw=Join-Path $CaptureRoot 'raw'
    if(!$LabelOnly) {
        if($GameProcessId -le 0){throw 'Missing game process ID'}
        $api=(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'nr-capture-openvr.txt') -Raw).Trim()
        if(!(Test-Path -LiteralPath $api -PathType Leaf)){throw 'SteamVR API path missing'}
        $helper=Join-Path $PSScriptRoot 'cpvr_fov_image_audit.exe'
        $child=Start-Process -FilePath $helper -ArgumentList @('--capture',('"'+$api+'"'),$GameProcessId,('"'+$raw+'"')) -PassThru -WindowStyle Hidden
        $null=$child.Handle
        if(!$child.WaitForExit(12000)) {$child.Kill();$child.WaitForExit();throw 'Eye capture helper timed out'}
        if($child.ExitCode -ne 0){throw "Eye capture failed: $($child.ExitCode)"}
    }
    Add-Type -AssemblyName System.Drawing
    # A reload can leave a tiny HUD on an otherwise black mirror. Keep raw
    # evidence, but do not publish it to Pictures as a successful comparison.
    $quality=@()
    foreach($eye in @('left','right')) {
        $probe=[Drawing.Bitmap]::FromFile((Join-Path $raw ($eye+'.bmp')))
        try {
            $dark=0;$samples=0;$sum=0.0
            $stepX=[Math]::Max(1,[int][Math]::Floor($probe.Width/128))
            $stepY=[Math]::Max(1,[int][Math]::Floor($probe.Height/128))
            for($y=0;$y -lt $probe.Height;$y+=$stepY){for($x=0;$x -lt $probe.Width;$x+=$stepX){
                $pixel=$probe.GetPixel($x,$y);$samples++
                $sum+=($pixel.R+$pixel.G+$pixel.B)/3.0
                if($pixel.R -le 3 -and $pixel.G -le 3 -and $pixel.B -le 3){$dark++}
            }}
            $quality+=@{eye=$eye;samples=$samples;darkFraction=$dark/[double]$samples;meanRgb=$sum/$samples}
        } finally {$probe.Dispose()}
    }
    $quality | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $CaptureRoot 'image-quality.json')
    if(@($quality | Where-Object {$_.darkFraction -gt 0.995 -and $_.meanRgb -lt 0.5}).Count){
        throw 'Near-black eye mirror: possibly loading, fade, or unavailable scene. Raw BMPs retained; no Pictures copies. Capture again during visible gameplay. A genuinely black scene may also trigger this check.'
    }
    $lines=@("NR $(if($settings.NrEnabled){'ON'}else{'OFF'}) | $(if($settings.NrProcessingOrder -eq 0){'AFTER'}else{'BEFORE'}) UPSCALING | $(if($settings.NrFoveated){'FOVEATED'}else{'FULL FRAME'})")
    if($settings.NrEnabled) {
        foreach($number in 1..3) {
            if($number -eq 2 -and !$settings.NrSecondPass){continue}
            if($number -eq 3 -and (!$settings.NrSecondPass -or !$settings.NrThirdPass)){continue}
            $prefix=if($number -eq 1){'Nr'}else{"NrPass$number"}
            $network=@('Default','A','B')[[int]$settings.($prefix+'Preset')]
            $appearance=@('A','B','C')[[int]$settings.($prefix+'Style')]
            $lines+=('Pass {0}: Network {1} | Appearance {2} | Scale {3:0.00} | Shared intensity {4:0.00}' -f $number,$network,$appearance,$settings.($prefix+'WorkingScale'),$settings.NrIntensity)
        }
    }
    $run=Split-Path -Leaf $CaptureRoot
    if(!$PicturesRoot){$PicturesRoot=[Environment]::GetFolderPath('MyPictures')}
    if(!$PicturesRoot){throw 'Windows Pictures folder unavailable; raw captures retained'}
    $pictures=Join-Path (Join-Path $PicturesRoot 'CheekyFoveatedDLSS') $run
    if(Test-Path -LiteralPath $pictures){throw 'Pictures destination already exists; refusing to overwrite'}
    New-Item -ItemType Directory -Path $pictures | Out-Null
    foreach($eye in @('left','right')) {
        $source=Join-Path $raw ($eye+'.bmp')
        $bitmap=[Drawing.Bitmap]::FromFile($source)
        $font=$null;$canvas=$null;$graphics=$null;$measure=$null
        try {
            $text=@($lines)+@("$($eye.ToUpperInvariant()) | $run | sequential SteamVR mirror")
            $size=[Math]::Max(12,[Math]::Floor($bitmap.Width/90))
            $measure=[Drawing.Graphics]::FromImage($bitmap)
            do {
                if($font){$font.Dispose()}
                $font=New-Object Drawing.Font('Consolas',[single]$size)
                $max=0
                foreach($line in $text){$max=[Math]::Max($max,$measure.MeasureString($line,$font).Width)}
                $size--
            } while($max -gt ($bitmap.Width-32) -and $size -ge 6)
            if($max -gt ($bitmap.Width-32)){throw 'Image too narrow for comparison labels'}
            $spacing=[int][Math]::Ceiling($font.GetHeight($measure))+10
            $header=$spacing*$text.Count+24
            $canvas=New-Object Drawing.Bitmap($bitmap.Width,($bitmap.Height+$header))
            $graphics=[Drawing.Graphics]::FromImage($canvas)
            $graphics.Clear([Drawing.Color]::Black)
            $graphics.DrawImageUnscaled($bitmap,0,$header)
            for($i=0;$i -lt $text.Count;$i++){
                $graphics.DrawString($text[$i],$font,[Drawing.Brushes]::Lime,[single]16,[single](12+$i*$spacing))
            }
            $output=Join-Path $CaptureRoot ($eye+'-labelled.png')
            if(Test-Path -LiteralPath $output){throw 'Labelled image already exists'}
            $canvas.Save($output,[Drawing.Imaging.ImageFormat]::Png)
            Copy-Item -LiteralPath $output -Destination $pictures
        } finally {
            if($graphics){$graphics.Dispose()};if($canvas){$canvas.Dispose()}
            if($font){$font.Dispose()};if($measure){$measure.Dispose()};$bitmap.Dispose()
        }
    }
    Copy-Item -LiteralPath $metadata -Destination $pictures
    @("Pictures: $pictures",'Original BMPs retained in raw. Green labels describe settings at capture request; eyes are sequential, not simultaneous.') |
        Set-Content -LiteralPath (Join-Path $CaptureRoot 'capture-result.txt')
} catch {
    $_ | Out-String | Set-Content -LiteralPath (Join-Path $CaptureRoot 'capture-error.txt')
    exit 1
}
