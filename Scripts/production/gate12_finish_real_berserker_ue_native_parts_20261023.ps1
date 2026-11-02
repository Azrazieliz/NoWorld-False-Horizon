$ErrorActionPreference='Stop'
$r='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$editor='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$script="$r\Scripts\production\gate12_import_real_koikatsu_version_native_parts_unreal_20261023.py"
$env:G12_NATIVE_FORM='Berserker'
$env:G12_NATIVE_STATE='UNDERWEAR'
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value "$r\Saved\Gate12NativeDDC"
for($i=1;$i -le 5;$i++){
 $env:G12_NATIVE_PART=[string]$i
 $id=$i.ToString('00')
 $log="$r\Saved\Gate12Readiness\G12_Berserker_NativeUE_Part$($id)_20261023.log"
 $receipt="$r\Saved\Gate12Readiness\GATE12_Berserker_UNDERWEAR_NATIVE_UE_PART_$($id)_20261023.json"
 if(Test-Path $receipt){
  $prior=Get-Content $receipt -Raw | ConvertFrom-Json
  if($prior.status -eq 'REAL_SOURCE_NATIVE_UNREAL_SKELETAL_PART_IMPORTED'){
   Write-Output "NATIVE_PRESERVED=$id"
   continue
  }
  throw "Existing incomplete receipt must be investigated: $receipt"
 }
 & $editor "$r\OfflineGame.uproject" "-ExecutePythonScript=$script" -unattended -nullrhi -NoSound -nop4 -log *> $log
 $exit=$LASTEXITCODE
 if(!(Test-Path $receipt)){throw "NATIVE_PART_NO_RECEIPT=$id exit=$exit"}
 $proof=Get-Content $receipt -Raw | ConvertFrom-Json
 Write-Output "NATIVE_PART=$id STATUS=$($proof.status) BONES=$($proof.unreal_bones) MORPHS=$($proof.unreal_morphs)"
 if($proof.status -ne 'REAL_SOURCE_NATIVE_UNREAL_SKELETAL_PART_IMPORTED'){throw "Part failed: $id"}
 if($exit -ne 0){throw "Unreal commandlet error after part $id"}
}
Write-Output "ALL_REAL_BERSERKER_NATIVE_PARTS_IMPORTED=6"
