param(
 [ValidateSet('CLOTHED','UNDERWEAR','NUDE')][string]$State='UNDERWEAR',
 [int]$MaxSeconds=690
)
$ErrorActionPreference='Stop'
$root='D:\Tools\GameAssetPipeline\KoikatsuParty\characters\LUMINOUS_VALENTINE_V3\gate12_fresh_export_20261011'
$script='D:\Tools\GameAssetPipeline\KoikatsuParty\pipeline\gate12_luminous_fresh_state_blender_roundtrip.py'
$dir=Join-Path $root ("BLENDER_ROUNDTRIPS\"+$State)
$source=Get-Content (Join-Path $root ("FRESH_EXPORT_"+$State+"_RECEIPT.json")) -Raw |ConvertFrom-Json
if($source.status -ne 'PASS_REAL_PARTY_KKBP_PMX_TEXTURE_METADATA_EXPORT_COPIED_AND_HASHED'){throw 'Unapproved source state'}
if((Get-Process -Name 'Koikatsu Party' -ErrorAction SilentlyContinue)){throw 'Do not run Blender when Party retains source game RAM'}
if((Get-Process -Name 'blender' -ErrorAction SilentlyContinue)){throw 'Another Blender session is already running'}
if(Test-Path (Join-Path $dir ('Luminous_V3_'+$State+'_source.blend'))){throw 'Existing source Blender scene must not be overwritten'}
New-Item -ItemType Directory -Path $dir -Force|Out-Null
$os=Get-CimInstance Win32_OperatingSystem
$ram=[math]::Round($os.FreePhysicalMemory/1024)
$commit=[math]::Round($os.FreeVirtualMemory/1024)
if($ram -lt 2350 -or $commit -lt 5100){throw "Insufficient memory to preserve high-res source state RAM=$ram COMMIT=$commit"}
$args=@('--background','--factory-startup','--python',('"'+$script+'"'),'--',$State)
$p=Start-Process -FilePath 'D:\Tools\Blender\blender.exe' -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'blender_stdout.log') -RedirectStandardError (Join-Path $dir 'blender_stderr.log')
Write-Output "LUMINOUS_$($State)_BLENDER_PID=$($p.Id)"
$start=[DateTime]::UtcNow
$status='RUNNING'
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 5
  $p.Refresh()
  $o=Get-CimInstance Win32_OperatingSystem
  $ram=[math]::Round($o.FreePhysicalMemory/1024)
  $commit=[math]::Round($o.FreeVirtualMemory/1024)
  $sec=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
  if(($sec % 25) -lt 5){Write-Output "STATE=$State T=$sec FREE_RAM_MB=$ram COMMIT_MB=$commit"}
  if($ram -lt 375 -or $commit -lt 1400){$status='STOPPED_RESOURCE_GUARD';break}
  if($sec -gt $MaxSeconds){$status='STOPPED_TIME_GUARD';break}
 }
 if($status -eq 'RUNNING'){$status='EXITED'}
}finally{
 if(-not $p.HasExited){Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue}
}
Write-Output "LUMINOUS_$($State)_BLENDER_PROCESS=$status"
$report=Join-Path $dir 'FRESH_KKBP_BLENDER_FBX_GLB_REPORT.json'
if(!(Test-Path $report)){Write-Output 'NO_BLENDER_RECEIPT';exit 21}
$j=Get-Content $report -Raw|ConvertFrom-Json
Write-Output "BLENDER_STATE=$State STATUS=$($j.status) STEP_COUNT=$(@($j.steps).Count)"
if($j.error){Write-Output "BLENDER_ERROR=$($j.error)"}
if($status -ne 'EXITED' -or $j.status -ne 'PASS_FRESH_KOIKATSU_TO_BLENDER_FBX_GLB_PRE_UNREAL'){exit 22}
Write-Output "BLENDER_FRESH_SOURCE_$($State)_PASS"
