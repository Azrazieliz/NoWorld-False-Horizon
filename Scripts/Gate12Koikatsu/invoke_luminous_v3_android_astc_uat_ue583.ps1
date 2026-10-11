$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$archive=Join-Path $w 'Saved\Gate12Koikatsu\LuminousV3\AndroidLuminousIsolatedASTC_v2'
$uat='D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat'
$map='/Game/Experimental/LuminousValentineV3/AndroidReady_20261011/Maps/L_LuminousV3_IsolatedAndroid'
if(!(Test-Path $uat) -or !(Test-Path (Join-Path $w 'OfflineGame.uproject'))){throw 'RunUAT project missing'}
$args=@('BuildCookRun',('-project="'+(Join-Path $w 'OfflineGame.uproject')+'"'),
 '-noP4','-platform=Android','-cookflavor=ASTC','-clientconfig=Development',
 '-skipbuild','-nocompileeditor','-cook','-stage','-pak','-package','-archive',
 ('-archivedirectory="'+$archive+'"'),('-map='+$map),
 '-unattended','-utf8output','-NoXGE','-MaxParallelActions=1')
Write-Output ('GATE12_LUMINOUS_UAT_COMMAND=BuildCookRun Android_ASTC isolated Luminous map '+$map)
& $uat @args
$exit=$LASTEXITCODE
Write-Output "GATE12_LUMINOUS_UAT_EXIT=$exit"
exit $exit
