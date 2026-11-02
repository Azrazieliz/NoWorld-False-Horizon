param([int]$MaxSeconds=160,[string]$Worktree='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010')
$ErrorActionPreference='Stop'
$w=[IO.Path]::GetFullPath($Worktree)
$f='D:\UnrealProjects\Offline-Game'
$root="$w\Saved\Gate12Koikatsu\MotionV3"
$receipt="$root\native_ue_onepart_refpose_audit.json"
$script="$w\Scripts\Gate12Koikatsu\motion_v3_inspect_native_onepart_ue583.py"
$exe='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$log="$root\native_onepart_readonly_unreal.log"
$watch="$root\native_onepart_watchdog.json"
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Wrong branch'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Frozen Foundation changed'}
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count){throw 'UE editor already open'}
$os=Get-CimInstance Win32_OperatingSystem
if($os.FreePhysicalMemory/1024 -lt 2200 -or $os.FreeVirtualMemory/1024 -lt 3500){throw 'Insufficient host headroom'}
$env:G12_GATE12_WORKTREE=$w
if(Test-Path $receipt){Remove-Item $receipt -Force}
$args=@(('"'+(Join-Path $w 'OfflineGame.uproject')+'"'),'-run=pythonscript',('-script="'+$script+'"'),'-unattended','-nullrhi','-nop4','-nosplash','-NoShaderCompile',('-abslog="'+$log+'"'))
$p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput "$root\native_onepart_stdout.txt" -RedirectStandardError "$root\native_onepart_stderr.txt"
$start=[DateTime]::UtcNow
$state='RUNNING'
$samples=[System.Collections.Generic.List[object]]::new()
Write-Output "NATIVE_V3_READONLY_RIG_PID=$($p.Id)"
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 3
  $p.Refresh()
  $o=Get-CimInstance Win32_OperatingSystem
  $ram=[math]::Round($o.FreePhysicalMemory/1024)
  $commit=[math]::Round($o.FreeVirtualMemory/1024)
  $elapsed=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
  $samples.Add([pscustomobject]@{elapsedSec=$elapsed;freeRamMB=$ram;freeCommitMB=$commit})
  if($ram -lt 390 -or $commit -lt 1900){$state='STOPPED_RESOURCE_GUARD';break}
  if($elapsed -gt $MaxSeconds){$state='STOPPED_TIMEOUT';break}
  if(($elapsed%18) -lt 3){Write-Output "V3_REFPOSE_T=$elapsed FREE_RAM_MB=$ram COMMIT_MB=$commit"}
 }
 if($state -eq 'RUNNING'){$state='EXITED'}
}finally{
 if(-not $p.HasExited){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
 [ordered]@{status=$state;startedUtc=$start.ToString('o');completedUtc=[DateTime]::UtcNow.ToString('o');samples=$samples.ToArray()}|ConvertTo-Json -Depth 7|Set-Content $watch -Encoding UTF8
}
Write-Output "NATIVE_V3_REFPOSE_PROCESS=$state"
if(!(Test-Path $receipt)){Write-Output 'NATIVE_V3_NO_UE_PYTHON_RECEIPT';exit 20}
$j=Get-Content $receipt -Raw|ConvertFrom-Json
Write-Output "NATIVE_V3_REFPOSE_STATUS=$($j.status) BONES=$(@($j.bone_poses).Count)"
if($j.status -ne 'PASS_READONLY_UE_ONEPART_REFERENCE_POSE'){exit 21}
if($state -ne 'EXITED'){exit 22}
