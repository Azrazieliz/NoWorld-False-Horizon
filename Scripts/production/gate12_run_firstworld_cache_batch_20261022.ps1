param([int]$FromIndex=1,[int]$ToIndex=6)
$ErrorActionPreference='Stop'
$r='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$ue='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$py="$r\Scripts\production\gate12_incremental_original_world_mesh_cache_20261022.py"
$receipts="$r\Saved\Gate12Readiness"
if($FromIndex -lt 0 -or $ToIndex -gt 12 -or $FromIndex -gt $ToIndex){throw 'Invalid exact source mesh index interval'}
if(!(Test-Path $py) -or !(Test-Path $ue)){throw 'Original UE asset tool or approved source script absent'}
Set-Item -Path 'Env:UE-ZenDataPath' -Value "$r\Saved\Gate12NativeZenData"
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value "$r\Saved\Gate12NativeDDC"
foreach($i in $FromIndex..$ToIndex){
 $tag='{0:d2}' -f $i
 $receipt="$receipts\Gate12_MeshCache_$($tag)_20261022.json"
 if(Test-Path $receipt){
  $done=Get-Content $receipt -Raw|ConvertFrom-Json
  if($done.status -eq 'ORIGINAL_MESH_LOADED_LOD_QUERIED_NO_ASSET_OVERWRITE' -and $done.lod0_vertices -gt 0){
   Write-Output "PRESERVED_APPROVED_MESH=$tag VERTICES=$($done.lod0_vertices)"
   continue
  }
 }
 $free=[Math]::Round((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1024)
 if($free -lt 2350){throw "Aborting before mesh $tag; available RAM only $free MiB"}
 $env:G12_MESH_INDEX=[string]$i
 $log="$receipts\Gate12_MeshCache_$($tag)_20261022.log"
 & $ue "$r\OfflineGame.uproject" '-run=pythonscript' "-script=$py" '-unattended' '-NullRHI' '-NoSound' '-nop4' "-ZenDataPath=$r\Saved\Gate12NativeZenData" "-LocalDataCachePath=$r\Saved\Gate12NativeDDC" *> $log
 $code=$LASTEXITCODE
 if(!(Test-Path $receipt)){throw "Missing acceptance receipt for source mesh $tag exit $code"}
 $result=Get-Content $receipt -Raw|ConvertFrom-Json
 Write-Output "SOURCE_MESH=$tag ENGINE_EXIT=$code STATUS=$($result.status) VERTICES=$($result.lod0_vertices)"
 if($code -ne 0 -or $result.status -ne 'ORIGINAL_MESH_LOADED_LOD_QUERIED_NO_ASSET_OVERWRITE'){throw "Stopped on actual failure at $tag, source untouched"}
}
Write-Output "G12_APPROVED_SOURCE_MESH_BATCH_FINISHED $FromIndex..$ToIndex"
