# Exact approved Alter original wardrobe native import; one FBX and one
# UnrealEditor process at a time on low-RAM workstation.
$ErrorActionPreference='Stop'
$r='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$art='D:\Tools\GameAssetPipeline\ProductionAutomation\Gate01\LuminousAlterCandidates_20261016\BLENDER_FRESH'
$ue='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$script="$r\Scripts\production\gate12_import_approved_alter_wardrobe_native_part_ue583_20261022.py"
$receipts="$r\Saved\Gate12Readiness"
if(!(Test-Path $ue) -or !(Test-Path $script)){throw 'Missing existing engine or native importer'}
Set-Item -Path 'Env:UE-ZenDataPath' -Value "$r\Saved\Gate12NativeZenData"
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value "$r\Saved\Gate12NativeDDC"
$results=@()
foreach($state in @('UNDERWEAR','NUDE')){
 $folder="$art\$state\UNREAL_ORIGINAL_PARTS"
 $sealed="$folder\GATE12_ALTER_$($state)_NATIVE_PARTS_20261022.json"
 if(!(Test-Path $sealed)){throw "State source part export receipt absent: $state"}
 $source=Get-Content $sealed -Raw |ConvertFrom-Json
 if($source.status -ne 'EXACT_ORIGINAL_APPROVED_ALTER_WARDROBE_SKINNED_PARTS_EXPORTED' -or $source.parts.Count -ne 6){
  throw "This is not a complete six-part independently authored wardrobe source: $state"
 }
 foreach($part in 0..5){
  $tag='{0:d2}' -f $part
  $receipt="$receipts\Gate12_Alter_$($state)_UE_OriginalPart_$($tag)_20261022.json"
  if(Test-Path $receipt){
   $prev=Get-Content $receipt -Raw | ConvertFrom-Json
   if($prev.status -eq 'ACTUAL_USER_CONFIRMED_ALTER_WARDROBE_PART_IMPORTED_NATIVE'){
    Write-Output "PRESERVED_ACCEPTED_UNREAL_NATIVE_ASSET=$state $tag"
    $results+=@{state=$state;part=$part;asset=$prev.native_mesh_path;existing=$true}
    continue
   }
  }
  $memory=[math]::Round((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1024)
  if($memory -lt 2200){throw "Abort $state $tag before import: only $memory MiB physical RAM"}
  $env:G12_ALTER_STATE=$state
  $env:G12_ALTER_PART=[string]$part
  $log="$receipts\Gate12_Alter_$($state)_UE_OriginalPart_$($tag)_20261022.log"
  & $ue "$r\OfflineGame.uproject" '-run=pythonscript' "-script=$script" '-unattended' '-nop4' '-NoSound' '-NullRHI' "-ZenDataPath=$r\Saved\Gate12NativeZenData" "-LocalDataCachePath=$r\Saved\Gate12NativeDDC" *> $log
  $exit=$LASTEXITCODE
  if(!(Test-Path $receipt)){throw "No native import receipt for $state $tag, UE=$exit"}
  $proof=Get-Content $receipt -Raw|ConvertFrom-Json
  if($proof.status -ne 'ACTUAL_USER_CONFIRMED_ALTER_WARDROBE_PART_IMPORTED_NATIVE'){
   throw "State $state part $tag native import did not meet original source quality: $($proof.status) $($proof.error)"
  }
  if($exit -ne 0){throw "UE returned $exit despite receipt for $state $tag"}
  Write-Output "ACTUAL_NATIVE_ALTER_UE_PART=$state $tag SRC_BONES=$($proof.source_armature_bones) UE_BONES=$($proof.native_skeleton_bones) MORPHS=$($proof.native_morph_targets)"
  $results+=@{state=$state;part=$part;asset=$proof.native_mesh_path;source_bones=$proof.source_armature_bones;native_bones=$proof.native_skeleton_bones}
 }
}
if($results.Count -ne 12){throw "Native wardrobe import count incomplete: $($results.Count)"}
$handoff=[ordered]@{
 schema='noworld.gate12.alter.approved.native.wardrobe.ue583.v1'
 status='TWELVE_GENUINE_ALTER_WARDROBE_NATIVE_SKELETAL_MESHES_IMPORT_RECEIPTS_CONFIRMED'
 base_source_substituted=$false
 original_source_geometry_replaced=$false
 missing_states=@()
 parts=$results
}
$handoff|ConvertTo-Json -Depth 6|Set-Content "$receipts\Gate12_Alter_AllThreeStates_SixPartUEIntake_20261022.json" -Encoding UTF8
'ALTER_ALL_NUDE_UNDERWEAR_NATIVE_IMPORTS_COMPLETED'
