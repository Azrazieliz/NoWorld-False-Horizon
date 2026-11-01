# Build canonical original Grand whole character registry from the real 57 Unreal skeletal mesh files.
$ErrorActionPreference='Stop'
$root='D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031'
$assetRoot=Join-Path $root 'Content\NoWorld\Characters\Luminous\Grand\ExactOriginalClothed'
$cpp=Join-Path $root 'Source\OfflineGame\Private\Koikatsu\OGExactOriginalLuminousParts.cpp'
$receiptDir=Join-Path $root 'docs\production\gate12\evidence'
$receipt=Join-Path $receiptDir 'G12_Grand_ExactCard_57Renderer_NativeRegistry_20261101.json'
$expectedCard='26c48e1c03c154f9f39a0b06afaade53423c11dbd079604e430162eb4cae5353'
$stream='D:\UnrealProjects\G12_LuminousNativeImport_20261031\Saved\GrandExactUnitySourceStream'
$card=Join-Path $root 'Assets\NoWorld\Luminous\KoikatsuOriginalCards\LUMINOUS_GRAND_CONVERGENCE_CLOTHED_KK.png'
if((Get-FileHash $card -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedCard){throw 'Original Grand card changed'}
if((Get-Content (Join-Path $stream 'GATE12_GRAND_EXACT_SOURCE_STREAM_COMPLETE.sha256') -Raw).Trim() -ne $expectedCard){throw 'Exact original extraction incomplete'}
$meshes=@(Get-ChildItem $assetRoot -Recurse -File -Filter '*.uasset' |Where-Object{$_.DirectoryName -match '\\SkeletalMeshes$' -and $_.Name -notmatch '_Skeleton|_PhysicsAsset'})
$groups=@(Get-ChildItem $assetRoot -Directory |Sort-Object Name)
if($groups.Count -ne 9 -or $meshes.Count -ne 57){throw ('Actual native source mesh count unexpected: '+$groups.Count+'/'+$meshes.Count)}
$rows=@()
foreach($group in $groups){
 $members=@($meshes |Where-Object{$_.FullName.StartsWith($group.FullName+'\')})
 if($members.Count -lt 1){throw ('Missing actual meshes in '+$group.Name)}
 if(@(Get-ChildItem $group.FullName -Recurse -File -Filter '*_Skeleton.uasset').Count -lt 1){throw ('Missing actual skeleton in '+$group.Name)}
 foreach($mesh in $members){
  $match=[regex]::Match($mesh.BaseName,'Renderer_([0-9]{3})$')
  if(!$match.Success){
   if($group.Name -ne 'Part_00'){throw ('Unrecognized actual mesh source '+$mesh.Name)}
   $sourceId=18
  } else {$sourceId=[int]$match.Groups[1].Value}
  $sourceFile=Join-Path $stream ('ExactGrand_Renderer_'+$sourceId.ToString('D3')+'.unitymesh')
  if(!(Test-Path $sourceFile)){throw ('Original source renderer missing '+$sourceId)}
  $relative=$mesh.FullName.Substring((Join-Path $root 'Content').Length+1).Replace('\','/')
  $noExt=$relative.Substring(0,$relative.Length-7)
  $soft='/Game/'+$noExt+'.'+$mesh.BaseName
  if($soft.Contains('..') -or $soft -notmatch '^/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_'){throw ('Bad native mesh object path '+$soft)}
  $rows+=[pscustomobject]@{renderer_index=$sourceId;group=$group.Name;soft_object_path=$soft;uasset_path=$relative;source_sha256=(Get-FileHash $sourceFile -Algorithm SHA256).Hash.ToLowerInvariant();uasset_bytes=$mesh.Length}
 }
}
$rows=@($rows|Sort-Object renderer_index)
if(@($rows|Select-Object -ExpandProperty renderer_index -Unique).Count -ne 57){throw 'Source renderer indices duplicated'}
$raw=[IO.File]::ReadAllText($cpp)
if($raw.Contains('static const TCHAR* const GGrandParts[]')){throw 'Grand source was already registered'}
$lines=@('static const TCHAR* const GGrandParts[] = {')
foreach($r in $rows){$lines+='    TEXT("'+$r.soft_object_path+'"),'}
$lines+='};'
$fragment=($lines -join [string][char]10)+[string][char]10+[string][char]10
$anchor='static const FExactForm GForms[] = {'
if(!$raw.Contains($anchor)){throw 'Shared registry changed'}
$raw=$raw.Replace($anchor,$fragment+$anchor)
$old=' { TEXT("character:luminous_valentine.version_berserker_v4"), 882, GBerserkerParts, UE_ARRAY_COUNT(GBerserkerParts) },'
if(!$raw.Contains($old)){throw 'Canonical Berserker record changed'}
$raw=$raw.Replace($old,$old+[string][char]10+' { TEXT("character:luminous_valentine.version_grand_v4"), 561, GGrandParts, UE_ARRAY_COUNT(GGrandParts) },')
if(!$raw.Contains('return Form.Count == 9;')){throw 'Shared form count contract changed'}
$raw=$raw.Replace('return Form.Count == 9;','return Form.Count == 9 || (VersionId == TEXT("character:luminous_valentine.version_grand_v4") && Form.Count == 57);')
[IO.File]::WriteAllText($cpp,$raw,[Text.UTF8Encoding]::new($false))
New-Item -ItemType Directory -Path $receiptDir -Force|Out-Null
$details=@{schema='noworld.original_grand_exact_card_57_native_character_registry.v1';form='Grand Convergence';ordinary_lineage=$false;exact_original_card_sha256=$expectedCard;original_active_unity_renderers=57;native_unreal_skeletal_meshes=$meshes.Count;native_asset_groups=$groups.Count;native_uasset_count=@(Get-ChildItem $assetRoot -Recurse -File -Filter '*.uasset').Count;source_original_armature_bones=561;registered_component_count=$rows.Count;compilation_and_gameplay_testing_performed=$false;original_meshes=$rows}
[IO.File]::WriteAllText($receipt,($details|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
Write-Output ('GRAND_REGISTERED_CHARACTER_MESHES='+$rows.Count)
Write-Output ('GRAND_NATIVE_RECEIPT='+$receipt)
