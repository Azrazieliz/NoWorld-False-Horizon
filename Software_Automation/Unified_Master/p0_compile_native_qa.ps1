$ErrorActionPreference='Stop'
$g='D:\Tools\Steam\steamapps\common\Koikatsu Party'
$r='D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME'
$qa='D:\UnrealProjects\NW_SoftwareMaster_P0_20261016\B_CAPTURE_PLUGIN'
$mono='D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\TOOLS\Unity_5.6.2f1_SM01\Editor\Data\Mono\lib\mono\2.0'
if(!(Test-Path (Join-Path $qa 'P0NativeAssembledProbe.cs'))) {throw 'P0 staged source missing'}
$original=(Get-FileHash "$r\MODDING\SM01RuntimeController_Unity35Pinned.dll" -Algorithm SHA256).Hash
$refs=@("$mono\mscorlib.dll","$mono\System.dll","$mono\System.Core.dll","$g\BepInEx\core\BepInEx.dll",
 "$g\BepInEx\plugins\KKAPI.dll","$g\BepInEx\plugins\KK_BepisPlugins\Sideloader.dll",
 "$g\Koikatsu Party_Data\Managed\Assembly-CSharp.dll","$g\Koikatsu Party_Data\Managed\UnityEngine.dll",
 "$g\BepInEx\plugins\KKABMX.dll","$g\BepInEx\plugins\KK_Plugins\KK_MaterialEditor.dll")
$out=Join-Path $qa 'P0_SM01RuntimeController_Unity35_QA.dll'
$args=@('/nologo','/noconfig','/target:library','/nostdlib+','/optimize+',('/out:'+$out))
foreach($ref in $refs) {$args+=('/reference:'+$ref)}
$args+=@(Get-ChildItem $qa -Filter '*.cs'|Select-Object -ExpandProperty FullName)
& 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe' @args
if($LASTEXITCODE -ne 0) {throw 'P0 compile failed'}
$now=(Get-FileHash "$r\MODDING\SM01RuntimeController_Unity35Pinned.dll" -Algorithm SHA256).Hash
if($now -ne $original){throw 'Master source compiler default DLL was modified'}
Write-Output "P0_COMPILE_PASS output=$out sha=$((Get-FileHash $out -Algorithm SHA256).Hash) original_master_sha=$now"
