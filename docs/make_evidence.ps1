# Build evidence figures from the recorded videos (ASCII only)
Add-Type -AssemblyName System.Drawing

$outDir   = 'C:\Users\Administrator\Desktop\STM\work\5\docs\images'
$valsFile = Join-Path $env:TEMP 'v1_vals.txt'
$onFile   = Join-Path $env:TEMP 'frames\on.jpg'
$offFile  = Join-Path $env:TEMP 'frames\off.jpg'

$fontTitle = New-Object Drawing.Font('Arial', 17, [Drawing.FontStyle]::Bold)
$fontText  = New-Object Drawing.Font('Arial', 12)
$fontSmall = New-Object Drawing.Font('Arial', 10)
$brush     = [Drawing.Brushes]::Black
$brushGray = New-Object Drawing.SolidBrush ([Drawing.Color]::FromArgb(90,90,90))
$penAxis   = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(60,60,60)), 2
$penGrid   = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(225,225,225)), 1
$penLine   = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(0,90,200)), 3

# ---------------------------------------------------------- brightness chart
$vals = Get-Content $valsFile | Where-Object { $_ -match '^\d' } | ForEach-Object { [double]$_ }
$n = $vals.Count
$fps = 10.0

$bmp = New-Object Drawing.Bitmap 1100, 560
$g   = [Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = 'AntiAlias'
$g.TextRenderingHint = 'AntiAlias'
$g.Clear([Drawing.Color]::White)

$g.DrawString('Measured brightness of the breathing LED (from video frame average)', $fontTitle, $brush, 35, 22)
$g.DrawString('YAVG  (0-255)', $fontSmall, $brushGray, 40, 60)

$px = 95; $py = 85; $pw = 950; $ph = 380
$baseY = $py + $ph
$yMin = 98.0; $yMax = 112.0
$xMax = $n / $fps

$g.DrawLine($penAxis, $px, $py, $px, $baseY)
$g.DrawLine($penAxis, $px, $baseY, $px + $pw, $baseY)

for ($k = 0; $k -le 5; $k++) {
    $xg = [int]($px + $pw * $k / 5.0)
    $g.DrawLine($penGrid, $xg, $py, $xg, $baseY)
    $g.DrawString("$k s", $fontSmall, $brushGray, $xg - 8, $baseY + 6)
}
for ($v = 98; $v -le 112; $v += 2) {
    $yg = [int]($baseY - $ph * ($v - $yMin) / ($yMax - $yMin))
    $g.DrawLine($penGrid, $px, $yg, $px + $pw, $yg)
    $g.DrawString("$v", $fontSmall, $brushGray, $px - 34, $yg - 8)
}

$pts = New-Object 'System.Collections.Generic.List[Drawing.Point]'
for ($i = 0; $i -lt $n; $i++) {
    $xg = [int]($px + $pw * ($i / $fps) / $xMax)
    $yg = [int]($baseY - $ph * ($vals[$i] - $yMin) / ($yMax - $yMin))
    $pts.Add((New-Object Drawing.Point $xg, $yg))
}
$g.DrawLines($penLine, $pts.ToArray())

# peak markers (measured peaks at about 1.4 s, 3.0 s, 4.6 s -> T = 1.6 s)
$penDash = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(180,120,40)), 2
$penDash.DashStyle = 'Dash'
foreach ($t in 1.4, 3.0, 4.6) {
    $xg = [int]($px + $pw * $t / $xMax)
    $g.DrawLine($penDash, $xg, $py, $xg, $baseY)
    $g.DrawString(("t = {0:0.0} s" -f $t), $fontSmall, $brushGray, $xg - 24, $py + 4)
}

$g.DrawString('Peak-to-peak interval: 1.6 s  (design value 1.4 s; HAL_Delay rounds every step up to 8 ms)', $fontText, $brush, 95, 500)
$bmp.Save((Join-Path $outDir 'breath-measured.png'), [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

# ---------------------------------------------------------- ON / OFF montage
$on  = [Drawing.Image]::FromFile($onFile)
$off = [Drawing.Image]::FromFile($offFile)
$h   = 520
$wOn  = [int]($on.Width  * $h / $on.Height)
$wOff = [int]($off.Width * $h / $off.Height)

$canvasW = 20 + $wOn + 20 + $wOff + 20
$canvasH = 520 + 130
$bmp2 = New-Object Drawing.Bitmap $canvasW, $canvasH
$g2   = [Drawing.Graphics]::FromImage($bmp2)
$g2.SmoothingMode = 'AntiAlias'
$g2.TextRenderingHint = 'AntiAlias'
$g2.Clear([Drawing.Color]::White)

$g2.DrawString('Task 1: LED ON / OFF (two frames captured 1 s apart)', $fontTitle, $brush, 20, 18)
$g2.DrawString('LED ON', $fontText, $brush, 20, 52)
$g2.DrawString('LED OFF', $fontText, $brush, 20 + $wOn + 20, 52)

$g2.DrawImage($on,  20, 78, $wOn, $h)
$g2.DrawImage($off, 20 + $wOn + 20, 78, $wOff, $h)

$penBox = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(200,200,200)), 1
$g2.DrawRectangle($penBox, 20, 78, $wOn, $h)
$g2.DrawRectangle($penBox, 20 + $wOn + 20, 78, $wOff, $h)

$g2.DrawString('Full cycle: 2 s ON / 2 s OFF  (TIM2 update interrupt x 2000, 1 ms each)', $fontText, $brushGray, 20, 78 + $h + 14)
$bmp2.Save((Join-Path $outDir 'blink-onoff.png'), [Drawing.Imaging.ImageFormat]::Png)
$g2.Dispose(); $bmp2.Dispose()
$on.Dispose(); $off.Dispose()

Write-Output 'evidence figures done'
