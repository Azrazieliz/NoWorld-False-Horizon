@echo off
setlocal
set "PROJECT=D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010"
set "UE-ZenDataPath=%PROJECT%\Saved\Gate12NativeZenData"
set "UE-LocalDataCachePath=%PROJECT%\Saved\Gate12NativeDDC"
if not exist "%UE-ZenDataPath%" mkdir "%UE-ZenDataPath%"
if not exist "%UE-LocalDataCachePath%" mkdir "%UE-LocalDataCachePath%"
start "NoWorld - False Horizon - First World" "D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%\OfflineGame.uproject" /Game/NoWorld/FirstWorld/Maps/L_FirstWorld_Gate02_R1_StreamingRuntime -game -log -windowed -ResX=1280 -ResY=720 "-ZenDataPath=%UE-ZenDataPath%" "-LocalDataCachePath=%UE-LocalDataCachePath%" "-ExecCmds=Editor.AsyncStaticMeshCompilationMaxConcurrency 1,Editor.AsyncSkeletalMeshCompilationMaxConcurrency 1"
endlocal
