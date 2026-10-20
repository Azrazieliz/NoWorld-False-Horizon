$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$build='D:\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat'
$log="$w\Saved\Gate12Readiness\Gate12_20261020_FourAuthenticTouchEmotes_EditorBuild.log"
if(!(Test-Path $build)){throw 'Missing engine Build.bat'}
$other=Get-Process -Name UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool -ErrorAction SilentlyContinue
if($other){Write-Output 'EDITOR_OR_BUILD_ALREADY_ACTIVE';exit 12}
$mem=Get-CimInstance Win32_OperatingSystem
if($mem.FreePhysicalMemory/1024 -lt 2400){Write-Output 'EDITOR_BUILD_RAM_GUARD';exit 12}
& $build OfflineGameEditor Win64 Development "-Project=$w\OfflineGame.uproject" -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUba *> $log
$rc=$LASTEXITCODE
Write-Output "G12_FOUR_CLIP_EDITOR_BUILD_EXIT=$rc LOG=$log"
if(Test-Path $log){Get-Content $log -Tail 15}
exit $rc
