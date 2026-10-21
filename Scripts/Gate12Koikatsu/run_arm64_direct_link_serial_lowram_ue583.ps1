param(
  [int]$MaxWaitSeconds=480,
  [int]$MaxLinkSeconds=720
)
$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$f='D:\UnrealProjects\Offline-Game'
$r=Join-Path $w 'Saved\Gate12Koikatsu\NativeAndroidARM64'
$cmd=Join-Path $r 'link_arm64_singlethread_candidate.cmd'
$target=Join-Path $r 'direct_link_candidate\libUnreal_gate12_candidate.so'
$old=Join-Path $w 'Binaries\Android\OfflineGame-arm64.so'
$sourceRsp=Join-Path $w 'Intermediate\Build\Android\a\OfflineGame\Development\OfflineGame-arm64.so.rsp'
$receipt=Join-Path $r ('direct_arm64_link_'+[DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss')+'.json')
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Wrong isolated branch'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Frozen Foundation modified'}
if(!(Test-Path $cmd) -or !(Test-Path $sourceRsp)){throw 'Missing validated linker command / native link response'}
if(Test-Path $target){throw 'Do not replace prior candidate without review'}
$parts=@(Get-Content $sourceRsp|Where-Object{$_ -match '^"[^"]+\.o"$'})
if($parts.Count -lt 800){throw 'Missing actual Unreal monolithic ELF input list'}
foreach($line in $parts){$obj=$line.Trim('"').Replace('/','\');if(!(Test-Path -LiteralPath $obj)){throw ('Missing link input '+$obj)}}
$origHash=(Get-FileHash $old -Algorithm SHA256).Hash
New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force|Out-Null
$waitStart=[DateTime]::UtcNow
$samples=@()
$observedConsecutive=0
Write-Output "ARM64_LINK_WAIT_FOR_MEASURED_RESOURCES input_obj_count=$($parts.Count)"
while(([DateTime]::UtcNow-$waitStart).TotalSeconds -lt $MaxWaitSeconds){
  $o=Get-CimInstance Win32_OperatingSystem
  $ram=[math]::Round($o.FreePhysicalMemory/1024)
  $commit=[math]::Round($o.FreeVirtualMemory/1024)
  $sec=[math]::Round(([DateTime]::UtcNow-$waitStart).TotalSeconds)
  $samples+=@([pscustomobject]@{phase='WAIT';second=$sec;physicalMB=$ram;commitMB=$commit})
  if($ram -ge 2200 -and $commit -ge 4900){$observedConsecutive++}else{$observedConsecutive=0}
  if($observedConsecutive -ge 2){break}
  if($sec%30 -lt 12){Write-Output "WAIT_T=$sec FREE_PHYS_MB=$ram FREE_COMMIT_MB=$commit"}
  Start-Sleep -Seconds 12
}
if($observedConsecutive -lt 2){
 $obj=[ordered]@{status='NOT_STARTED_RESOURCE_WINDOW_UNAVAILABLE';wait_sec=[math]::Round(([DateTime]::UtcNow-$waitStart).TotalSeconds);input_count=$parts.Count;original_so_sha256=$origHash;user_java_processes_touched=$false;samples=$samples}
 $obj|ConvertTo-Json -Depth 5|Set-Content $receipt -Encoding UTF8
 Write-Output "ARM64_LINK_NOT_STARTED_RESOURCE_WINDOW_RECEIPT=$receipt"
 exit 12
}
if(@(Get-Process -Name 'UnrealEditor*','clang++','dotnet' -ErrorAction SilentlyContinue).Count -gt 0){
 Write-Output 'LINK_NOT_STARTED_CONCURRENT_COMPILER';exit 14
}
$stdout=Join-Path $r 'direct_link_stdout.log'
$stderr=Join-Path $r 'direct_link_stderr.log'
$t=[DateTime]::UtcNow
$p=Start-Process 'cmd.exe' -ArgumentList @('/c',('"'+$cmd+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
Write-Output "ARM64_DIRECT_SINGLE_THREAD_LINK_STARTED_PID=$($p.Id)"
$state='RUNNING'
$lowCount=0
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 3
  $p.Refresh()
  $o=Get-CimInstance Win32_OperatingSystem
  $ram=[math]::Round($o.FreePhysicalMemory/1024)
  $commit=[math]::Round($o.FreeVirtualMemory/1024)
  $sec=[math]::Round(([DateTime]::UtcNow-$t).TotalSeconds)
  $samples+=@([pscustomobject]@{phase='LINK';second=$sec;physicalMB=$ram;commitMB=$commit})
  if($sec%15 -lt 3){Write-Output "LINK_T=$sec FREE_PHYS_MB=$ram COMMIT_FREE_MB=$commit"}
  if($ram -lt 250){$lowCount++}else{$lowCount=0}
  if($lowCount -ge 3){$state='STOPPED_SUSTAINED_PHYSICAL_PRESSURE';break}
  if($commit -lt 1800){$state='STOPPED_COMMIT_PRESSURE';break}
  if($sec -gt $MaxLinkSeconds){$state='STOPPED_TIME_LIMIT';break}
 }
 if($state -eq 'RUNNING'){$state='PROCESS_EXITED'}
}finally{
 if(-not $p.HasExited){& taskkill.exe /T /F /PID $p.Id|Out-Null}
 $p.Refresh()
 $exit=if($p.HasExited){$p.ExitCode}else{$null}
 $elf=$false
 if(Test-Path $target){
  $fs=[IO.File]::OpenRead($target)
  try{
   $bytes=New-Object byte[] 20
   $count=$fs.Read($bytes,0,20)
   $elf=($count -eq 20 -and $bytes[0] -eq 127 -and $bytes[1] -eq 69 -and $bytes[2] -eq 76 -and $bytes[3] -eq 70 -and $bytes[4] -eq 2 -and $bytes[18] -eq 183)
  }finally{$fs.Dispose()}
 }
 $origUnchanged=((Get-FileHash $old -Algorithm SHA256).Hash -eq $origHash)
 $pass=($state -eq 'PROCESS_EXITED' -and $exit -eq 0 -and $elf -and $origUnchanged -and (Get-Item $target -ErrorAction SilentlyContinue).Length -gt 100MB)
 $obj=[ordered]@{
   status=if($pass){'PASS_DIRECT_LLD_SINGLE_THREAD_ARM64_ELF_CANDIDATE'}else{'NOT_PASS_DIRECT_ARM64_LINK'}
   process_state=$state;exit_code=$exit;elf_arm64_header_verified=$elf
   original_so_untouched=$origUnchanged;input_objects=$parts.Count
   source_branch='production/gate12-koikatsu-ue583-fixture-20261010'
   started_utc=$t.ToString('o');finished_utc=[DateTime]::UtcNow.ToString('o')
   candidate=$target;candidate_sha256=if($pass){(Get-FileHash $target -Algorithm SHA256).Hash.ToLowerInvariant()}else{$null}
   output_stdout=$stdout;output_stderr=$stderr;user_java_processes_touched=$false
   samples=$samples
 }
 $obj|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $receipt -Encoding UTF8
 Write-Output "ARM64_DIRECT_LINK_RESULT=$($obj.status) CODE=$exit RECEIPT=$receipt"
 if(Test-Path $stderr){Get-Content $stderr -Tail 12}
 if(!$pass){exit 28}
}
