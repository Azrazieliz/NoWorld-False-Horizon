$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$r=Join-Path $w 'Saved\Gate12Koikatsu\LuminousV3'
$sourceReceipt=Join-Path $r 'luminous_isolated_astc_apk_build_receipt_v2.json'
$verifyReceipt=Join-Path $r 'luminous_signed_installable_apk_verification.json'
$appId='com.azrazieliz.gate12luminousv3'
$targetDir=Join-Path $r 'PhoneReady'
$aapt='C:\Users\mimim\AppData\Local\Android\Sdk\build-tools\36.1.0\aapt.exe'
$signer='C:\Users\mimim\AppData\Local\Android\Sdk\build-tools\36.1.0\apksigner.bat'
if(Test-Path $verifyReceipt){throw 'Previous verification exists and cannot be replaced'}
if(!(Test-Path $sourceReceipt)){throw 'No validated Android UAT receipt'}
$j=Get-Content $sourceReceipt -Raw|ConvertFrom-Json
if($j.status -ne 'PARTIAL_APK_REQUIRES_SIGNATURE_AND_PACKAGE_ID_VERIFICATION' -or !$j.configs_byte_restored){
 throw "APK packaging not successful/isolated or config transaction not restored: $($j.status)"
}
if(!(Test-Path $aapt) -or !(Test-Path $signer)){throw 'Android build tools unavailable'}
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
if(!(Test-Path (Join-Path $env:JAVA_HOME 'bin\java.exe'))){throw 'Android signing Java runtime missing'}
$apk=$j.apk_path
if(!(Test-Path $apk)){throw 'UAT-generated APK absent'}
$initial=(Get-FileHash $apk -Algorithm SHA256).Hash.ToLowerInvariant()
if($initial -ne $j.apk_sha256){throw 'APK changed since original UAT receipt'}
$badging=@(& $aapt dump badging $apk)
if($LASTEXITCODE -ne 0){throw 'aapt cannot parse APK'}
$packageLine=$badging|Where-Object{$_ -match "^package:"}|Select-Object -First 1
if(!$packageLine -or $packageLine -notmatch ("name='"+[regex]::Escape($appId)+"'")){
 throw "Wrong application ID: $packageLine"
}
if($packageLine -match 'com.azrazieliz.OfflineGame|com.azrazieliz.gate12fixture'){throw 'NOT an isolated Luminous application'}
$signerResult=@(& $signer verify --verbose --print-certs $apk)
if($LASTEXITCODE -ne 0){throw 'APK signature failed verification'}
$certificateLine=$signerResult|Where-Object{$_ -match 'Signer #1 certificate SHA-256 digest:'}|Select-Object -First 1
if(!$certificateLine){throw 'Signer certificate digest missing'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead($apk)
try{
 $names=@($zip.Entries |ForEach-Object{$_.FullName})
 if(!($names|Where-Object{$_ -match '^lib/arm64-v8a/libUnreal\.so$'})){
  throw 'APK does not embed Android ARM64 Unreal native library'
 }
 $packedData=$names|Where-Object{$_ -match '\.(pak|utoc|ucas|obb|obb\.png)$'}
 if(!$packedData){throw 'No cooked game data found inside expected all-in-APK experiment'}
 $entryCount=$names.Count
}finally{$zip.Dispose()}
New-Item -ItemType Directory -Path $targetDir -Force|Out-Null
$dest=Join-Path $targetDir 'LuminousValentineV3_Gate12_Android_ASTC_Isolated.apk'
if(Test-Path $dest){throw 'Existing verified APK preserved; no overwrite'}
Copy-Item -LiteralPath $apk -Destination $dest -ErrorAction Stop
$copyHash=(Get-FileHash $dest -Algorithm SHA256).Hash.ToLowerInvariant()
if($copyHash -ne $initial){throw 'Verified install-ready APK copy differs from original'}
$sdk='C:\Users\mimim\AppData\Local\Android\Sdk\platform-tools\adb.exe'
[ordered]@{
 status='PASS_SIGNED_SEPARATE_LUMINOUS_EXPERIMENTAL_APK_READY_FOR_PHYSICAL_INSTALL'
 app_id=$appId;not_the_normal_app='com.azrazieliz.OfflineGame'
 signing_certificate_sha256=($certificateLine -replace '^.*:\s*','').Trim()
 unsigned_or_mismatched=$false;apk_path=$dest
 apk_sha256=$copyHash;apk_bytes=(Get-Item $dest).Length
 apk_zip_entry_count=$entryCount;contains_arm64_native_library=$true
 cooked_data_inside_apk=$true
 normal_app_or_saved_data_uninstalled=$false
 original_game_saves_modified=$false
 physical_phone_connected=$false
 physical_phone_installed=$false
 physical_device_visual_performance_qa='NOT_TESTED'
 'missing_original_facial_morph_targets_45'='UNRESOLVED_BLOCKS_GATE11_ARTIFACT_READY'
 initial_animation_blending='PARTIAL'
 gate11_artifact_ready=$false
 adb_path=$sdk
 utc=[DateTime]::UtcNow.ToString('o')
}|ConvertTo-Json -Depth 5|Set-Content $verifyReceipt -Encoding UTF8
Write-Output "GATE12_SIGNED_APK_READY_FOR_PLUG_IN=$dest SHA256=$copyHash APP_ID=$appId"
