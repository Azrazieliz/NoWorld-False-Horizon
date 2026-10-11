param([int]$MaxSeconds=1500)
$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$f='D:\UnrealProjects\Offline-Game'
$r=Join-Path $w 'Saved\Gate12Koikatsu\LuminousV3'
$configEngine=Join-Path $w 'Config\DefaultEngine.ini'
$configGame=Join-Path $w 'Config\DefaultGame.ini'
$archive=Join-Path $r 'AndroidLuminousIsolatedASTC_v2'
$transaction=Join-Path $r 'LuminousAndroidConfigTransaction_v2'
$receipt=Join-Path $r 'luminous_isolated_astc_apk_build_receipt_v2.json'
$package='com.azrazieliz.gate12luminousv3'
$map='/Game/Experimental/LuminousValentineV3/AndroidReady_20261011/Maps/L_LuminousV3_IsolatedAndroid'
$runUat='D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
if((git -C $w branch --show-current).Trim() -ne 'production/gate12-koikatsu-ue583-fixture-20261010'){throw 'Wrong Gate12 worktree'}
if((git -C $f rev-parse HEAD).Trim() -ne '3f54f07cacbf193e160d0be15a7be5a7bc730c3b' -or (git -C $f status --porcelain)){throw 'Frozen Foundation dirty'}
if($package -eq 'com.azrazieliz.OfflineGame' -or $package -match 'fixture'){throw 'App identity not isolated Luminous'}
if(!(Test-Path $runUat)){throw 'Unreal 5.8.3 RunUAT absent'}
if((Get-Process -Name 'UnrealEditor*','dotnet','java','clang++' -ErrorAction SilentlyContinue|Measure-Object).Count -gt 0){throw 'Compile/editor/process active, refusing concurrent package'}
$entry=Join-Path $r 'luminous_isolated_android_entry_author_receipt_ue583.json'
$motion=Join-Path $r 'luminous_native_three_wardrobe_motion_v3_bound_ue583.json'
if(!(Test-Path $entry) -or !(Test-Path $motion)){throw 'Android-specific entry or native Motion V3 receipt missing'}
$j=Get-Content $entry -Raw|ConvertFrom-Json
if($j.status -ne 'PASS_ISOLATED_ANDROID_LUMINOUS_NATIVE_ENTRY_AUTHORED_PRE_COLD_TEST' -or
   $j.android_application_id -ne $package -or
   $j.native_mocap_clips -ne 9 -or
   $j.native_assets_clothed -ne 45 -or $j.native_assets_underwear -ne 39 -or $j.native_assets_nude -ne 37){throw 'Incomplete Luminous-only entry assets'}
$ubt=Get-Content (Join-Path $w 'Saved\Gate12Koikatsu\NativeAndroidARM64\arm64_native_build_receipt.json') -Raw|ConvertFrom-Json
if($ubt.status -ne 'PASS_ANDROID_ARM64_NATIVE_LINK'){throw 'Native UE Android build not PASS'}
$bin=Join-Path $w 'Binaries\Android\OfflineGame-arm64.so'
if(!(Test-Path $bin) -or (Get-FileHash $bin -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ubt.rebuilt_binary_sha256){throw 'Android .so does not match native build receipt'}
$sourceFiles=@(Get-ChildItem (Join-Path $w 'Source\OfflineGame\Private\Koikatsu') -File -Filter 'OGLuminousV3*.cpp')+
             @(Get-ChildItem (Join-Path $w 'Source\OfflineGame\Public\Koikatsu') -File -Filter 'OGLuminousV3*.h')
$sourceMostRecent=($sourceFiles|Sort-Object LastWriteTimeUtc -Descending|Select-Object -First 1).LastWriteTimeUtc
$binaryTimestamp=(Get-Item $bin).LastWriteTimeUtc
if($binaryTimestamp -lt $sourceMostRecent){throw 'Android native link is older than the new Luminous UI/gameplay code'}
if(Test-Path $transaction){throw 'Previous config transaction exists; restore/inspect before packaging'}
if(Test-Path $receipt){throw 'Prior Luminous Android build receipt already exists'}
New-Item -ItemType Directory -Path $transaction -Force|Out-Null
New-Item -ItemType Directory -Path $archive -Force|Out-Null
$engineBackup=Join-Path $transaction 'DefaultEngine.ini.UNTOUCHED'
$gameBackup=Join-Path $transaction 'DefaultGame.ini.UNTOUCHED'
[IO.File]::WriteAllBytes($engineBackup,[IO.File]::ReadAllBytes($configEngine))
[IO.File]::WriteAllBytes($gameBackup,[IO.File]::ReadAllBytes($configGame))
$originalEngine=(Get-FileHash $configEngine -Algorithm SHA256).Hash.ToLowerInvariant()
$originalGame=(Get-FileHash $configGame -Algorithm SHA256).Hash.ToLowerInvariant()
$start=[DateTime]::UtcNow
$status='BLOCKED_BEFORE_LAUNCH'
$uatPid=$null
$samples=@()
$stdout=Join-Path $archive 'uat_stdout.txt'
$stderr=Join-Path $archive 'uat_stderr.txt'
Write-Output "GATE12_LUMINOUS_APK_BEGIN PACKAGE=$package MAP=$map"
try {
  $config= [IO.File]::ReadAllText($configEngine)
  if(!$config.Contains('PackageName=com.azrazieliz.gate12fixture') -or
     !$config.Contains('GameDefaultMap=/Game/Experimental/Gate12Koikatsu/PlayableQA/Maps/L_KoikatsuPlayable_UE583')){throw 'Unexpected default map/package source; refuse unsafe patch'}
  $config=$config.Replace('PackageName=com.azrazieliz.gate12fixture',"PackageName=$package")
  $config=$config.Replace('GameDefaultMap=/Game/Experimental/Gate12Koikatsu/PlayableQA/Maps/L_KoikatsuPlayable_UE583',"GameDefaultMap=$map")
  $config=$config.Replace('EditorStartupMap=/Game/Experimental/Gate12Koikatsu/FullFidelityQA/Maps/L_KK_NativeRestSafeSway_UE583',"EditorStartupMap=$map")
  [IO.File]::WriteAllText($configEngine,$config,[Text.UTF8Encoding]::new($false))
  $game=[IO.File]::ReadAllText($configGame)
  # This is a Luminous-only experimental APK; do not cook unrelated legacy
  # Koikatsu fixture directories. Original bytes are restored in finally.
  $game=[regex]::Replace($game,'(?m)^\+DirectoriesToAlwaysCook=\(Path="/Game/Experimental/Gate12Koikatsu/[^\"]+"\)\r?\n?','')
  $lines=@(
  '+DirectoriesToAlwaysCook=(Path="/Game/Experimental/LuminousValentineV3/CLOTHED/Parts")',
  '+DirectoriesToAlwaysCook=(Path="/Game/Experimental/LuminousValentineV3/UNDERWEAR/Parts")',
  '+DirectoriesToAlwaysCook=(Path="/Game/Experimental/LuminousValentineV3/NUDE/Parts")',
  '+DirectoriesToAlwaysCook=(Path="/Game/Experimental/LuminousValentineV3/NativeMocapV3")',
  '+DirectoriesToAlwaysCook=(Path="/Game/Experimental/LuminousValentineV3/AndroidReady_20261011")'
  )
  foreach($line in $lines){if(!$game.Contains($line)){$game+=[Environment]::NewLine+$line}}
  [IO.File]::WriteAllText($configGame,$game,[Text.UTF8Encoding]::new($false))
  if(!((Get-Content $configEngine -Raw).Contains("PackageName=$package"))){throw 'App package override not applied'}
  $sdk='C:\Users\mimim\AppData\Local\Android\Sdk'
  $env:ANDROID_HOME=$sdk;$env:ANDROID_SDK_ROOT=$sdk
  $env:NDKROOT=Join-Path $sdk 'ndk\27.2.12479018';$env:NDK_ROOT=$env:NDKROOT
  $env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
  $env:UE_SDKS_ROOT=$null
  $o=Get-CimInstance Win32_OperatingSystem
  if($o.FreePhysicalMemory/1024 -lt 3300 -or $o.FreeVirtualMemory/1024 -lt 5800){throw 'RAM/commit threshold insufficient for Luminous Android cook'}
  # Existing compiled arm64 native link, cook only our exact Luminous map,
  # stage, package/sign with Android Development debug signer and archive.
  $uatWrapper=Join-Path $w 'Scripts\Gate12Koikatsu\invoke_luminous_v3_android_astc_uat_ue583.ps1'
  if(!(Test-Path $uatWrapper)){throw 'Required safe RunUAT PowerShell wrapper not found'}
  $arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$uatWrapper+'"'))
  $p=Start-Process -FilePath 'powershell.exe' -ArgumentList $arguments -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
  $uatPid=$p.Id
  $status='RUNNING'
  Write-Output "GATE12_LUMINOUS_ASTC_UAT_PID=$uatPid"
  while(-not $p.HasExited){
   Start-Sleep -Seconds 5;$p.Refresh()
   $o=Get-CimInstance Win32_OperatingSystem
   $sec=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
   $ram=[math]::Round($o.FreePhysicalMemory/1024)
   $commit=[math]::Round($o.FreeVirtualMemory/1024)
   $samples+=@([ordered]@{elapsed_seconds=$sec;ram_free_mib=$ram;commit_free_mib=$commit})
   if($sec%35 -lt 5){Write-Output "GATE12_LUMINOUS_ASTC_TIME=$sec RAM_FREE_MIB=$ram COMMIT_FREE_MIB=$commit"}
   if($ram -lt 420 -or $commit -lt 1800){$status='BLOCKED_HOST_RAM_GUARD';break}
   if($sec -gt $MaxSeconds){$status='BLOCKED_COOK_TIME_GUARD';break}
  }
  if($status -eq 'RUNNING'){$status='PROCESS_EXITED'}
}finally{
 if($null -ne $uatPid -and $status -ne 'PROCESS_EXITED'){
   & taskkill.exe /T /F /PID $uatPid 2>&1|Out-Null
 }
 # The config restore happens even after cook failure; originals are preserved
 # separately in transaction evidence to recover any forced termination.
 [IO.File]::WriteAllBytes($configEngine,[IO.File]::ReadAllBytes($engineBackup))
 [IO.File]::WriteAllBytes($configGame,[IO.File]::ReadAllBytes($gameBackup))
 $restoreOK=((Get-FileHash $configEngine -Algorithm SHA256).Hash.ToLowerInvariant() -eq $originalEngine -and
             (Get-FileHash $configGame -Algorithm SHA256).Hash.ToLowerInvariant() -eq $originalGame)
 if(!$restoreOK){throw 'CRITICAL: config restore failed, backups preserved in transaction; stop'}
 $raw=if(Test-Path $stdout){Get-Content $stdout -Raw}else{''}
 $uatOK=$status -eq 'PROCESS_EXITED' -and ($raw -match 'BUILD SUCCESSFUL|AutomationTool exiting with ExitCode=0')
 $found=@(Get-ChildItem $archive -Recurse -File -Filter '*.apk' -ErrorAction SilentlyContinue)
 $distinct=@($found |Where-Object{$_.Name -match 'arm64|OfflineGame'})
 $apk=if($distinct.Count -eq 1){$distinct[0]}else{$null}
 $result=[ordered]@{status=if($uatOK -and $apk -and $restoreOK){'PARTIAL_APK_REQUIRES_SIGNATURE_AND_PACKAGE_ID_VERIFICATION'}else{'NOT_PASS_ANDROID_LUMINOUS_PACKAGE'};
   process_status=$status;uat_success=$uatOK;apk_count=$found.Count;target_package=$package;target_map=$map;
   apk_path=if($apk){$apk.FullName}else{$null};
   apk_sha256=if($apk){(Get-FileHash $apk.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}else{$null};
   configs_byte_restored=$restoreOK;win_native_source_sha_validated_at_start=$true;
   full_native_state='PARTIAL_45_SOURCE_MORPHS_MISSING';
   local_full_runtime_session='PASS_HEADLESS_NATIVE_3_WARDROBE_AND_3_MOTION';
   physical_samsung_install='NOT_TESTED';normal_game_app='UNTOUCHED';
   utc=[DateTime]::UtcNow.ToString('o');samples=$samples}
 $result|ConvertTo-Json -Depth 8|Set-Content -Path $receipt -Encoding UTF8
 Write-Output "GATE12_LUMINOUS_UAT_RESULT=$($result.status) UAT_EXIT=$status APK_COUNT=$($found.Count) CONFIG_RESTORED=$restoreOK"
 if(Test-Path $stdout){Get-Content $stdout -Tail 30}
}
