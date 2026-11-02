param([int]$MaxSeconds=840)
$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$f='D:\UnrealProjects\Offline-Game'
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Not isolated Gate12 branch'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Frozen Foundation changed'}
if(@(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue).Count -gt 0){Write-Output 'NOT_STARTED_EDITOR_IN_USE';exit 12}
$o=Get-CimInstance Win32_OperatingSystem
# 2026-10-18 actual 12-action ARM64 compile started with 3.3GB physical /
# 5.3GB commit available, then spent 106s and hit STOPPED_COMMIT_PRESSURE.
# Fail *before* full UBT invocation until the host can afford a native rebuild;
# never sacrifice protected user apps or a current editor to free memory.
if($o.FreePhysicalMemory/1024 -lt 3000 -or $o.FreeVirtualMemory/1024 -lt 6500){
  Write-Output ('NOT_STARTED_NATIVE_REBUILD_MEMORY_RAM_MB='+[math]::Round($o.FreePhysicalMemory/1024)+
    '_COMMIT_MB='+[math]::Round($o.FreeVirtualMemory/1024)+
    '_REQUIRES_RAM_3000_COMMIT_6500')
  exit 12
}
$sdk='C:\Users\mimim\AppData\Local\Android\Sdk'
$env:ANDROID_HOME=$sdk;$env:ANDROID_SDK_ROOT=$sdk
$env:NDKROOT=Join-Path $sdk 'ndk\27.2.12479018'
$env:NDK_ROOT=$env:NDKROOT
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$dotnet='D:\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
$ubt='D:\Epic Games\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
$r=Join-Path $w 'Saved\Gate12Koikatsu\NativeAndroidARM64'
New-Item -ItemType Directory -Force -Path $r|Out-Null
$bin=Join-Path $w 'Binaries\Android\OfflineGame-arm64.so'
if(Test-Path $bin){
 $oldhash=(Get-FileHash $bin -Algorithm SHA256).Hash.ToLowerInvariant()
 $bak=Join-Path $r ('before_new_native_'+$oldhash.Substring(0,16)+'_OfflineGame-arm64.so')
 if(!(Test-Path $bak)){Copy-Item $bin $bak -ErrorAction Stop}
 if((Get-FileHash $bak -Algorithm SHA256).Hash.ToLowerInvariant() -ne $oldhash){throw 'ARM64 binary backup mismatch'}
}
$args=@(('"' + $ubt + '"'),'OfflineGame','Android','Development',('-Project="' + (Join-Path $w 'OfflineGame.uproject') + '"'),
 # Restore UE shared precompiled headers: NoPCH exhausted 8GB physical RAM on this host.
 '-MaxParallelActions=1','-NoUBA','-NoXGE','-NoHotReload','-WaitMutex')
$t=[DateTime]::UtcNow
$p=Start-Process -FilePath $dotnet -ArgumentList $args -PassThru -WindowStyle Hidden -RedirectStandardOutput (Join-Path $r 'ubt_stdout.txt') -RedirectStandardError (Join-Path $r 'ubt_stderr.txt')
Write-Output "ANDROID_ARM64_UBT_START_PID=$($p.Id)"
$samples=@();$status='RUNNING'
try{
 while(-not $p.HasExited){
  Start-Sleep -Seconds 2;$p.Refresh();$o=Get-CimInstance Win32_OperatingSystem
  $ram=[math]::Round($o.FreePhysicalMemory/1024)
  $commit=[math]::Round($o.FreeVirtualMemory/1024)
  $sec=[math]::Round(([DateTime]::UtcNow-$t).TotalSeconds)
  $samples+=@([pscustomobject]@{sec=$sec;freePhysicalMB=$ram;freeCommitMB=$commit})
  if($sec%30 -lt 2){Write-Output "ANDROID_NATIVE_TIME=$sec RAM=$ram COMMIT=$commit"}
  if($ram -lt 450){$status='STOPPED_RAM_PRESSURE';break}
  if($commit -lt 2400){$status='STOPPED_COMMIT_PRESSURE';break}
  if($sec -gt $MaxSeconds){$status='STOPPED_TIME_LIMIT';break}
 }
 if($status -eq 'RUNNING'){$status='PROCESS_EXITED'}
}finally{
 if(-not $p.HasExited){& taskkill.exe /T /F /PID $p.Id|Out-Null}
 $p.Refresh()
 $output=Join-Path $r 'ubt_stdout.txt'
 $content=if(Test-Path $output){Get-Content $output -Raw}else{''}
 $pass=($status -eq 'PROCESS_EXITED' -and $content -match 'Result: Succeeded' -and (Test-Path $bin))
 $receipt=[ordered]@{status=if($pass){'PASS_ANDROID_ARM64_NATIVE_LINK'}else{'NOT_PASS_ANDROID_ARM64_NATIVE_LINK'};
  processStatus=$status;startedUTC=$t.ToString('o');endedUTC=[DateTime]::UtcNow.ToString('o');
  sourceWorktree=$w;target='OfflineGame Android Development';physicalPhone='NOT_TESTED';
  rebuilt_binary_sha256=if($pass){(Get-FileHash $bin -Algorithm SHA256).Hash.ToLowerInvariant()}else{$null};
  samples=$samples}
 $uniquereceipt=Join-Path $r ('arm64_native_build_receipt_'+$t.ToString('yyyyMMdd_HHmmss')+'.json')
 $receipt|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $uniquereceipt -Encoding UTF8
 try {
  Copy-Item -LiteralPath $uniquereceipt -Destination (Join-Path $r 'arm64_native_build_receipt.json') -Force -ErrorAction Stop
 } catch {
  Write-Output ('CANONICAL_RECEIPT_LOCKED_BACKUP_VALID='+$uniquereceipt)
 }
 Write-Output "ARM64_NATIVE_RESULT=$($receipt.status) UNIQUE_RECEIPT=$uniquereceipt"
 if(Test-Path $output){Get-Content $output -Tail 12}
 if(-not $pass){exit 28}
}
