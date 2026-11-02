# Resource-bound Win64 first-World package from the EXISTING project.
# Never cooks disposable QA maps or substitutes placeholder assets.
$ErrorActionPreference='Stop'
$r='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$uat='D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
$archive="$r\Builds\Gate12_FirstWorld_Production_Win64_20261021"
$log="$r\Saved\Gate12Readiness\Gate12_FirstWorld_Win64_ResourceBoundCook_20261021.log"
$receipt="$r\Saved\Gate12Readiness\Gate12_FirstWorld_Win64_ResourceBoundResult_20261021.json"
$ddc="$r\Saved\Gate12NativeDDC"
$tmp="$r\Saved\Gate12NativeBuildTemp"
if(!(Test-Path $uat)){throw "UE 5.8 UAT not installed"}
if(Test-Path $archive){throw "Existing archive is protected; no destructive overwrite: $archive"}
New-Item -ItemType Directory -Force $ddc,$tmp,(Split-Path $archive) | Out-Null
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value $ddc
$env:TEMP=$tmp
$env:TMP=$tmp
$params=@('BuildCookRun',
  "-project=$r\OfflineGame.uproject",
  '-noP4','-unattended','-utf8output',
  '-platform=Win64','-clientconfig=Development',
  '-build','-cook','-stage','-pak','-package','-archive',
  "-archivedirectory=$archive",'-nocompileeditor',
  '-numcookerstospawn=1','-MaxParallelActions=1',
  "-AdditionalCookerOptions=-DDC=(Local) -LocalDataCachePath=$ddc -ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=8")
$status=[ordered]@{status='STARTED'; archive=$archive;first_world='/Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_StreamingRuntime';method='one cooker and one build action, D-drive DDC/TEMP, existing native assets';exit_code=$null;exe_paths=@();pak_count=0;errors=@()}
try {
  & $uat @params *> $log
  $status.exit_code=$LASTEXITCODE
  if($LASTEXITCODE -eq 0){
    $exe=@(Get-ChildItem $archive -Filter 'OfflineGame.exe' -File -Recurse -ErrorAction SilentlyContinue)
    $pak=@(Get-ChildItem $archive -Filter '*.pak' -File -Recurse -ErrorAction SilentlyContinue)
    $status.exe_paths=@($exe | ForEach-Object {$_.FullName})
    $status.pak_count=$pak.Count
    $status.status=if($exe.Count -gt 0 -and $pak.Count -gt 0){'WIN64_GAME_PACKAGE_CREATED_UNQUALIFIED_GAMEPLAY'}else{'UAT_SUCCEEDED_BUT_PACKAGE_ASSETS_MISSING'}
  }else{
    $status.status='WIN64_RESOURCE_BOUND_PACKAGE_FAILED'
    $status.errors+=@(Get-Content $log -Tail 25)
  }
}catch {
  $status.status='WIN64_PACKAGING_EXCEPTION'
  $status.errors+=@($_.Exception.Message)
}finally {
  $status | ConvertTo-Json -Depth 5 | Out-File -FilePath $receipt -Encoding UTF8
  Write-Output ("WIN64_PACKAGE_STATUS="+$status.status)
  Write-Output ("RECEIPT="+$receipt)
  Write-Output ("ARCHIVE="+$archive)
  if(Test-Path $log){Get-Content $log -Tail 20}
}
