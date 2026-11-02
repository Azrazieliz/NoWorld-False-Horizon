$ErrorActionPreference='Stop'
$base='D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011'
$py='D:\Tools\Python311\python.exe'
$blender='D:\Tools\Blender\blender.exe'
$src='D:\Tools\GameAssetPipeline\KoikatsuParty\pipeline\gate12_fresh_luminous_blender_conversion.py'
$audit='D:\Tools\GameAssetPipeline\KoikatsuParty\pipeline\gate12_verify_luminous_converted_glb.py'
$states=@('UNDERWEAR','NUDE')
foreach($state in $states){
 $dir="$base\BLENDER_FRESH\$state"
 New-Item -ItemType Directory -Force $dir |Out-Null
 $resultFile="$dir\GATE12_FRESH_BLENDER_CONVERSION_RECEIPT.json"
 $out="$dir\Blender_console_stdout.txt"
 $err="$dir\Blender_console_stderr.txt"
 if(Test-Path $resultFile){
  $prior=Get-Content $resultFile -Raw|ConvertFrom-Json
  if($prior.status -eq 'PASS_REAL_PRIMARY_PARTY_SOURCE_BLENDER_FBX_GLB_EXPORTED_PENDING_INDEPENDENT_VALIDATION'){
    & $py $audit $state
    if($LASTEXITCODE){throw "$state old receipt cannot be independently validated"}
    Write-Output "V3_$($state)_ALREADY_PRESENT_VERIFIED"
    continue
  }
  throw "Previous failed or incomplete attempt exists for $state; manual inspection required"
 }
 $os=Get-CimInstance Win32_OperatingSystem
 if(($os.FreePhysicalMemory/1024) -lt 2100 -or ($os.FreeVirtualMemory/1024) -lt 3500){throw "$state refused due low starting memory"}
 $args=@('-b','--factory-startup','-t','2','--python',('"' + $src + '"'),'--',$state)
 $p=Start-Process -FilePath $blender -ArgumentList $args -PassThru -WindowStyle Hidden -RedirectStandardOutput $out -RedirectStandardError $err
 Write-Output "V3_$($state)_BLENDER_PID=$($p.Id)"
 $start=[DateTime]::UtcNow
 $status='RUNNING'
 try{
  while(!$p.HasExited){
   Start-Sleep -Seconds 3;$p.Refresh()
   $os=Get-CimInstance Win32_OperatingSystem
   $ram=[math]::Round($os.FreePhysicalMemory/1024)
   $commit=[math]::Round($os.FreeVirtualMemory/1024)
   $sec=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
   if(($sec % 24) -lt 3){
    Write-Output "V3_$($state)_IMPORT_PROGRESS_SECONDS=$sec FREE_RAM=$ram COMMIT=$commit"
    Get-Content $out -Tail 1 -ErrorAction SilentlyContinue|ForEach-Object{Write-Output $_}
   }
   if($ram -lt 300 -or $commit -lt 1100){$status='STOPPED_RESOURCE_GUARD';break}
   if($sec -gt 485){$status='STOPPED_TIME_GUARD';break}
  }
 }finally{
  if(-not $p.HasExited){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
 }
 $p.Refresh()
 if($status -ne 'RUNNING'){throw "V3_$state Blender $status"}
 if(!(Test-Path $resultFile)){throw "V3_$state Blender missing receipt"}
 $actual=Get-Content $resultFile -Raw|ConvertFrom-Json
 if($actual.status -ne 'PASS_REAL_PRIMARY_PARTY_SOURCE_BLENDER_FBX_GLB_EXPORTED_PENDING_INDEPENDENT_VALIDATION'){
  throw "V3_$state Blender finished with bad receipt $($actual.status)"
 }
 & $py $audit $state
 if($LASTEXITCODE){throw "V3_$state GLB independent morphology audit failed"}
 Write-Output "V3_$($state)_BLENDER_AND_GLB_INDEPENDENT_VALIDATION=PASS"
}
Write-Output 'V3_ALL_THREE_REAL_PARTY_BLENDER_EXPORTS_INDEPENDENTLY_VALIDATED'
