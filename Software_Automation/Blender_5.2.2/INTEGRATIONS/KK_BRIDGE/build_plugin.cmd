@echo off
setlocal
set ROOT=D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_5.2.2\INTEGRATIONS\KK_BRIDGE
set GAME=D:\Tools\Steam\steamapps\common\Koikatsu Party
set CSC=C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe
"%CSC%" /nologo /target:library /optimize+ /out:"%ROOT%\NoWorld.BlenderBridge.dll" /reference:"%GAME%\BepInEx\core\BepInEx.dll" /reference:"%GAME%\BepInEx\plugins\KKAPI.dll" /reference:"%GAME%\BepInEx\plugins\KK_ObjImport.dll" /reference:"%GAME%\Koikatsu Party_Data\Managed\UnityEngine.dll" /reference:"%GAME%\Koikatsu Party_Data\Managed\Assembly-CSharp.dll" "%ROOT%\NoWorld.BlenderBridge.cs"
if errorlevel 1 exit /b 1
echo BRIDGE_CSC_COMPILED_OK
