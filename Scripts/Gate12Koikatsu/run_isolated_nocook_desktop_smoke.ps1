<#
Gate 12 — REAL Win64 editor GAME runtime smoke test without cooking.
Uses an isolated *copied* .uproject / Config and independent Saved directory.
Original Content and compiled Editor Binaries are junctioned; never touched.
No APK, UAT, Android cook/install, Registry edits, or original save access.
Results are observations of actual game launch, not automatic gameplay acceptance.
#>
param(
  [ValidateRange(20,720)]
  [int]$RunSeconds = 180,
  [switch]$SkipScreenshot,
  [switch]$Journey,
  [switch]$Onscreen
)
$ErrorActionPreference='Stop'
$Root=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$Branch=(& git -C $Root branch --show-current).Trim()
if ($Branch -ne 'production/gate12-koikatsu-ue583-fixture-20261010') {
  throw "Unexpected Git worktree: $Branch"
}
$Exe='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path $Exe)) { throw 'Win64 UnrealEditor.exe missing.' }
if (Get-Process UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
  throw 'Other Unreal processes exist; refusing low-RAM parallel run.'
}
$BootIni=Get-Content (Join-Path $Root 'Config\DefaultEngine.ini') -Raw
if ($BootIni -notmatch '(?m)^GameDefaultMap=/Game/Maps/StartingWorld\s*$') {
  throw 'GameDefaultMap must be the real StartingWorld, not an experimental QA map.'
}
$Destination=Join-Path $Root 'Saved\Gate12Readiness\Isolated_NoCook_Win64_Game'
$Project=Join-Path $Destination 'OfflineGame.uproject'
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
if (!(Test-Path $Project)) {
  Copy-Item -LiteralPath (Join-Path $Root 'OfflineGame.uproject') -Destination $Project
}
$Config=Join-Path $Destination 'Config'
if (!(Test-Path $Config)) {New-Item -ItemType Directory -Path $Config -Force | Out-Null}
Get-ChildItem (Join-Path $Root 'Config') -File | ForEach-Object {
  Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $Config $_.Name) -Force
}
foreach ($Name in @('Binaries','Content')) {
  $To=Join-Path $Destination $Name
  if (!(Test-Path $To)) {
    New-Item -ItemType Junction -Path $To -Target (Join-Path $Root $Name) -ErrorAction Stop | Out-Null
  }
  $Item=Get-Item -LiteralPath $To -Force
  if (!$Item.Attributes.HasFlag([IO.FileAttributes]::ReparsePoint)) {
    throw "Refusing unexpected non-junction directory: $To"
  }
}
$IsolatedSaved=Join-Path $Destination 'Saved'
New-Item -ItemType Directory -Path $IsolatedSaved -Force | Out-Null
$Stamp=Get-Date -Format 'yyyyMMdd_HHmmss'
$Log=Join-Path $IsolatedSaved "Gate12_Win64_Game_$Stamp.log"
$Result=Join-Path $Destination 'runtime_smoke_observation.json'
$Args=@(
 $Project,
 '/Game/Maps/StartingWorld',
 '-game','-windowed','-ResX=432','-ResY=700',
 '-NoSplash','-NoP4','-NoSound',
 "-ABSLOG=$Log"
)
if (!$Onscreen) {
 $Args += '-RenderOffscreen'
}
if ($Journey) {
 $Args += '-Gate12RuntimeJourney'
}
if (!$SkipScreenshot) {
 $Args += '-ExecCmds="HighResShot 1"'
}
Write-Host "NO-COOK isolated Win64 game, project=$Project"
$Process=Start-Process -FilePath $Exe -ArgumentList $Args -PassThru
$Timeout=(Get-Date).AddSeconds($RunSeconds)
$ScreenshotDir=Join-Path $IsolatedSaved 'Screenshots'
$ObservedShot=$null
$JourneyPath=Join-Path $IsolatedSaved 'Gate12Readiness\runtime_ruler_world_journey.json'
$JourneyResult=$null
$DesktopCaptureFailures=@()
$DesktopCapturePaths=@()
$LastCapturedStep=-1
if ($Onscreen) {
  Add-Type -AssemblyName System.Windows.Forms
  Add-Type -AssemblyName System.Drawing
  Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class Gate12VisibleWindow {
 [StructLayout(LayoutKind.Sequential)]
 public struct RECT { public int Left, Top, Right, Bottom; }
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr handle, out RECT rect);
}
'@
  $VisibleEvidence=Join-Path $IsolatedSaved 'Gate12Readiness\OnscreenFrames'
  New-Item -ItemType Directory -Path $VisibleEvidence -Force | Out-Null
}
try {
  while ((Get-Date) -lt $Timeout -and !$Process.HasExited) {
    Start-Sleep -Seconds 3
    if (Test-Path $ScreenshotDir) {
      $ObservedShot=Get-ChildItem $ScreenshotDir -Recurse -File -Filter '*.png' -ErrorAction SilentlyContinue |
        Where-Object { $_.LastWriteTime -ge $Process.StartTime } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
      if ($ObservedShot -and !$Journey) { break }
    }
    if ($Journey -and (Test-Path $JourneyPath)) {
      try {
        if ((Get-Item $JourneyPath).LastWriteTime -ge $Process.StartTime) {
          $JourneyResult=Get-Content $JourneyPath -Raw | ConvertFrom-Json
        }
      } catch { $JourneyResult=$null }
    }
    if ($Onscreen -and $JourneyResult -and $JourneyResult.steps.Count -gt $LastCapturedStep) {
      try {
        $Live=Get-Process -Id $Process.Id -ErrorAction Stop
        $Hwnd=$Live.MainWindowHandle
        if ($Hwnd -eq [IntPtr]::Zero) {throw 'Game native window not created or not visible'}
        $Rect=New-Object Gate12VisibleWindow+RECT
        if (![Gate12VisibleWindow]::GetWindowRect($Hwnd,[ref]$Rect)) {throw 'GetWindowRect failed'}
        $Left=[Math]::Max(0,$Rect.Left)
        $Top=[Math]::Max(0,$Rect.Top)
        $Width=[Math]::Min($Rect.Right-$Left,[System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Width-$Left)
        $Height=[Math]::Min($Rect.Bottom-$Top,[System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Height-$Top)
        if ($Width -le 0 -or $Height -le 0) {throw 'Game window outside visible monitor'}
        $Image=New-Object System.Drawing.Bitmap($Width,$Height)
        $Canvas=[System.Drawing.Graphics]::FromImage($Image)
        try {
          $Canvas.CopyFromScreen($Left,$Top,0,0,$Image.Size)
          $Idx=$JourneyResult.steps.Count
          $StepName=($JourneyResult.steps[-1].name -replace '[^a-zA-Z0-9_-]','_')
          $Shot=Join-Path $VisibleEvidence ("Step_{0:D2}_{1}.png" -f $Idx,$StepName)
          $Image.Save($Shot,[System.Drawing.Imaging.ImageFormat]::Png)
          $DesktopCapturePaths += $Shot
          $LastCapturedStep=$Idx
        } finally { $Canvas.Dispose();$Image.Dispose() }
      } catch {
        $DesktopCaptureFailures += [string]$_
      }
    }
    if ($JourneyResult -and $JourneyResult.complete) { break }
  }
  if ($JourneyResult -and $JourneyResult.complete) {
    # Screenshot requests flush on the following real Slate frame.
    Start-Sleep -Seconds 3
  }
  $GameUiShots=@(Get-ChildItem (Join-Path $IsolatedSaved 'Gate12Readiness\JourneyScreenshots') -File -Filter '*.png' -ErrorAction SilentlyContinue |
      Where-Object { $_.LastWriteTime -ge $Process.StartTime } |
      Sort-Object Name |
      Select-Object -ExpandProperty FullName)
  $LogContainsStarted=$false
  $LogContainsMap=$false
  $LogContainsError=$false
  if (Test-Path $Log) {
    $Source=Get-Content $Log -Raw
    $LogContainsStarted=($Source -match 'Game Engine Initialized|Game Engine Init|LogLoad:.*StartingWorld')
    $LogContainsMap=($Source -match 'StartingWorld')
    $LogContainsError=($Source -match 'Fatal error:|Assertion failed:|EXCEPTION_ACCESS_VIOLATION')
  }
  $Report=[ordered]@{
    schema='GATE12_WIN64_GAME_SMOKE_NOCOOK_V1'
    utc=(Get-Date).ToUniversalTime().ToString('o')
    project=$Project
    isolated_saved=$IsolatedSaved
    original_saves_untouched_by_project_path=$true
    launch_pid=$Process.Id
    game_boot_map='/Game/Maps/StartingWorld'
    startup_log=$Log
    game_engine_or_map_reported=$LogContainsStarted
    starting_world_seen_in_log=$LogContainsMap
    fatal_error_seen_in_log=$LogContainsError
    screenshot=if($ObservedShot){$ObservedShot.FullName}else{$null}
    screenshot_bytes=if($ObservedShot){$ObservedShot.Length}else{0}
    onscreen_window_capture_requested=[bool]$Onscreen
    onscreen_game_window_frames=$DesktopCapturePaths
    onscreen_capture_failures=$DesktopCaptureFailures
    native_slate_ui_screenshots=$GameUiShots
    native_slate_ui_screenshot_count=$GameUiShots.Count
    journey_enabled=[bool]$Journey
    journey_path=if($Journey){$JourneyPath}else{$null}
    journey_finished=if($JourneyResult){[bool]$JourneyResult.complete}else{$false}
    journey_passed=if($JourneyResult){[bool]$JourneyResult.passed}else{$false}
    programmatic_existing_ui_handlers_tested=[bool]($JourneyResult -and $JourneyResult.complete)
    proven_interactive_navigation=$false
    proven_android_device=$false
    outcome=if ($LogContainsError){'CRASH_OR_FATAL'}elseif($Journey -and $JourneyResult -and $JourneyResult.complete -and $JourneyResult.passed){'PROGRAMMATIC_NATIVE_RULER_WORLD_RULER_PASS'}elseif($Journey -and $JourneyResult -and $JourneyResult.complete){'NATIVE_RULER_WORLD_RULER_FAILED'}elseif($Journey){'NATIVE_JOURNEY_UNVERIFIED'}elseif($ObservedShot){'WIN64_GAME_BOOT_SCREENSHOT_CAPTURED'}elseif($LogContainsMap){'MAP_BOOT_LOG_ONLY'}else{'BOOT_UNVERIFIED'}
  }
  $Report | ConvertTo-Json -Depth 5 |Set-Content $Result -Encoding UTF8
  Write-Host ($Report | ConvertTo-Json -Depth 5)
} finally {
  if (!$Process.HasExited) {
    Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
    Write-Host "Stopped only isolated Win64 game PID $($Process.Id)"
  }
}
