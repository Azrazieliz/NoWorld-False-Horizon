param([int]$MaxSeconds=220,[string]$Worktree='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
$ErrorActionPreference='Stop'
$w=[IO.Path]::GetFullPath($Worktree)
$f='D:\UnrealProjects\Offline-Game'
$root="$w\Saved\Gate12Koikatsu\MotionV3"
$receipt="$root\native_mocap_v3_onepart_cold_unreal.json"
$script="$w\Scripts\Gate12Koikatsu\verify_motion_v3_real_cmu_onepart_cold_ue583.py"
$exe='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Wrong experimental branch'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Frozen Foundation changed'}
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count){throw 'Unreal Editor already running'}
$os=Get-CimInstance Win32_OperatingSystem
if($os.FreePhysicalMemory/1024 -lt 2200 -or $os.FreeVirtualMemory/1024 -lt 3550){Write-Output 'V3_NATIVE_AUTHOR_NOT_STARTED_MEMORY';exit 12}
$env:G12_GATE12_WORKTREE=$w
if(Test-Path $receipt){Move-Item $receipt ("$root\native_mocap_v3_previous_attempt_"+[DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss')+'.json')}
$args=@(('"'+(Join-Path $w 'OfflineGame.uproject')+'"'),'-run=pythonscript',('-script="'+$script+'"'),'-unattended','-nullrhi','-nop4','-nosplash','-NoShaderCompile',('-abslog="'+("$root\native_v3cmu_cold_onepart_unreal.log")+'"'))
$p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput "$root\native_v3cmu_cold_onepart_stdout.txt" -RedirectStandardError "$root\native_v3cmu_cold_onepart_stderr.txt"
$start=[DateTime]::UtcNow;$state='RUNNING';$samples=[System.Collections.Generic.List[object]]::new()
Write-Output "V3_REAL_MOCAP_NATIVE_AUTHOR_PID=$($p.Id)"
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 3;$p.Refresh();$os=Get-CimInstance Win32_OperatingSystem
  $elapsed=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
  $ram=[math]::Round($os.FreePhysicalMemory/1024)
  $commit=[math]::Round($os.FreeVirtualMemory/1024)
  $samples.Add([pscustomobject]@{elapsed=$elapsed;freeRamMB=$ram;freeCommitMB=$commit})
  if($ram -lt 380 -or $commit -lt 1300){$state='STOPPED_RAM_GUARD';break}
  if($elapsed -gt $MaxSeconds){$state='STOPPED_TIMEOUT';break}
  if(($elapsed % 21) -lt 3){Write-Output "V3_MOCAP_UNREAL_SECONDS=$elapsed PHYSICAL_FREE_MB=$ram COMMIT_MB=$commit"}
 }
 if($state -eq 'RUNNING'){$state='EXITED'}
}finally{
 if(-not $p.HasExited){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
 [ordered]@{state=$state;startedUtc=$start.ToString('o');completedUtc=[DateTime]::UtcNow.ToString('o');samples=$samples.ToArray()}|ConvertTo-Json -Depth 5|Set-Content "$root\native_v3cmu_cold_onepart_watchdog.json" -Encoding UTF8
}
Write-Output "V3_NATIVE_AUTHOR_PROCESS=$state"
if(!(Test-Path $receipt)){Write-Output 'V3_NATIVE_AUTHOR_NO_RECEIPT';exit 18}
$o=Get-Content $receipt -Raw|ConvertFrom-Json
Write-Output "V3_NATIVE_AUTHOR_STATUS=$($o.status) SAVED_ASSETS=$(@($o.clips).Count)"
if($o.status -ne 'PASS_FRESH_UE5_NATIVE_3_MOCAP_CLIPS_ONEPART_PRE_VISUAL'){Write-Output $o.error;exit 19}
if(@($o.clips).Count -ne 3){throw 'Incomplete motion clip set'}
