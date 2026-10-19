# Generate PWM / blink waveform figures for the lab report (ASCII only)
Add-Type -AssemblyName System.Drawing

$outDir = 'C:\Users\Administrator\Desktop\STM\work\5\docs\images'
if (!(Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

$fontTitle = New-Object Drawing.Font('Arial', 18, [Drawing.FontStyle]::Bold)
$font      = New-Object Drawing.Font('Arial', 14)
$fontSmall = New-Object Drawing.Font('Arial', 11)
$brush     = [Drawing.Brushes]::Black
$brushGray = New-Object Drawing.SolidBrush ([Drawing.Color]::FromArgb(90,90,90))
$penAxis   = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(70,70,70)), 2
$penGrid   = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(220,220,220)), 1
$penWave   = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(0,90,200)), 3
$penWave2  = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(200,60,40)), 3

# ---------------------------------------------------------------- figure 1
$bmp = New-Object Drawing.Bitmap 1200, 950
$g   = [Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = 'AntiAlias'
$g.TextRenderingHint = 'AntiAlias'
$g.Clear([Drawing.Color]::White)

$g.DrawString('PWM output on TIM2_CH2 (PA1), f = 1 kHz, T = 1 ms', $fontTitle, $brush, 40, 25)

$duties = @(0.25, 0.50, 0.75)
$names  = @('25 % duty  (CCR2 = 250)', '50 % duty  (CCR2 = 500)', '75 % duty  (CCR2 = 750)')
$plotX  = 120
$plotW  = 980
$plotH  = 170
$top    = 90
$gap    = 70

for ($i = 0; $i -lt 3; $i++) {
    $y0 = $top + $i * ($plotH + $gap)
    $base = $y0 + $plotH
    $g.DrawString($names[$i], $font, $brush, $plotX, $y0 - 34)

    # axes
    $g.DrawLine($penAxis, $plotX, $base, $plotX + $plotW, $base)
    $g.DrawLine($penAxis, $plotX, $y0, $plotX, $base)

    # 3 periods
    for ($k = 0; $k -le 3; $k++) {
        $xg = $plotX + [int]($plotW * $k / 3.0)
        $g.DrawLine($penGrid, $xg, $y0, $xg, $base)
        if ($k -lt 3) {
            $g.DrawString("$k ms", $fontSmall, $brushGray, $xg, $base + 6)
        }
    }
    $g.DrawString('5 V', $fontSmall, $brushGray, $plotX - 55, $y0 - 8)
    $g.DrawString('0 V', $fontSmall, $brushGray, $plotX - 55, $base - 10)

    # waveform
    $d = $duties[$i]
    $pts = New-Object 'System.Collections.Generic.List[Drawing.Point]'
    $pts.Add((New-Object Drawing.Point ($plotX), ($base)))
    $step = $plotW / 3.0
    for ($k = 0; $k -lt 3; $k++) {
        $xStart = [int]($plotX + $k * $step)
        $xFall  = [int]($plotX + ($k + $d) * $step)
        $xEnd   = [int]($plotX + ($k + 1) * $step)
        $pts.Add((New-Object Drawing.Point ($xStart), ($y0)))
        $pts.Add((New-Object Drawing.Point ($xFall),  ($y0)))
        $pts.Add((New-Object Drawing.Point ($xFall),  ($base)))
        $pts.Add((New-Object Drawing.Point ($xEnd),   ($base)))
    }
    $g.DrawLines($penWave, $pts.ToArray())

    # ton / T annotations on first period
    $xStart = $plotX
    $xFall  = [int]($plotX + $d * $step)
    $yAnno  = $y0 - 16
    $g.DrawLine($penWave2, $xStart, $yAnno, $xFall, $yAnno)
    $g.DrawString('Ton', $fontSmall, $brush, ($xStart + $xFall) / 2 - 12, $yAnno - 20)
    $g.DrawString('T = 1 ms', $fontSmall, $brushGray, $plotX + 8, $base + 26)
}

$bmp.Save((Join-Path $outDir 'pwm-duty.png'), [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

# ---------------------------------------------------------------- figure 2
$bmp = New-Object Drawing.Bitmap 1200, 620
$g   = [Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = 'AntiAlias'
$g.TextRenderingHint = 'AntiAlias'
$g.Clear([Drawing.Color]::White)

$g.DrawString('Breathing LED: duty ratio vs. time (one cycle = 1.4 s)', $fontTitle, $brush, 40, 25)

$px = 120; $py = 100; $pw = 980; $ph = 380
$baseY = $py + $ph
$g.DrawLine($penAxis, $px, $baseY, $px + $pw, $baseY)
$g.DrawLine($penAxis, $px, $py, $px, $baseY)
$g.DrawString('100 %', $fontSmall, $brushGray, $px - 60, $py - 8)
$g.DrawString('0 %',   $fontSmall, $brushGray, $px - 44, $baseY - 10)

# time axis 0 .. 1.5 s
for ($k = 0; $k -le 3; $k++) {
    $t = $k * 0.5
    $xg = [int]($px + $pw * $t / 1.5)
    $g.DrawLine($penGrid, $xg, $py, $xg, $baseY)
    $g.DrawString(("{0:0.0} s" -f $t), $fontSmall, $brushGray, $xg - 10, $baseY + 6)
}

# quadratic ramp 0..100 steps in 0.7 s, then down
$pts = New-Object 'System.Collections.Generic.List[Drawing.Point]'
$steps = 100
for ($i = 0; $i -le $steps; $i++) {
    $t = 0.7 * $i / $steps
    $duty = ($i * $i) / ($steps * $steps)
    $xg = [int]($px + $pw * $t / 1.5)
    $yg = [int]($baseY - $ph * $duty)
    $pts.Add((New-Object Drawing.Point $xg, $yg))
}
for ($i = $steps; $i -ge 0; $i--) {
    $t = 0.7 + 0.7 * ($steps - $i) / $steps
    $duty = ($i * $i) / ($steps * $steps)
    $xg = [int]($px + $pw * $t / 1.5)
    $yg = [int]($baseY - $ph * $duty)
    $pts.Add((New-Object Drawing.Point $xg, $yg))
}
$g.DrawLines($penWave, $pts.ToArray())

# dashed marker at 0.7 s
$penDash = New-Object Drawing.Pen ([Drawing.Color]::FromArgb(150,150,150)), 2
$penDash.DashStyle = 'Dash'
$xm = [int]($px + $pw * 0.7 / 1.5)
$g.DrawLine($penDash, $xm, $py, $xm, $baseY)
$g.DrawString('0.7 s (brightest)', $fontSmall, $brushGray, $xm + 8, $py + 6)

$g.DrawString('NOTE: carrier is 1 kHz PWM; the curve shows the CCR2 value (duty) updating every 7 ms.', $fontSmall, $brushGray, 40, 560)
$bmp.Save((Join-Path $outDir 'breath-envelope.png'), [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

# ---------------------------------------------------------------- figure 3
$bmp = New-Object Drawing.Bitmap 1200, 560
$g   = [Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = 'AntiAlias'
$g.TextRenderingHint = 'AntiAlias'
$g.Clear([Drawing.Color]::White)

$g.DrawString('Task 1: TIM2 update interrupt counts 2000 x 1 ms = 2 s, then toggles PA1', $fontTitle, $brush, 40, 25)

$px = 120; $py = 110; $pw = 980; $ph = 280
$baseY = $py + $ph
$g.DrawLine($penAxis, $px, $baseY, $px + $pw, $baseY)
$g.DrawLine($penAxis, $px, $py, $px, $baseY)
$g.DrawString('High', $fontSmall, $brushGray, $px - 60, $py - 8)
$g.DrawString('Low',  $fontSmall, $brushGray, $px - 52, $baseY - 10)

$total = 6.0
for ($k = 0; $k -le 6; $k++) {
    $xg = [int]($px + $pw * $k / $total)
    $g.DrawLine($penGrid, $xg, $py, $xg, $baseY)
    $g.DrawString(("{0:0} s" -f $k), $fontSmall, $brushGray, $xg - 10, $baseY + 6)
}

$pts = New-Object 'System.Collections.Generic.List[Drawing.Point]'
$pts.Add((New-Object Drawing.Point ($px), ($baseY)))
for ($k = 0; $k -lt 3; $k++) {
    $x1 = [int]($px + $pw * (2 * $k) / $total)
    $x2 = [int]($px + $pw * (2 * $k + 2) / $total)
    $pts.Add((New-Object Drawing.Point ($x1), ($py)))
    $pts.Add((New-Object Drawing.Point ($x2), ($py)))
    $pts.Add((New-Object Drawing.Point ($x2), ($baseY)))
}
$g.DrawLines($penWave, $pts.ToArray())

for ($k = 0; $k -lt 3; $k++) {
    $x1 = [int]($px + $pw * (2 * $k) / $total)
    $x2 = [int]($px + $pw * (2 * $k + 2) / $total)
    $g.DrawString('2 s ON', $fontSmall, $brush, ($x1 + $x2) / 2 - 25, $py + 12)
    if ($k -lt 2) {
        $g.DrawString('2 s OFF', $fontSmall, $brush, ($x2 + $x2) / 2 - 25, $baseY - 24)
    }
}

$bmp.Save((Join-Path $outDir 'blink-2s.png'), [Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

Write-Output 'figures done'
