param([int]$MaxSeconds=185)
$ErrorActionPreference='Stop'
$r='D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$exe='D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$qa='/Game/Gate12NativeKoikatsu20261020/VisualQA/L_LuminousV3_Dance_Full45_UE583_VisualQA'
$accept="$r\Saved\Gate12AnimationAcceptance"
$receipt="$accept\Gate12_FULL45_OneShaderWorker_LiveVisualQA_20261021.json"
$shots="$accept\Gate12_FULL45_LiveOneWorkerQA_20261021.png"
Set-Item -Path 'Env:UE-LocalDataCachePath' -Value "$r\Saved\Gate12NativeDDC"
$env:TEMP="$r\Saved\Gate12NativeBuildTemp";$env:TMP=$env:TEMP
$cmd=@("$r\OfflineGame.uproject",$qa,'-game','-windowed','-ResX=1280','-ResY=720',
  '-NoSound','-nosplash','-DDC=(Local)',
  "-LocalDataCachePath=$r\Saved\Gate12NativeDDC",
  '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=8',
  '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreadsDuringGame=8')
$existing=Get-CimInstance Win32_Process|Where-Object {$_.CommandLine -like "*$qa*" -and $_.Name -like 'UnrealEditor*'}
if($existing){throw 'Protected existing visual QA process. No duplicate'}
$state=[ordered]@{status='STARTED';max_seconds=$MaxSeconds;project=$r;map=$qa;
  method='One shader worker via process commandline overrides, D-drive local DDC, bounded RAM';
  process_id=$null;map_load_log_found=$false;game_window_seen=$false;
  visual_image=$shots;actual_full_body_accepted=$false;errors=@();samples=@()}
$p=$null
try {
 $p=Start-Process -FilePath $exe -ArgumentList $cmd -PassThru
 $state.process_id=$p.Id
 $t=[DateTime]::UtcNow
 while(!$p.HasExited) {
  Start-Sleep -Seconds 5;$p.Refresh()
  $os=Get-CimInstance Win32_OperatingSystem
  $ram=[math]::Round($os.FreePhysicalMemory/1024)
  $sec=[math]::Round(([DateTime]::UtcNow-$t).TotalSeconds)
  if($sec%20 -lt 6){Write-Output "GATE09_FULL45_SHADER_WORKER_ONE_TIME=$sec RAM_FREE_MB=$ram"}
  $state.samples+=@([ordered]@{sec=$sec;ram_free_mb=$ram;main_window=[string]$p.MainWindowTitle})
  if($p.MainWindowHandle -ne 0){$state.game_window_seen=$true}
  $log="$r\Saved\Logs\OfflineGame.log"
  if((Test-Path $log) -and (Select-String -Path $log -SimpleMatch -Pattern "Load map complete $qa" -Quiet)){
   $state.map_load_log_found=$true
  }
  if($ram -lt 850){$state.status='STOPPED_BELOW_850MB_SAFE_RAM';break}
  if($state.map_load_log_found -and $state.game_window_seen){
   $state.status='MAP_LOADED_GUI_READY_PENDING_ACTUAL_FRAME_QA';break
  }
  if($sec -ge $MaxSeconds){$state.status='TIME_BOUND_BEFORE_VISUAL_ACCEPTANCE';break}
 }
 if($p.HasExited -and $state.status -eq 'STARTED'){$state.status='GAME_PROCESS_EXITED_BEFORE_VISUAL'}
 if($state.map_load_log_found -and $state.game_window_seen) {
  $py="from PIL import ImageGrab; im=ImageGrab.grab(); im.save(r'$shots'); print('SCREENSHOT_WRITTEN',im.size)"
  & python -c $py
  if(Test-Path $shots){$state.screenshot_bytes=(Get-Item $shots).Length}
 }
} catch {
 $state.status='EXCEPTION_NOT_ACCEPTED';$state.errors+=@([string]$_)
} finally {
 if($p){
  $id=$p.Id
  $live=Get-CimInstance Win32_Process -Filter "ProcessId=$id"
  if($live -and $live.CommandLine -like "*$qa*" -and $live.Name -eq 'UnrealEditor.exe'){
   Stop-Process -Id $id -Force -ErrorAction SilentlyContinue
  }
  # Remove only proven shader compiler children belonging to this QA process.
  Get-CimInstance Win32_Process -Filter "ParentProcessId=$id" -ErrorAction SilentlyContinue|
    Where-Object {$_.Name -eq 'ShaderCompileWorker.exe'}|
    ForEach-Object {Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue}
 }
 $state|ConvertTo-Json -Depth 8|Set-Content -Path $receipt -Encoding UTF8
 Write-Output "GATE09_FULL45_LIVE_QA_STATUS=$($state.status) RECEIPT=$receipt"
}
