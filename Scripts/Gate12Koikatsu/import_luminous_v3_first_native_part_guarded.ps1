param([int]$MaxSeconds=330)
$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$f='D:\UnrealProjects\Offline-Game'
$root='D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011'
$manifestFile="$root\UE_INTAKE\LuminousValentineV3Clothed\shards\manifest_full_fidelity_primitives.json"
$qa="$w\Saved\Gate12Koikatsu\LuminousV3\FirstNativePart"
New-Item -ItemType Directory -Path $qa -Force|Out-Null
$manifest=Get-Content $manifestFile -Raw |ConvertFrom-Json
if($manifest.status -ne 'SOURCE_LOSSLESS_SHARDS_VERIFIED' -or $manifest.character_id -ne 'LuminousValentineV3Clothed'){throw 'Wrong real source'}
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Wrong Gate12 branch'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Frozen Foundation mismatch'}
$part=@($manifest.shards|Where-Object{$_.mesh_index -eq 3 -and $_.primitive_index -eq 0})
if($part.Count -ne 1){throw 'Could not uniquely select source 03-00'}
$x=$part[0]
if(!$x.primitives_equal -or !$x.source_image_payloads_preserved){throw 'Source bone/morph/texture invariant failed'}
$source=[IO.Path]::GetFullPath([string]$x.path)
if(!(Test-Path -LiteralPath $source)){throw 'True source shard absent'}
if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant() -ne ([string]$x.sha256).ToLowerInvariant()){throw 'Real shard SHA mismatch'}
$assetPath='/Game/Experimental/KoikatsuConverted/LuminousValentineV3Clothed/Parts/Mesh003Prim000'
$assetDisk=Join-Path $w 'Content\Experimental\KoikatsuConverted\LuminousValentineV3Clothed\Parts\Mesh003Prim000'
if(Test-Path $assetDisk){throw 'Refusing to overwrite prior Luminous native content'}
if(@(Get-Process -Name 'blender','UnrealEditor','UnrealEditor-Cmd' -ErrorAction SilentlyContinue).Count){throw 'Defer editor while another heavy editor runs'}
$os=Get-CimInstance Win32_OperatingSystem
if($os.FreePhysicalMemory/1024 -lt 2900 -or $os.FreeVirtualMemory/1024 -lt 5500){throw 'Host lacks safe headroom for genuine one-part UE skeletal native import'}
$settings="$qa\first_native_import_settings.json"
@{ImportGroups=@(@{GroupName='LuminousV3ClothedM03P00';Filenames=@($source);DestinationPath=$assetPath;bReplaceExisting=$false;bSkipReadOnly=$true})}|ConvertTo-Json -Depth 6|Set-Content $settings -Encoding UTF8
$exe='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project=Join-Path $w 'OfflineGame.uproject'
$log="$qa\first_real_luminous_native_unreal.log"
$args=@(('"' + $project + '"'),'-run=ImportAssets',('-importSettings="' + $settings + '"'),'-unattended','-nop4','-nullrhi','-nosplash',('-abslog="' + $log + '"'))
$p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput "$qa\first_unreal_stdout.txt" -RedirectStandardError "$qa\first_unreal_stderr.txt"
Write-Output "REAL_LUMINOUS_NATIVE_IMPORT_PID=$($p.Id)"
$start=[DateTime]::UtcNow
$status='RUNNING'
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 4
  $p.Refresh()
  $os=Get-CimInstance Win32_OperatingSystem
  $ram=[math]::Round($os.FreePhysicalMemory/1024)
  $commit=[math]::Round($os.FreeVirtualMemory/1024)
  $t=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
  if(($t%24) -le 4){Write-Output "LUMINOUS_NATIVE_PART_T=$t FREE_RAM=$ram COMMIT=$commit"}
  if($ram -lt 385 -or $commit -lt 1550){$status='STOPPED_MEMORY_GUARD';break}
  if($t -gt $MaxSeconds){$status='STOPPED_TIME_GUARD';break}
 }
 if($status -eq 'RUNNING'){$status='PROCESS_EXITED'}
}finally{
 if(-not $p.HasExited){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
}
$assets=@(Get-ChildItem $assetDisk -Recurse -File -Filter '*.uasset' -ErrorAction SilentlyContinue)
$receipt=[ordered]@{schema='G12_LUMINOUS_REAL_ONEPART_UE_NATIVE_IMPORT_1'
 source_glb_sha256=$manifest.original_glb_sha256
 source_state='CLOTHED'
 source_shard_sha256=$x.sha256
 source_morph_target_count=[int]$x.shard_primitive_counts[0].target_count
 source_textures_unmodified=[bool]$x.source_image_payloads_preserved
 ue_asset_path=$assetPath
 ue_editor_process_status=$status
 native_uasset_count=$assets.Count
 native_ue_skeletal_mesh_cold_verified=$false
 source_character_clothing_states_one_identity=$true
 other_44_parts_native_import='NOT_TESTED'
 android_cook='NOT_TESTED'
 device_runtime='NOT_TESTED'
 gate11_artifact_ready=$false
 timestamp_utc=[DateTime]::UtcNow.ToString('o')}
$receipt|ConvertTo-Json -Depth 8|Set-Content "$qa\first_native_import_receipt.json" -Encoding UTF8
Write-Output "NEW_LUMINOUS_NATIVE_RESULT=$status NATIVE_UASSETS=$($assets.Count)"
if($status -ne 'PROCESS_EXITED' -or $assets.Count -lt 1){exit 16}
Write-Output 'REAL_LUMINOUS_SOURCE_IMPORTED_UASSETS_PENDING_FRESH_EDITOR_COLD_VERIFY'
