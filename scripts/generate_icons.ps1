Add-Type -AssemblyName System.Drawing

function New-Icon {
    param(
        [string]$IconPath,
        [string]$Letters,
        [System.Drawing.Color]$BackA,
        [System.Drawing.Color]$BackB,
        [System.Drawing.Color]$Accent
    )

    $size = 256
    $bmp = New-Object System.Drawing.Bitmap $size, $size
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $rect = New-Object System.Drawing.Rectangle 0, 0, $size, $size
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $rect, $BackA, $BackB, 45
    $g.FillRectangle($brush, $rect)

    $card = New-Object System.Drawing.Rectangle 38, 34, 180, 188
    $radius = 24
    $shapePath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $null = $shapePath.AddArc($card.Left, $card.Top, $radius, $radius, 180, 90)
    $null = $shapePath.AddArc($card.Right - $radius, $card.Top, $radius, $radius, 270, 90)
    $null = $shapePath.AddArc($card.Right - $radius, $card.Bottom - $radius, $radius, $radius, 0, 90)
    $null = $shapePath.AddArc($card.Left, $card.Bottom - $radius, $radius, $radius, 90, 90)
    $shapePath.CloseFigure()
    $cardBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(238, 34, 37, 45))
    $g.FillPath($cardBrush, $shapePath)

    $pen = New-Object System.Drawing.Pen $Accent, 10
    $g.DrawLine($pen, 68, 84, 188, 84)
    $g.DrawLine($pen, 68, 122, 188, 122)
    $g.DrawLine($pen, 68, 160, 150, 160)

    $font = New-Object System.Drawing.Font "Segoe UI", 52, ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
    $textBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
    $format = New-Object System.Drawing.StringFormat
    $format.Alignment = [System.Drawing.StringAlignment]::Center
    $format.LineAlignment = [System.Drawing.StringAlignment]::Center
    $textRect = New-Object System.Drawing.RectangleF 0, 160, $size, 74
    $g.DrawString($Letters, $font, $textBrush, $textRect, $format)

    $iconHandle = $bmp.GetHicon()
    $icon = [System.Drawing.Icon]::FromHandle($iconHandle)
    $stream = [System.IO.File]::Open($IconPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
    $icon.Save($stream)
    $stream.Close()

    $g.Dispose()
    $shapePath.Dispose()
    $bmp.Dispose()
    $icon.Dispose()
}

if (-not (Test-Path assets)) {
    New-Item -ItemType Directory assets | Out-Null
}

New-Icon -IconPath "assets\AttendanceApp.ico" -Letters "AT" -BackA ([System.Drawing.Color]::FromArgb(30, 64, 175)) -BackB ([System.Drawing.Color]::FromArgb(15, 23, 42)) -Accent ([System.Drawing.Color]::FromArgb(96, 165, 250))
New-Icon -IconPath "assets\AttdFile.ico" -Letters ".A" -BackA ([System.Drawing.Color]::FromArgb(6, 95, 70)) -BackB ([System.Drawing.Color]::FromArgb(17, 24, 39)) -Accent ([System.Drawing.Color]::FromArgb(52, 211, 153))
