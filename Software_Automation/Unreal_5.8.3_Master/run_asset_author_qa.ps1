$ErrorActionPreference='Continue'
$env:NW_UNREAL_AUTHOR_JOB='D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unreal_5.8.3_Master\native_author_job_example.json'
$e='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$p='D:\UnrealProjects\NW_BlenderInterop_Sandbox_20261015\NW_BlenderInterop.uproject'
$s='D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Unreal_5.8.3_Master\native_asset_author.py'
& $e $p '-run=pythonscript' ("-script=$s") '-nullrhi' '-unattended' '-nosplash' '-nop4' '-NoSound'
Write-Output "NOWORLD_AUTHOR_HOST_EXIT=$LASTEXITCODE"
