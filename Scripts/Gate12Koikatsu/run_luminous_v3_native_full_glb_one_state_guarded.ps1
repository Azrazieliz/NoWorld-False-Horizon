param(
 [Parameter(Mandatory=$true)]
 [ValidateSet('CLOTHED','UNDERWEAR','NUDE')][string]$State,
 [int]$MaxSeconds=640
)
$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$f='D:\UnrealProjects\Offline-Game'
$source='D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011'
$exe='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$script="$w\Scripts\Gate12Koikatsu\import_luminous_v3_full_glb_real_ue583.py"
$audit="$source\BLENDER_FRESH\$State\GATE12_GLTF_REAL_GEOMETRY_MORPH_AUDIT.json"
$root="$w\Saved\Gate12Koikatsu\LuminousV3\$State"
$receipt="$root\native_full_glb_ue583_import_receipt.json"
New-Item -ItemType Directory -Force $root |Out-Null
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Wrong isolated worktree'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Original frozen project modified'}
$lock=(& 'D:\Tools\Python311\python.exe' "$w\Scripts\Gate12Koikatsu\verify_kk_nonanimation_baseline.py"|ConvertFrom-Json)
if($lock.status -ne 'PASS_NONANIMATION_BASELINE_BYTE_LOCK_ONLY'){throw 'Locked asset reference SHA drift'}
if(!(Test-Path $audit)){throw 'Real Party conversion not yet validated'}
$sourceAudit=Get-Content $audit -Raw|ConvertFrom-Json
if($sourceAudit.status -ne 'PASS_REAL_SOURCE_GEOMETRY_SKINS_UVS_IMAGES_WITH_MORPH_LINKS'){throw 'Invalid source morphology status'}
if(Test-Path $receipt){throw "Refuse to repeat an import; inspect existing receipt $receipt"}
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count){throw 'Unreal Editor already active'}
$o=Get-CimInstance Win32_OperatingSystem
$free=[math]::Round($o.FreePhysicalMemory/1024)
$commit=[math]::Round($o.FreeVirtualMemory/1024)
if($free -lt 2450 -or $commit -lt 4000){throw "Insufficient headroom for full quality Unreal import $free MiB RAM, $commit MiB commit"}
$env:G12_LUMINOUS_EXPORT_STATE=$State
$log="$root\Unreal_Luminous_Real_Party_Import.log"
$out="$root\Unreal_stdout.txt"
$err="$root\Unreal_stderr.txt"
$args=@(('"' + "$w\OfflineGame.uproject" + '"'),'-run=pythonscript',('-script="' + $script + '"'),'-unattended','-nullrhi','-nop4','-nosplash','-NoShaderCompile',('-abslog="' + $log + '"'))
$p=Start-Process -FilePath $exe -ArgumentList $args -PassThru -WindowStyle Hidden -RedirectStandardOutput $out -RedirectStandardError $err
Write-Output "LUMINOUS_$($State)_NATIVE_UNREAL_IMPORT_PID=$($p.Id)"
$start=[DateTime]::UtcNow
$status='RUNNING'
$samples=[System.Collections.Generic.List[object]]::new()
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 3
  $p.Refresh()
  $o=Get-CimInstance Win32_OperatingSystem
  $seconds=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
  $ram=[math]::Round($o.FreePhysicalMemory/1024)
  $commit=[math]::Round($o.FreeVirtualMemory/1024)
  $samples.Add([pscustomobject]@{seconds=$seconds;freePhysicalMiB=$ram;freeCommitMiB=$commit})
  if(($seconds%27) -lt 3){
    Write-Output "LUMINOUS_$($State)_UE_IMPORT_SECONDS=$seconds RAM=$ram COMMIT=$commit"
    Get-Content $log -Tail 2 -ErrorAction SilentlyContinue|ForEach-Object{Write-Output $_}
  }
  if($ram -lt 380 -or $commit -lt 1300){$status='STOPPED_RESOURCE_GUARD';break}
  if($seconds -gt $MaxSeconds){$status='STOPPED_TIME_GUARD';break}
 }
 if($status -eq 'RUNNING'){$status='EXITED'}
}finally{
 if(-not $p.HasExited){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
 [ordered]@{state=$State;process_status=$status;started_utc=$start.ToString('o');end_utc=[DateTime]::UtcNow.ToString('o');samples=$samples.ToArray()}|
   ConvertTo-Json -Depth 6|Set-Content "$root\native_direct_import_process_watchdog.json" -Encoding UTF8
}
Write-Output "LUMINOUS_$($State)_NATIVE_IMPORT_PROCESS=$status"
if(!(Test-Path $receipt)){Write-Output 'NO_NATIVE_UE_IMPORT_RECEIPT';exit 18}
$res=Get-Content $receipt -Raw|ConvertFrom-Json
Write-Output "LUMINOUS_$($State)_UE_IMPORT_RESULT=$($res.status) SKELETAL_MESHES=$($res.skeletal_mesh_count) MORPH_TARGETS=$($res.total_morph_asset_refs)"
if($res.status -ne 'PASS_REAL_NATIVE_UNREAL_SKELETAL_MORPH_TEXTURE_ASSETS_NOT_GAMEPLAY'){exit 19}
