param(
    [string]$Executable = '',
    [int]$MenuIterations = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Executable)) {
    $Executable = Join-Path $root 'build\Release\AttendanceApp.exe'
}
$Executable = [System.IO.Path]::GetFullPath($Executable)
if (-not (Test-Path -LiteralPath $Executable)) {
    throw "Executable not found: $Executable"
}

$parserCheck = Start-Process -FilePath $Executable -ArgumentList '--verify-parser' -Wait -PassThru
if ($parserCheck.ExitCode -ne 0) {
    throw "Internal parser regression checks failed with exit code $($parserCheck.ExitCode)."
}

Add-Type -AssemblyName Accessibility
Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class AttendanceSmokeNative
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern IntPtr GetDlgItem(IntPtr parent, int id);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern bool SetWindowText(IntPtr hwnd, string text);

    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string className, string title);

    [DllImport("user32.dll")]
    public static extern uint GetGuiResources(IntPtr process, uint flags);

    [DllImport("user32.dll")]
    public static extern IntPtr GetDC(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hwnd, IntPtr hdc, uint flags);

    [DllImport("user32.dll")]
    public static extern int ReleaseDC(IntPtr hwnd, IntPtr hdc);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out Rect rect);

    [DllImport("gdi32.dll")]
    public static extern uint GetPixel(IntPtr hdc, int x, int y);

    [DllImport("gdi32.dll")]
    public static extern IntPtr CreateCompatibleDC(IntPtr hdc);

    [DllImport("gdi32.dll")]
    public static extern IntPtr CreateCompatibleBitmap(IntPtr hdc, int width, int height);

    [DllImport("gdi32.dll")]
    public static extern IntPtr SelectObject(IntPtr hdc, IntPtr obj);

    [DllImport("gdi32.dll")]
    public static extern bool DeleteObject(IntPtr obj);

    [DllImport("gdi32.dll")]
    public static extern bool DeleteDC(IntPtr hdc);

    [DllImport("dwmapi.dll", EntryPoint = "DwmGetWindowAttribute")]
    public static extern int DwmGetWindowAttributeInt(
        IntPtr hwnd, int attribute, out int value, int valueSize);

    [DllImport("user32.dll")]
    public static extern bool IsWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern bool PostThreadMessage(uint threadId, uint message, IntPtr wParam, IntPtr lParam);

    [DllImport("oleacc.dll")]
    public static extern int AccessibleObjectFromWindow(
        IntPtr hwnd,
        uint objectId,
        ref Guid interfaceId,
        [MarshalAs(UnmanagedType.Interface)] out object accessibleObject);
}
'@

function Wait-Until {
    param(
        [scriptblock]$Condition,
        [int]$TimeoutMilliseconds,
        [string]$FailureMessage
    )
    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMilliseconds)
    do {
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 15
    } while ([DateTime]::UtcNow -lt $deadline)
    throw $FailureMessage
}

function Get-AccessibleName {
    param([IntPtr]$Window)
    if ($Window -eq [IntPtr]::Zero) { return '' }
    $accessibleInterface = [Guid]'618736E0-3C3D-11CF-810C-00AA00389B71'
    $clientObjectId = [Convert]::ToUInt32('FFFFFFFC', 16)
    $accessibleObject = $null
    $result = [AttendanceSmokeNative]::AccessibleObjectFromWindow(
        $Window,
        $clientObjectId,
        [ref]$accessibleInterface,
        [ref]$accessibleObject)
    if ($result -ne 0 -or $null -eq $accessibleObject) { return '' }
    return [string]$accessibleObject.accName(0)
}

function Set-EditTextLikeUser {
    param(
        [IntPtr]$Window,
        [string]$Text
    )
    [AttendanceSmokeNative]::SetWindowText($Window, '') | Out-Null
    foreach ($character in $Text.ToCharArray()) {
        [AttendanceSmokeNative]::SendMessage(
            $Window, 0x0102, [IntPtr][int]$character, [IntPtr]::Zero) | Out-Null
    }
}

function Get-ClientPixelRed {
    param(
        [IntPtr]$Window,
        [int]$X = 5,
        [int]$Y = 5
    )
    $dc = [AttendanceSmokeNative]::GetDC($Window)
    if ($dc -eq [IntPtr]::Zero) { throw 'Could not acquire a window DC.' }
    try {
        $color = [AttendanceSmokeNative]::GetPixel($dc, $X, $Y)
        if ($color -eq 0xFFFFFFFF) { throw 'Could not sample a window pixel.' }
        return [int]($color -band 0xFF)
    }
    finally {
        [AttendanceSmokeNative]::ReleaseDC($Window, $dc) | Out-Null
    }
}

function Get-TitleBarPixel {
    param([IntPtr]$Window)
    $rect = [AttendanceSmokeNative+Rect]::new()
    if (-not [AttendanceSmokeNative]::GetWindowRect($Window, [ref]$rect)) {
        throw 'Could not query the window rectangle.'
    }
    $screenDc = [AttendanceSmokeNative]::GetDC([IntPtr]::Zero)
    if ($screenDc -eq [IntPtr]::Zero) { throw 'Could not acquire the screen DC.' }
    $memoryDc = [AttendanceSmokeNative]::CreateCompatibleDC($screenDc)
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    $bitmap = [AttendanceSmokeNative]::CreateCompatibleBitmap($screenDc, $width, $height)
    if ($memoryDc -eq [IntPtr]::Zero -or $bitmap -eq [IntPtr]::Zero) {
        [AttendanceSmokeNative]::ReleaseDC([IntPtr]::Zero, $screenDc) | Out-Null
        throw 'Could not allocate the title-bar capture buffer.'
    }
    $oldBitmap = [AttendanceSmokeNative]::SelectObject($memoryDc, $bitmap)
    try {
        if (-not [AttendanceSmokeNative]::PrintWindow($Window, $memoryDc, 2)) {
            throw 'PrintWindow could not capture the window.'
        }
        $x = [int]($width / 2)
        $y = 24
        $color = [AttendanceSmokeNative]::GetPixel($memoryDc, $x, $y)
        if ($color -eq 0xFFFFFFFF) { throw 'Could not sample the title bar.' }
        return [uint32]$color
    }
    finally {
        [AttendanceSmokeNative]::SelectObject($memoryDc, $oldBitmap) | Out-Null
        [AttendanceSmokeNative]::DeleteObject($bitmap) | Out-Null
        [AttendanceSmokeNative]::DeleteDC($memoryDc) | Out-Null
        [AttendanceSmokeNative]::ReleaseDC([IntPtr]::Zero, $screenDc) | Out-Null
    }
}

function Get-ColorRefLuminance {
    param([uint32]$Color)
    $red = [int]($Color -band 0xFF)
    $green = [int](($Color -shr 8) -band 0xFF)
    $blue = [int](($Color -shr 16) -band 0xFF)
    return [int](($red * 299 + $green * 587 + $blue * 114) / 1000)
}

function Assert-AppTitleBarMaterial {
    param(
        [IntPtr]$Window,
        [int]$ExpectedBackdrop,
        [string]$Name,
        [int]$ExpectedDark = 1
    )
    $backdrop = 0
    $backdropResult = [AttendanceSmokeNative]::DwmGetWindowAttributeInt(
        $Window, 38, [ref]$backdrop, 4)
    if ($backdropResult -ne 0 -or $backdrop -ne $ExpectedBackdrop) {
        throw "$Name system backdrop is $backdrop (HRESULT $backdropResult), expected $ExpectedBackdrop."
    }
    $immersiveDark = 0
    $darkResult = [AttendanceSmokeNative]::DwmGetWindowAttributeInt(
        $Window, 20, [ref]$immersiveDark, 4)
    if ($darkResult -ne 0 -or $immersiveDark -ne $ExpectedDark) {
        throw "$Name did not adopt the expected title-bar color mode."
    }
    $pixel = Get-TitleBarPixel $Window
    if (($pixel -band 0x00FFFFFF) -eq 0) {
        throw "$Name title bar is still rendered as pure black."
    }
    $captionLuminance = Get-ColorRefLuminance $pixel
    return [pscustomobject]@{
        Backdrop = $backdrop
        ImmersiveDark = $immersiveDark
        CaptionColor = ('0x{0:X6}' -f ($pixel -band 0x00FFFFFF))
        CaptionLuminance = $captionLuminance
        Pixel = ('0x{0:X6}' -f ($pixel -band 0x00FFFFFF))
        PixelLuminance = (Get-ColorRefLuminance $pixel)
    }
}

function Invoke-ToolsMenuCycle {
    param(
        [IntPtr]$Main,
        [IntPtr]$Tools,
        [int]$Iteration
    )
    [AttendanceSmokeNative]::PostMessage($Main, 0x0111, [IntPtr]1020, $Tools) | Out-Null
    $menu = [IntPtr]::Zero
    Wait-Until {
        $script:menu = [AttendanceSmokeNative]::FindWindow('AttendanceThemedPopupMenu', $null)
        return $script:menu -ne [IntPtr]::Zero
    } 2000 "Tools menu did not open on iteration $Iteration."
    Start-Sleep -Milliseconds 80
    [AttendanceSmokeNative]::PostMessage($menu, 0x0100, [IntPtr]27, [IntPtr]::Zero) | Out-Null
    Wait-Until {
        return -not [AttendanceSmokeNative]::IsWindow($menu)
    } 2000 "Tools menu did not close on iteration $Iteration."
    Start-Sleep -Milliseconds 30
}

$testData = Join-Path $root 'build\runtime-smoke-appdata\AttendanceApp'
[System.IO.Directory]::CreateDirectory($testData) | Out-Null
$testSettings = @'
language=zh-HK
advanced_personalization=1
theme=0
font=Segoe UI
particles=1
animation_level=2
particle_level=1
report_template=3
risk_alerts=1
autosave_prompt=0
command_palette=1
advanced_filter=1
smooth_scrolling=1
scroll_animation_fps=60
'@
[System.IO.File]::WriteAllText(
    (Join-Path $testData 'settings.ini'),
    $testSettings,
    [System.Text.UTF8Encoding]::new($false))
Remove-Item -LiteralPath (Join-Path $testData 'autosave.attd') -Force -ErrorAction SilentlyContinue

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = $Executable
$startInfo.UseShellExecute = $false
$startInfo.EnvironmentVariables['ATTENDANCEAPP_DATA_DIR'] = $testData
$startInfo.EnvironmentVariables['ATTENDANCEAPP_TEST_MODE'] = '1'
$process = [System.Diagnostics.Process]::new()
$process.StartInfo = $startInfo
$main = [IntPtr]::Zero
$normalExit = $false

try {
    if (-not $process.Start()) { throw 'AttendanceApp did not start.' }
    $process.WaitForInputIdle(5000) | Out-Null
    Wait-Until {
        $process.Refresh()
        $script:main = $process.MainWindowHandle
        return $script:main -ne [IntPtr]::Zero
    } 10000 'AttendanceApp main window did not appear.'

    $dateName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($main, 1001))
    $studentName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($main, 1002))
    $notesName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($main, 1003))
    $searchName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($main, 1021))
    $listName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($main, 1004))
    $accessibilityNames = [ordered]@{
        Date = $dateName
        Student = $studentName
        Notes = $notesName
        Search = $searchName
        List = $listName
    }
    foreach ($entry in $accessibilityNames.GetEnumerator()) {
        if ([string]::IsNullOrWhiteSpace([string]$entry.Value)) {
            throw "The $($entry.Key) accessibility name is empty."
        }
    }

    $mainTitleBar = Assert-AppTitleBarMaterial $main 3 'Main window' 1
    $settingsWindow = [AttendanceSmokeNative]::SendMessage(
        $main, 0x802B, [IntPtr]3, [IntPtr]::Zero)
    if ($settingsWindow -eq [IntPtr]::Zero -or -not [AttendanceSmokeNative]::IsWindow($settingsWindow)) {
        throw 'The settings window did not open for title-bar verification.'
    }
    $settingsTitleBar = Assert-AppTitleBarMaterial $settingsWindow 3 'Settings window' 1

    $languageSwitchResult = [AttendanceSmokeNative]::SendMessage(
        $main, 0x802C, [IntPtr]0, [IntPtr]::Zero)
    if ($languageSwitchResult.ToInt32() -ne 1) {
        throw 'The atomic language-switch test hook did not run.'
    }
    $englishDateName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($main, 1001))
    $englishSettingsLanguageName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($settingsWindow, 4001))
    if ($englishDateName -ne 'Date/Time' -or $englishSettingsLanguageName -ne 'Language') {
        throw "Language switch did not update open windows atomically: '$englishDateName', '$englishSettingsLanguageName'."
    }
    $languageSwitchBackResult = [AttendanceSmokeNative]::SendMessage(
        $main, 0x802C, [IntPtr]19, [IntPtr]::Zero)
    if ($languageSwitchBackResult.ToInt32() -ne 1) {
        throw 'The atomic language-switch test hook did not restore zh-HK.'
    }
    $restoredDateName = Get-AccessibleName ([AttendanceSmokeNative]::GetDlgItem($main, 1001))
    if ($restoredDateName -ne $dateName) {
        throw "Language switch did not restore the original accessibility text: '$restoredDateName'."
    }

    $themeStart = Get-ClientPixelRed $main
    $titleBarStart = $mainTitleBar.CaptionLuminance
    [AttendanceSmokeNative]::PostMessage(
        $main, 0x802B, [IntPtr]1, [IntPtr]::Zero) | Out-Null

    Start-Sleep -Milliseconds 85
    $themeQuarter = Get-ClientPixelRed $main
    $titleBarQuarter = Get-ColorRefLuminance (Get-TitleBarPixel $main)
    Start-Sleep -Milliseconds 85
    $themeHalf = Get-ClientPixelRed $main
    $titleBarHalf = Get-ColorRefLuminance (Get-TitleBarPixel $main)
    Start-Sleep -Milliseconds 85
    $themeThreeQuarter = Get-ClientPixelRed $main
    $titleBarThreeQuarter = Get-ColorRefLuminance (Get-TitleBarPixel $main)
    Start-Sleep -Milliseconds 180
    $themeEnd = Get-ClientPixelRed $main

    $themeSamples = @($themeStart, $themeQuarter, $themeHalf, $themeThreeQuarter, $themeEnd)
    if ($themeStart -gt 16 -or $themeEnd -lt 240) {
        throw "Theme transition endpoints are invalid: $($themeSamples -join ', ')."
    }
    $themeIsMonotonic = ($themeQuarter -gt $themeStart) -and ($themeHalf -gt $themeQuarter) -and ($themeThreeQuarter -ge $themeHalf) -and ($themeEnd -ge $themeThreeQuarter)
    if (-not $themeIsMonotonic) {
        throw "Theme transition did not expose monotonic intermediate frames: $($themeSamples -join ', ')."
    }
    Start-Sleep -Milliseconds 120
    $lightTitleBar = Assert-AppTitleBarMaterial $main 3 'Main window in light theme' 0
    $settingsLightTitleBar = Assert-AppTitleBarMaterial $settingsWindow 3 'Settings window in light theme' 0
    $titleBarSamples = @($titleBarStart, $titleBarQuarter, $titleBarHalf, $titleBarThreeQuarter, $lightTitleBar.CaptionLuminance)
    $titleBarIsMonotonic = ($titleBarQuarter -gt $titleBarStart) -and ($titleBarHalf -gt $titleBarQuarter) -and ($titleBarThreeQuarter -ge $titleBarHalf) -and ($lightTitleBar.CaptionLuminance -ge $titleBarThreeQuarter)
    if (-not $titleBarIsMonotonic) {
        throw "Title-bar transition did not expose monotonic intermediate frames: $($titleBarSamples -join ', ')."
    }

    [AttendanceSmokeNative]::PostMessage(
        $main, 0x802B, [IntPtr]2, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 170
    $themeDisableHalf = Get-ClientPixelRed $main
    Start-Sleep -Milliseconds 260
    $themeReturn = Get-ClientPixelRed $main
    if ($themeDisableHalf -le 16 -or $themeDisableHalf -ge 240 -or $themeReturn -gt 16) {
        throw "Disabling advanced personalization did not animate back to dark: $themeDisableHalf, $themeReturn."
    }
    Start-Sleep -Milliseconds 120
    $returnTitleBar = Assert-AppTitleBarMaterial $main 3 'Main window after returning to dark' 1
    $settingsReturnTitleBar = Assert-AppTitleBarMaterial $settingsWindow 3 'Settings window after returning to dark' 1
    [AttendanceSmokeNative]::PostMessage(
        $settingsWindow, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    Wait-Until {
        return -not [AttendanceSmokeNative]::IsWindow($settingsWindow)
    } 2000 'The settings window did not close after title-bar verification.'
    $list = [AttendanceSmokeNative]::GetDlgItem($main, 1004)
    Set-EditTextLikeUser ([AttendanceSmokeNative]::GetDlgItem($main, 1002)) 'Release smoke student'
    [AttendanceSmokeNative]::SendMessage(
        [AttendanceSmokeNative]::GetDlgItem($main, 1006),
        0x00F5,
        [IntPtr]::Zero,
        [IntPtr]::Zero) | Out-Null
    Wait-Until {
        return [AttendanceSmokeNative]::SendMessage(
            $list, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero).ToInt32() -eq 1
    } 2000 'The smoke-test attendance record was not created.'

    [AttendanceSmokeNative]::SetWindowText(
        [AttendanceSmokeNative]::GetDlgItem($main, 1021), '缺席') | Out-Null
    Wait-Until {
        return [AttendanceSmokeNative]::SendMessage(
            $list, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero).ToInt32() -eq 1
    } 2000 'Searching for the localized Absent status did not keep the matching row.'

    $tools = [AttendanceSmokeNative]::GetDlgItem($main, 1020)
    for ($iteration = 0; $iteration -lt 5; ++$iteration) {
        Invoke-ToolsMenuCycle $main $tools (-5 + $iteration)
    }
    $process.Refresh()
    $gdiBefore = [AttendanceSmokeNative]::GetGuiResources($process.Handle, 0)
    for ($iteration = 0; $iteration -lt $MenuIterations; ++$iteration) {
        Invoke-ToolsMenuCycle $main $tools $iteration
    }
    Start-Sleep -Milliseconds 100
    $process.Refresh()
    $gdiAfter = [AttendanceSmokeNative]::GetGuiResources($process.Handle, 0)
    if (($gdiAfter - $gdiBefore) -gt 8) {
        throw "GDI objects grew from $gdiBefore to $gdiAfter."
    }

    [AttendanceSmokeNative]::PostMessage($main, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
    $normalExit = $process.WaitForExit(5000)
    if (-not $normalExit) { throw 'AttendanceApp did not exit after WM_CLOSE.' }

    [pscustomobject]@{
        DateAccessibilityName = $dateName
        StudentAccessibilityName = $studentName
        NotesAccessibilityName = $notesName
        SearchAccessibilityName = $searchName
        ListAccessibilityName = $listName
        MainTitleBar = ($mainTitleBar | ConvertTo-Json -Compress)
        SettingsTitleBar = ($settingsTitleBar | ConvertTo-Json -Compress)
        AtomicLanguageSwitch = "$dateName -> $englishDateName -> $restoredDateName"
        LightTitleBar = ($lightTitleBar | ConvertTo-Json -Compress)
        SettingsLightTitleBar = ($settingsLightTitleBar | ConvertTo-Json -Compress)
        ReturnTitleBar = ($returnTitleBar | ConvertTo-Json -Compress)
        SettingsReturnTitleBar = ($settingsReturnTitleBar | ConvertTo-Json -Compress)
        LocalizedSearchRows = 1
        ThemeTransitionSamples = ($themeSamples -join ', ')
        TitleBarTransitionSamples = ($titleBarSamples -join ', ')
        AdvancedDisableMidpoint = $themeDisableHalf
        ThemeReturnSample = $themeReturn
        GdiObjectsBefore = $gdiBefore
        GdiObjectsAfter = $gdiAfter
        ExitCode = $process.ExitCode
    } | Format-List
}
finally {
    if ($process -and -not $process.HasExited) {
        if ($main -ne [IntPtr]::Zero) {
            [AttendanceSmokeNative]::PostMessage($main, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
            $normalExit = $process.WaitForExit(1000)
        }
        if (-not $process.HasExited) {
            $mainThread = $process.Threads | Sort-Object StartTime | Select-Object -First 1
            if ($mainThread) {
                [AttendanceSmokeNative]::PostThreadMessage(
                    [uint32]$mainThread.Id, 0x0012, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
                $process.WaitForExit(1000) | Out-Null
            }
        }
        if (-not $process.HasExited) {
            $process.Kill()
            $process.WaitForExit()
        }
    }
    $process.Dispose()
}
