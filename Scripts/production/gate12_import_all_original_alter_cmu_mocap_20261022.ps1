# Original approved Alter state-specific genuine CMU motion source only.
# No synthetic animation authoring. Full original Blender rigs preserved.
$ErrorActionPreference='Stop'
$r='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$cmd='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$script="$r\Scripts\production\gate12_retarget_real_cmu_to_original_alter_state_20261022.py"
$readiness="$r\Saved\Gate12Readiness"
Set-Item 'Env:UE-ZenDataPath' "$r\Saved\Gate12NativeZenData"
Set-Item 'Env:UE-LocalDataCachePath' "$r\Saved\Gate12NativeDDC"
$completed=@()
foreach($state in @('CLOTHED','UNDERWEAR','NUDE')){
 foreach($motion in @('Walk','Run','Jump')){
  $receipt="$readiness\Gate12_Alter_$($state)_RealCMU_$($motion)_20261022.json"
  if(Test-Path $receipt){
   $present=Get-Content $receipt -Raw|ConvertFrom-Json
   if($present.status -eq 'IMPORTED_GENUINE_CMU_MOCAP_ON_EXACT_ALTER_RIG_PENDING_VISUAL'){
    $completed+=@{state=$state;motion=$motion;asset=$present.native_animsequence;tracks=$present.mapped_bone_count}
    Write-Output "EXISTING_GENUINE_MOCAP_PRESERVED=$state $motion"
    continue
   }
  }
  $ram=[math]::Round((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1024)
  if($ram -lt 2200){throw "Insufficient RAM for exact source $state $motion ($ram MiB)"}
  $env:G12_ALTER_MOCAP_STATE=$state
  $env:G12_ALTER_MOCAP_ACTION=$motion
  $log="$readiness\Gate12_Alter_$($state)_RealCMU_$($motion)_20261022.log"
  & $cmd "$r\OfflineGame.uproject" '-run=pythonscript' "-script=$script" '-unattended' '-nop4' '-NullRHI' '-NoSound' "-ZenDataPath=$r\Saved\Gate12NativeZenData" "-LocalDataCachePath=$r\Saved\Gate12NativeDDC" *> $log
  $exit=$LASTEXITCODE
  if(!(Test-Path $receipt)){throw "No exact native clip receipt: $state $motion"}
  $proof=Get-Content $receipt -Raw|ConvertFrom-Json
  if($exit -ne 0 -or $proof.status -ne 'IMPORTED_GENUINE_CMU_MOCAP_ON_EXACT_ALTER_RIG_PENDING_VISUAL'){
   throw "CMU retarget failed $state ${motion}: $($proof.error) UE=$exit"
  }
  $completed+=@{state=$state;motion=$motion;asset=$proof.native_animsequence;tracks=$proof.mapped_bone_count}
  Write-Output "NATIVE_SOURCE_MOCAP=$state $motion BONE_TRACKS=$($proof.mapped_bone_count) FRAMES=$($proof.frames)"
 }
}
if($completed.Count -ne 9){throw "Expected 9 original-source retargeting receipts, got $($completed.Count)"}
$seal=[ordered]@{
 schema='noworld.gate12.luminous_alter.allwardrobe.genuine_mocap.v1'
 status='NINE_GENUINE_CMU_NATIVE_ALTER_CLIPS_ALL_THREE_SOURCE_WARDROBE_RIGS'
 approved_source_geometry_unchanged=$true
 visual_quality_acceptance='NOT_YET_INSPECTED_ON_ACTUAL_GAME'
 clips=$completed
}
$seal|ConvertTo-Json -Depth 5|Set-Content "$readiness\Gate12_Alter_AllWardrobes_NineRealCMUClips_20261022.json" -Encoding UTF8
Write-Output "FINISHED_GENUINE_ALTER_CMU_ASSET_RETARGETING_9_OF_9"
