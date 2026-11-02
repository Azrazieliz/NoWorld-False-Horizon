param(
 [ValidateSet('CLOTHED','UNDERWEAR','NUDE')][string]$State='CLOTHED',
 [ValidateRange(0,200)][int]$StartIndex=0,
 [ValidateRange(1,8)][int]$Count=1,
 [int]$MaxSeconds=560
)
$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$f='D:\UnrealProjects\Offline-Game'
$src="$w\Scripts\Gate12Koikatsu\import_luminous_v3_lossless_shards_one_batch_ue583.py"
$exe='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$q="$w\Saved\Gate12Koikatsu\LuminousV3\$State"
$out="$q\native_shards_$('{0:000}' -f $StartIndex)_count$('{0:00}' -f $Count)_ue583.json"
New-Item -ItemType Directory -Force $q |Out-Null
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Wrong worktree'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Frozen Foundation modified'}
$lock=(& 'D:\Tools\Python311\python.exe' "$w\Scripts\Gate12Koikatsu\verify_kk_nonanimation_baseline.py"|ConvertFrom-Json)
if($lock.status -ne 'PASS_NONANIMATION_BASELINE_BYTE_LOCK_ONLY'){throw 'Frozen input SHA contract failed'}
if(Test-Path $out){throw "Existing batch receipt; refusal to overwrite $out"}
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count){throw 'UE editor active'}
$o=Get-CimInstance Win32_OperatingSystem
if($o.FreePhysicalMemory/1024 -lt 2500 -or $o.FreeVirtualMemory/1024 -lt 3800){
 throw "Not enough memory $([math]::Round($o.FreePhysicalMemory/1024))MiB"}
$env:G12_LUMINOUS_STATE=$State
$env:G12_LUMINOUS_START_INDEX=[string]$StartIndex
$env:G12_LUMINOUS_MAX_IMPORT_PARTS=[string]$Count
$stdout="$q\ue_shards_$($StartIndex)_$($Count).stdout.log"
$stderr="$q\ue_shards_$($StartIndex)_$($Count).stderr.log"
$abslog="$q\ue_shards_$($StartIndex)_$($Count).unreal.log"
$args=@(('"' + "$w\OfflineGame.uproject" + '"'),'-run=pythonscript',('-script="' + $src + '"'),'-unattended','-nullrhi','-nop4','-nosplash','-NoShaderCompile',('-abslog="'+$abslog+'"'))
$p=Start-Process -FilePath $exe -ArgumentList $args -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
Write-Output "G12_LUMINOUS_$($State)_SHARDS_START=$StartIndex COUNT=$Count PID=$($p.Id)"
$start=[DateTime]::UtcNow;$processState='RUNNING'
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 3
  $p.Refresh()
  $o=Get-CimInstance Win32_OperatingSystem
  $secs=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
  $ram=[math]::Round($o.FreePhysicalMemory/1024)
  $commit=[math]::Round($o.FreeVirtualMemory/1024)
  if(($secs % 27) -lt 3){Write-Output "G12_UE_SHARD_T=$secs RAM=$ram COMMIT=$commit";Get-Content $abslog -Tail 2 -ErrorAction SilentlyContinue}
  if($ram -lt 365 -or $commit -lt 1200){$processState='STOPPED_RAM_GUARD';break}
  if($secs -gt $MaxSeconds){$processState='STOPPED_TIME_GUARD';break}
 }
 if($processState -eq 'RUNNING'){$processState='EXITED'}
}finally{
 if(-not $p.HasExited){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
 [ordered]@{status=$processState;state=$State;start_index=$StartIndex;count=$Count;
  utc=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json -Depth 4|
  Set-Content "$q\native_shards_$($StartIndex)_$($Count).watchdog.json" -Encoding UTF8
}
Write-Output "SHARD_NATIVE_PROCESS_STATUS=$processState"
if(Test-Path $out){$receipt=Get-Content $out -Raw|ConvertFrom-Json;
 Write-Output "SHARD_NATIVE_RESULT=$($receipt.status) IMPORTED=$(@($receipt.native_parts).Count)"
 $receipt.native_parts|Select index,mesh,primitive,ue_skeletal_mesh_count,ue_total_native_morph_targets,status |Format-Table -AutoSize
 if($receipt.status -notin @('PASS_THIS_BATCH_NATIVE_UE_SOURCE_AND_MORPHS','PARTIAL_NATIVE_SKELETAL_SOURCE_WITH_EXPRESSION_DIFFERENCES_NOT_PRODUCTION_ACCEPTED')){exit 22}
if($receipt.status -like 'PARTIAL*'){Write-Output "NATIVE_MORPH_QUALITY_DEFICIT_NOT_GATE11_ACCEPTED=$($receipt.total_effective_morphs_not_imported)"}
}else{Write-Output 'NO_FINAL_NATIVE_RECEIPT';exit 21}
