# Exact original-card GLB -> real Unreal Engine SkeletalMesh packages.
# One true source mesh per editor commandlet to limit Windows commit budget.
param([string]$Plan='D:\UnrealProjects\G12_LuminousNativeImport_20261031\Saved\G12_AllGenuineOriginalCardNativeImportPlan.json')
$ErrorActionPreference='Stop'
$root='D:\UnrealProjects\G12_LuminousNativeImport_20261031'
$project=Join-Path $root 'G12_LuminousNativeImport.uproject'
$engine='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$ledger=Join-Path $root 'Saved\G12_OriginalCard_UnrealNativeImport.jsonl'
$single=Join-Path $root 'Saved\G12_OneSourceMeshOnly.json'
if(!(Test-Path $project) -or !(Test-Path $engine)){throw 'Unreal native project/engine missing'}
$groups=@((Get-Content $Plan -Raw|ConvertFrom-Json).ImportGroups)
if($groups.Count -lt 12){throw 'Not all three genuine-card multi-mesh models admitted'}
foreach($group in $groups){
 $name=[string]$group.GroupName
 if($name -notmatch '^ExactCard_(Awakened|Berserker|Grand)_[0-9][0-9]$'){throw "Unexpected asset source: $name"}
 $source=[string]$group.Filenames[0]
 $sourcePath=Resolve-Path -LiteralPath $source -ErrorAction Stop
 $approvedPattern=(Join-Path $root 'AuthenticClothedOriginalCard3D')+'*'
 if($sourcePath.Path -notlike $approvedPattern){
  throw "Not an original-card source export: $source"
 }
 $package=[string]$group.DestinationPath
 if($package -notmatch '^/Game/NoWorld/Characters/Luminous/(Awakened|Berserker|Grand)/ExactOriginalClothed/Part_[0-9][0-9]$'){
  throw "Unapproved Unreal import destination: $package"
 }
 $relative=$package.Replace('/Game/','').Replace('/','\')
 $dir=Join-Path (Join-Path $root 'Content') $relative
 $existing=@(Get-ChildItem $dir -Recurse -Filter '*.uasset' -File -ErrorAction SilentlyContinue|Where-Object {$_.FullName -like '*SkeletalMeshes*' -and $_.Name -notmatch '_Skeleton|_PhysicsAsset'})
 if($existing.Count -gt 0){Write-Host 'PREVIOUS_NATIVE_MESH_PRESERVED' $name;continue}
 @{ImportGroups=@($group)}|ConvertTo-Json -Depth 9|Set-Content -LiteralPath $single -Encoding UTF8
 $log=Join-Path $root ("Saved\G12RealNative_"+$name+".log")
 Write-Host 'BEGIN_AUTHENTIC_ORIGINAL_NATIVE_SKELETAL_IMPORT' $name
 & $engine $project -run=ImportAssets ('-importSettings='+$single) -unattended -nop4 -nullrhi -nosplash ('-abslog='+$log) 2>&1 | Out-Null
 $exit=$LASTEXITCODE
 $meshes=@(Get-ChildItem $dir -Recurse -Filter '*.uasset' -File -ErrorAction SilentlyContinue|Where-Object {$_.FullName -like '*SkeletalMeshes*' -and $_.Name -notmatch '_Skeleton|_PhysicsAsset'})
 $skeletons=@(Get-ChildItem $dir -Recurse -Filter '*_Skeleton.uasset' -File -ErrorAction SilentlyContinue)
 $pass=$meshes.Count -gt 0 -and $skeletons.Count -gt 0 -and $exit -eq 0
 $row=@{name=$name;native_unreal_skeletal_mesh=$pass;exit_code=$exit;meshes=$meshes.Count;skeletons=$skeletons.Count;source=$source;package=$package;timestamp_utc=[DateTime]::UtcNow.ToString('o')}
 ($row|ConvertTo-Json -Compress)|Add-Content -LiteralPath $ledger -Encoding UTF8
 Write-Host 'ORIGINAL_CARD_REAL_UNREAL_SKELETAL_IMPORT_RESULT' $name $pass 'exit' $exit
}
Write-Host 'ALL_AUTHENTIC_SOURCE_MESH_IMPORTS_EVALUATED'
