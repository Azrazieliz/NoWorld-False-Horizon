$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$uat='D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
$r="$w\Saved\Gate12Readiness\FourAuthenticEmotesAndroid_20261020"
$archive="$r\Archive"
New-Item -ItemType Directory -Force -Path $r | Out-Null
$sourceBinary="$w\Binaries\Android\OfflineGame-arm64.so"
$nativeReceipt="$w\Saved\Gate12Koikatsu\NativeAndroidARM64\arm64_native_build_receipt.json"
if(!(Test-Path $sourceBinary) -or !(Test-Path $nativeReceipt)){throw 'Current ARM64 native library and verified receipt required'}
$receipt=Get-Content $nativeReceipt -Raw | ConvertFrom-Json
if($receipt.status -ne 'PASS_ANDROID_ARM64_NATIVE_LINK'){throw 'ARM64 native link not accepted'}
if((Get-FileHash $sourceBinary -Algorithm SHA256).Hash.ToLowerInvariant() -ne $receipt.rebuilt_binary_sha256){throw 'Current binary differs from ARM64 receipt'}
$device=(& adb devices -l) -join "`n"
if($device -notmatch 'R3GYC0LSC7K\s+device'){Write-Output 'NOT_STARTED_PHYSICAL_DEVICE_DISCONNECTED';exit 12}
$o=Get-CimInstance Win32_OperatingSystem
if($o.FreePhysicalMemory/1024 -lt 1900 -or $o.FreeVirtualMemory/1024 -lt 4000){Write-Output 'NOT_STARTED_COOK_MEMORY_GUARD';exit 12}
$env:ANDROID_HOME='C:\Users\mimim\AppData\Local\Android\Sdk'
$env:ANDROID_SDK_ROOT=$env:ANDROID_HOME
$env:NDKROOT=Join-Path $env:ANDROID_HOME 'ndk\27.2.12479018'
$env:NDK_ROOT=$env:NDKROOT
$env:JAVA_HOME='C:\Program Files\Android\Android Studio\jbr'
$map='/Game/Experimental/LuminousValentineV3/AndroidReady_20261011/Maps/L_LuminousV3_IsolatedAndroid'
$args=@('BuildCookRun',('-project="'+$w+'\OfflineGame.uproject"'),'-noP4','-platform=Android','-cookflavor=ASTC','-clientconfig=Development','-skipbuild','-nocompileeditor','-cook','-stage','-pak','-package','-archive',('-archivedirectory="'+$archive+'"'),('-map='+$map),'-unattended','-utf8output','-NoXGE','-MaxParallelActions=1')
& $uat @args *> "$r\uat.log"
$rc=$LASTEXITCODE
Write-Output "FOUR_AUTHENTIC_EMOTES_ANDROID_UAT_EXIT=$rc"
if(Test-Path "$r\uat.log"){Get-Content "$r\uat.log" -Tail 18}
exit $rc
