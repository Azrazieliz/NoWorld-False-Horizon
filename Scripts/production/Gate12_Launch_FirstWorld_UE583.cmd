@echo off
setlocal
set "PROJECT=D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010"
set "UE-ZenDataPath=%PROJECT%\Saved\Gate12NativeZenData"
set "UE-LocalDataCachePath=%PROJECT%\Saved\Gate12NativeDDC"
if not exist "%UE-ZenDataPath%" mkdir "%UE-ZenDataPath%"
if not exist "%UE-LocalDataCachePath%" mkdir "%UE-LocalDataCachePath%"
start "" "D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%\OfflineGame.uproject" "-ZenDataPath=%UE-ZenDataPath%" "-LocalDataCachePath=%UE-LocalDataCachePath%"
endlocal
