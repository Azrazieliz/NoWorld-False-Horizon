param([switch]$Install)
$ErrorActionPreference='Stop'
$r='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Saved\Gate12Koikatsu\LuminousV3'
$j=Join-Path $r 'luminous_signed_installable_apk_verification.json'
if(!(Test-Path $j)){throw 'Isolated Luminous APK has not passed signature/package verification'}
$x=Get-Content $j -Raw|ConvertFrom-Json
if($x.status -ne 'PASS_SIGNED_SEPARATE_LUMINOUS_EXPERIMENTAL_APK_READY_FOR_PHYSICAL_INSTALL'){throw 'Verified Luminous APK receipt is not PASS'}
$apk=$x.apk_path
if(!(Test-Path $apk) -or (Get-FileHash $apk -Algorithm SHA256).Hash.ToLowerInvariant() -ne $x.apk_sha256){throw 'Verified APK missing or modified'}
$adb='C:\Users\mimim\AppData\Local\Android\Sdk\platform-tools\adb.exe'
$aapt='C:\Users\mimim\AppData\Local\Android\Sdk\build-tools\36.1.0\aapt.exe'
$signer='C:\Users\mimim\AppData\Local\Android\Sdk\build-tools\36.1.0\apksigner.bat'
if(!(Test-Path $adb) -or !(Test-Path $aapt) -or !(Test-Path $signer)){throw 'Android SDK not available'}
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
if(!(Test-Path (Join-Path $env:JAVA_HOME 'bin\java.exe'))){throw 'Android signing Java unavailable'}
$badging=@(& $aapt dump badging $apk)
if($LASTEXITCODE -ne 0 -or !($badging|Where-Object{$_ -match "^package: name='com\.azrazieliz\.gate12luminousv3'"})){throw 'Wrong package; no installation'}
& $signer verify $apk
if($LASTEXITCODE -ne 0){throw 'APK signing verification failed'}
$serial='R3GYC0LSC7K'
$devices=@(& $adb devices -l)
if(!($devices|Where-Object{$_ -match ('^'+$serial+'\s+device\b')})){
 Write-Output "PHONE_NOT_CONNECTED: Unlock Samsung phone and authorize USB debugging; then rerun."
 exit 12
}
$model=(& $adb -s $serial shell getprop ro.product.model).Trim()
if($model -notmatch 'SM-S948|S26 Ultra'){throw "Unexpected phone $model; refusing install"}
$normalBefore=@(& $adb -s $serial shell pm path 'com.azrazieliz.OfflineGame')
Write-Output "S26_ULTRA_PRESENT=$model NORMAL_OFFLINEGAME_PRESENT=$(@($normalBefore).Count -gt 0)"
if(!$Install){Write-Output 'PREINSTALL_PASS: No app installed/changed. Rerun -Install only when ready.';exit 0}
$prior=@(& $adb -s $serial shell pm path 'com.azrazieliz.gate12luminousv3')
if(@($prior).Count -gt 0){
 & $adb -s $serial install -r -t $apk
}else{
 & $adb -s $serial install -t $apk
}
if($LASTEXITCODE -ne 0){throw 'Isolated Luminous install failed; no uninstall or clear data attempted'}
$new=@(& $adb -s $serial shell pm path 'com.azrazieliz.gate12luminousv3')
if(!($new|Where-Object{$_ -match '^package:'})){throw 'Luminous app not found after install'}
$normalAfter=@(& $adb -s $serial shell pm path 'com.azrazieliz.OfflineGame')
if(($normalBefore -join ' ') -ne ($normalAfter -join ' ')){throw 'Normal installed app changed unexpectedly'}
[ordered]@{status='PASS_SEPARATE_LUMINOUS_APK_PHYSICAL_ADB_INSTALL';phone=$model;serial=$serial;
 package='com.azrazieliz.gate12luminousv3';normal_app_unchanged=$true;
 apk_sha256=$x.apk_sha256;manual_phone_visual_and_touch_test='PENDING';
 time_utc=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json -Depth 4 |
 Set-Content (Join-Path $r 'luminous_physical_install_receipt.json') -Encoding UTF8
Write-Output 'PASS_S26_ULTRA_LUMINOUS_INSTALLED_NORMAL_GAME_PRESERVED'
