<#
.SYNOPSIS
 Source-only, NO-COOK UE 5.8.3 native regression runner for Gate 12.
 Always use the original protected worktree. No UAT, cook, package, deploy,
 APK, adb, saves, Foundation checkout, or clean.
#>
param(
  [ValidateSet('Gate12','Full')]
  [string]$Scope='Gate12'
)
$ErrorActionPreference='Stop'
$Root=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$Editor='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $Root 'OfflineGame.uproject'
if (!(Test-Path $Editor)) { throw "Native UnrealEditor-Cmd.exe missing: $Editor" }
if (!(Test-Path $Project)) { throw "Existing production project missing: $Project" }
$Branch=(& git -C $Root branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $Branch -ne 'production/gate12-koikatsu-ue583-fixture-20261010') {
 throw "Wrong worktree or branch; refusing verification: $Branch"
}
if (Get-Process UnrealEditor-Cmd,UnrealEditor -ErrorAction SilentlyContinue) {
 throw 'An Unreal Editor process is already running; refusing parallel low-RAM regression.'
}
$Pattern=if ($Scope -eq 'Full') {'OfflineGame'} else {'OfflineGame.Gate12'}
$Stamp=Get-Date -Format 'yyyyMMdd_HHmmss'
$Destination=Join-Path $Root "Saved\Gate12Readiness\Native_$($Scope)_$Stamp"
New-Item -ItemType Directory -Path $Destination -Force | Out-Null
$Transcript=Join-Path $Destination 'native_ue583_stdout.log'
$Arguments=@(
 $Project,
 '-NullRHI',
 '-unattended',
 '-nopause',
 '-NoSound',
 '-NoSplash',
 '-NoP4',
 "-ExecCmds=Automation RunTests $Pattern; Quit",
 "-ReportExportPath=$Destination",
 "-Log=$Destination\native_editor.log"
)
Write-Host "Running NO-COOK native automation: $Pattern"
& $Editor @Arguments 2>&1 | Tee-Object -FilePath $Transcript
$Exit=$LASTEXITCODE
$Index=Join-Path $Destination 'index.json'
$Summary=[ordered]@{
 scope=$Scope
 pattern=$Pattern
 utc=(Get-Date).ToUniversalTime().ToString('o')
 compiled_current_source_required=$true
 editor_exit=$Exit
 report_path=$Index
 report_exists=(Test-Path $Index)
 passed=$false
 reason=''
}
if (!(Test-Path $Index)) {
 $Summary.reason='Native report not generated; NEVER treat missing test evidence as PASS'
} elseif ($Exit -ne 0) {
 $Summary.reason="UnrealEditor-Cmd returned failure exit code: $Exit"
} else {
 $Report=Get-Content $Index -Raw | ConvertFrom-Json
 $Failed=[int]($Report.failed)
 $Passed=[int]($Report.succeeded)
 $WithWarnings=[int]($Report.succeededWithWarnings)
 $Summary['native_succeeded']=$Passed
 $Summary['native_warnings']=$WithWarnings
 $Summary['native_failed']=$Failed
 $Summary['passed']=($Failed -eq 0 -and ($Passed+$WithWarnings) -gt 0)
 if (!$Summary.passed) {
  $Summary.reason='Native report shows failures or zero executed tests'
 }
}
$Summary | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $Destination 'verification_summary.json') -Encoding UTF8
Write-Host ($Summary | ConvertTo-Json -Depth 5)
if (!$Summary.passed) { exit 1 }
exit 0
