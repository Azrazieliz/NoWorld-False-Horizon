param([int]$MaxWaitSeconds=480,[int]$CookMaxSeconds=450)
$ErrorActionPreference='Stop'
$w='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$r="$w\Saved\Gate12Koikatsu\PlayableASTC"
New-Item -ItemType Directory -Path $r -Force|Out-Null
$start=[DateTime]::UtcNow
$conditions=0;$samples=@()
while(([DateTime]::UtcNow-$start).TotalSeconds -lt $MaxWaitSeconds){
 $m=Get-CimInstance Win32_OperatingSystem
 $free=[math]::Round($m.FreePhysicalMemory/1024)
 $commit=[math]::Round($m.FreeVirtualMemory/1024)
 $elapsed=[math]::Round(([DateTime]::UtcNow-$start).TotalSeconds)
 $samples+=@([pscustomobject]@{second=$elapsed;physicalMB=$free;commitMB=$commit})
 if($free -ge 1900 -and $commit -ge 3900){$conditions++}else{$conditions=0}
 if($elapsed%30 -lt 12){Write-Output "PLAYABLE_COOK_RESOURCE_WAIT_T=$elapsed PHYSICAL_FREE_MB=$free COMMIT_HEADROOM_MB=$commit"}
 if($conditions -ge 2){break}
 Start-Sleep -Seconds 12
}
if($conditions -lt 2){
 $v=[ordered]@{status='WAIT_TIMEOUT_OTHER_PROCESS_MEMORY_PRESSURE';time=[DateTime]::UtcNow.ToString('o');samples=$samples}
 $v|ConvertTo-Json -Depth 5|Set-Content "$r\playable_cook_resource_window.json" -Encoding UTF8
 Write-Output 'PLAYABLE_COOK_NOT_LAUNCHED_UNRELATED_WORKLOADS_ACTIVE'
 exit 12
}
[ordered]@{status='PASS_MEASURED_START_RESOURCE_WINDOW';samples=$samples}|ConvertTo-Json -Depth 5|Set-Content "$r\playable_cook_resource_window.json" -Encoding UTF8
Write-Output 'PLAYABLE_COOK_MEASURED_RESOURCE_WINDOW_PASS_STARTING_NATIVE_UNREAL_COOK'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$w\Scripts\Gate12Koikatsu\run_playable_android_astc_cook_lowram_ue583.ps1" -MaxSeconds $CookMaxSeconds
Write-Output "PLAYABLE_COOK_NATIVE_WRAPPER_EXIT=$LASTEXITCODE"
$log="$r\ue583_android_playable_map_cook.log"
if(Test-Path $log){
 $raw=Get-Content $log -Raw
 $ok=$raw -match 'Success - 0 error\(s\), 0 warning\(s\)'
 $map="$w\Saved\Cooked\Android_ASTC\OfflineGame\Content\Experimental\Gate12Koikatsu\PlayableQA\Maps\L_KoikatsuPlayable_UE583.umap"
 [ordered]@{status=if($ok -and (Test-Path $map)){'PASS_NATIVE_ANDROID_ASTC_PLAYABLE_LEVEL_COOK'}else{'NOT_PASS_NATIVE_ANDROID_ASTC_PLAYABLE_LEVEL_COOK'};mapCooked=(Test-Path $map);nativeCookerSuccess=$ok;sourceMap='/Game/Experimental/Gate12Koikatsu/PlayableQA/Maps/L_KoikatsuPlayable_UE583';runCompletedUTC=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json -Depth 4|Set-Content "$r\playable_cook_acceptance.json" -Encoding UTF8
 Write-Output ("PLAYABLE_COOK_ACCEPTANCE_MAP_PRESENT="+(Test-Path $map)+" COOKER_ZERO_ERROR="+$ok)
}
