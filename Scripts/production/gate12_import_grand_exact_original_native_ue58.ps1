# Direct asset authoring only: source-card-matched Grand -> native UE SkeletalMesh.
# No UE C++ compilation, packaging, runtime QA or tests in this script.
param([int]$StartGroup=0,[int]$EndGroup=8)
$ErrorActionPreference='Stop'
$sourceRoot='D:\UnrealProjects\G12_LuminousNativeImport_20261031'
$gameRoot='D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031'
$editor='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project=Join-Path $sourceRoot 'G12_LuminousNativeImport.uproject'
$original=Join-Path $sourceRoot 'AuthenticClothedOriginalCard3D\Grand'
$sourceStream=Join-Path $sourceRoot 'Saved\GrandExactUnitySourceStream'
$manifest=Join-Path $original 'GRAND_EXACT_ORIGINAL_SOURCE_BLENDER_MANIFEST.json'
$expected='26c48e1c03c154f9f39a0b06afaade53423c11dbd079604e430162eb4cae5353'
if(!(Test-Path $editor) -or !(Test-Path $project) -or !(Test-Path $manifest)) { throw 'Native authoring engine or original-card conversion is unavailable.' }
if((Get-Content (Join-Path $sourceStream 'GATE12_GRAND_EXACT_SOURCE_STREAM_COMPLETE.sha256') -Raw).Trim() -ne $expected){throw 'Original Grand card extraction is not complete'}
$proof=Get-Content $manifest -Raw|ConvertFrom-Json
if($proof.card_sha256 -ne $expected -or $proof.original_active_renderer_count -ne 57 -or @($proof.groups).Count -ne 9){ throw 'Exact original-card grouped source manifest invalid' }
$logDir=Join-Path $sourceRoot 'Saved'
$ledger=Join-Path $logDir 'G12_Grand_ExactOriginalNativeAuthoring.jsonl'
for($i=$StartGroup;$i -le $EndGroup;$i++){
 if($i -lt 0 -or $i -gt 8){throw 'Only the nine original Grand source groups are authorized'}
 $n=$i.ToString('D2')
 $glb=Join-Path $original ('Luminous_Grand_ExactOriginalClothed_Part_'+$n+'.glb')
 if(!(Test-Path $glb) -or (Get-Item $glb).Length -lt 30000){throw ('Missing original skinned Grand GLB group '+$n)}
 $name='ExactCard_Grand_'+$n
 $dest='/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_'+$n
 $sourceNative=Join-Path $sourceRoot ('Content\NoWorld\Characters\Luminous\Grand\ExactOriginalClothed\Part_'+$n)
 $gameNative=Join-Path $gameRoot ('Content\NoWorld\Characters\Luminous\Grand\ExactOriginalClothed\Part_'+$n)
 if(Test-Path $gameNative){
  $existing=@(Get-ChildItem $gameNative -Recurse -File -Filter '*.uasset' -ErrorAction SilentlyContinue)
  if($existing.Count -gt 0){Write-Output ('ORIGINAL_GRAND_NATIVE_GROUP_ALREADY_PRESERVED='+$n);continue}
 }
 $plan=Join-Path $logDir ('G12_Grand_OriginalGroup_'+$n+'.json')
 $entry=@{ImportGroups=@(@{GroupName=$name;Filenames=@($glb);DestinationPath=$dest;bReplaceExisting=$false;bSkipReadOnly=$true})}
 [IO.File]::WriteAllText($plan,($entry|ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
 $log=Join-Path $logDir ('G12_Grand_NativeAuthoring_'+$n+'.log')
 Write-Output ('IMPORTING_ORIGINAL_GRAND_SKINNED_CONTENT='+$n)
 & $editor $project -run=ImportAssets ('-importSettings='+$plan) -unattended -nop4 -nullrhi -nosplash ('-abslog='+$log) 2>&1|Out-Null
 $exit=$LASTEXITCODE
 $packages=@(Get-ChildItem $sourceNative -Recurse -File -Filter '*.uasset' -ErrorAction SilentlyContinue)
 $row=@{group=$n;source_original_card_sha256=$expected;source_glb=$glb;native_content_count=$packages.Count;native_exit_code=$exit;native_destination=$gameNative;utc=[DateTime]::UtcNow.ToString('o')}
 $nativeMeshes=@($packages|Where-Object{$_.FullName -match '\\SkeletalMeshes\\' -and $_.Name -notmatch '_Skeleton|_PhysicsAsset'})
 $nativeSkeletons=@($packages|Where-Object{$_.Name -match '_Skeleton[.]uasset$'})
 if($nativeMeshes.Count -gt 0 -and $nativeSkeletons.Count -gt 0){
  if(!(Test-Path $gameNative)){New-Item $gameNative -ItemType Directory -Force|Out-Null}
  Copy-Item (Join-Path $sourceNative '*') -Destination $gameNative -Recurse -Force
  $row.actual_game_packages=@(Get-ChildItem $gameNative -Recurse -File -Filter '*.uasset'|ForEach-Object{$_.FullName.Substring($gameRoot.Length+1)})
  $row.status='NATIVE_ASSETS_AUTHORED_IN_GAME_SOURCE'
 } else {
  $row.status='NATIVE_IMPORT_BLOCKED'
 }
 ($row|ConvertTo-Json -Depth 7 -Compress)|Add-Content -LiteralPath $ledger
 Write-Output ('GRAND_ORIGINAL_NATIVE_RESULT='+($row|ConvertTo-Json -Compress -Depth 5))
 if($row.status -ne 'NATIVE_ASSETS_AUTHORED_IN_GAME_SOURCE'){throw ('Native source import failed for original Grand part '+$n)}
}
