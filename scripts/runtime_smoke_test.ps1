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

Add-Type -AssemblyName Accessibility
Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class AttendanceSmokeNative
{
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

    $gdiBefore = [AttendanceSmokeNative]::GetGuiResources($process.Handle, 0)
    $tools = [AttendanceSmokeNative]::GetDlgItem($main, 1020)
    for ($iteration = 0; $iteration -lt $MenuIterations; ++$iteration) {
        [AttendanceSmokeNative]::PostMessage($main, 0x0111, [IntPtr]1020, $tools) | Out-Null
        $menu = [IntPtr]::Zero
        Wait-Until {
            $script:menu = [AttendanceSmokeNative]::FindWindow('AttendanceThemedPopupMenu', $null)
            return $script:menu -ne [IntPtr]::Zero
        } 2000 "Tools menu did not open on iteration $iteration."
        [AttendanceSmokeNative]::PostMessage($menu, 0x0100, [IntPtr]27, [IntPtr]::Zero) | Out-Null
        Wait-Until {
            return -not [AttendanceSmokeNative]::IsWindow($menu)
        } 2000 "Tools menu did not close on iteration $iteration."
    }
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
        LocalizedSearchRows = 1
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
