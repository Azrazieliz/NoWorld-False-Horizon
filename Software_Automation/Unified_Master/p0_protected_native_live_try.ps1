$ErrorActionPreference='Stop'
$qa='D:\UnrealProjects\NW_SoftwareMaster_P0_20261016\B_CAPTURE_PLUGIN'
$s='D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\SANDBOXES\SM01_KKParty_6436979_20261014'
$r='D:\Tools\GameAssetPipeline\ProductionAutomation\Software_Automation\MASTER_PACKAGES\KoikatsuParty_Steam6436979_HF4.2_SM01_v0.4.0_RUNTIME\AUTOMATION'
$plugin="$s\BepInEx\plugins\SM01RuntimeController.dll"
$original="$qa\SM01RuntimeController_SandboxOriginal_Backup.dll"
$qaPlugin="$qa\P0_SM01RuntimeController_Unity35_QA.dll"
$manifest='D:\UnrealProjects\NW_SoftwareMaster_P0_20261016\B_NATIVE_QA_SOURCE_MANIFEST.json'
$m=Get-Content $manifest -Raw | ConvertFrom-Json
if(!(Test-Path $original) -or !(Test-Path $qaPlugin)){throw 'QA DLL or original source backup absent'}
$originalHash=(Get-FileHash $original -Algorithm SHA256).Hash
if((Get-FileHash $plugin -Algorithm SHA256).Hash -ne $originalHash){throw 'Sandbox already modified, refusing deployment'}
foreach($k in $m.source_inputs.PSObject.Properties.Name) {
 $a=$m.source_inputs.$k
 $sha=(Get-FileHash -LiteralPath $a.source -Algorithm SHA256).Hash
 $csha=(Get-FileHash -LiteralPath $a.clone -Algorithm SHA256).Hash
 if($sha -ne $a.sha256 -or $sha -ne $csha){throw "Clone/hash changed: $k"}
}
$live=@(Get-Process 'Koikatsu Party' -ErrorAction SilentlyContinue)
if($live.Count -gt 0){throw 'A Koikatsu process is already running; do not interrupt it'}
Write-Output 'P0_NATIVE_PREFLIGHT=PASS two_hashlocked_clones_originals_unchanged'
try {
 Copy-Item $qaPlugin $plugin -Force -ErrorAction Stop
 if((Get-FileHash $plugin -Algorithm SHA256).Hash -ne (Get-FileHash $qaPlugin -Algorithm SHA256).Hash){throw 'QA plugin staging failed'}
 Write-Output 'P0_NATIVE_QA_PLUGIN_STAGED'
 $app=Start-Process "$s\Koikatsu Party.exe" -WorkingDirectory $s -PassThru
 Write-Output "P0_NATIVE_SANDBOX_PID=$($app.Id)"
 Start-Sleep -Seconds 6
 $py='D:\Tools\Python311\python.exe'
 Write-Output 'P0_MAKER_NAV_START'
 & $py "$r\navigate_maker_guarded.py" --pid $app.Id --limit 55
 Write-Output "P0_MAKER_NAV_EXIT=$LASTEXITCODE"
 if($LASTEXITCODE -ne 0){throw 'Guarded Maker navigation did not reach ready'}
 $client="$qa\p0_native_client.py"
 & $py $client --op p0_load_source_clone --file-name 'P0_BASE_CLOTHED.png' --nonce 'P0_CLONE_BASE_LOAD_20261016_1545' --timeout 15
 Write-Output "P0_BASE_LOAD_RC=$LASTEXITCODE"
 if($LASTEXITCODE -ne 0){throw 'Native cloned card loading failed'}
 & $py $client --op p0_inspect_assembled --nonce 'P0_BASE_ASSEMBLED_INSPECT_20261016_1545' --timeout 15
 Write-Output "P0_BASE_NATIVE_RENDERERS_RC=$LASTEXITCODE"
 if($LASTEXITCODE -ne 0){throw 'Native assembled renderer readback failed'}
 & $py $client --op p0_load_source_clone --file-name 'P0_ALTER_CANDIDATE_224249541.png' --nonce 'P0_CLONE_ALTER_LOAD_20261016_1545' --timeout 15
 if($LASTEXITCODE -ne 0){throw 'Alter clone load failed'}
 & $py $client --op p0_inspect_assembled --nonce 'P0_ALTER_ASSEMBLED_INSPECT_20261016_1545' --timeout 15
 if($LASTEXITCODE -ne 0){throw 'Alter assembled renderer readback failed'}
 Write-Output 'P0_NATIVE_BOTH_CLONES_READBACK_PASS'
} catch {
 Write-Output ("P0_NATIVE_CAPTURE_BLOCKER="+$_.Exception.Message)
} finally {
 $proc=@(Get-Process 'Koikatsu Party' -ErrorAction SilentlyContinue | Where-Object {$_.Path -like "$s*"})
 foreach($x in $proc){Stop-Process -Id $x.Id -Force -ErrorAction SilentlyContinue}
 Start-Sleep -Seconds 2
 for($attempt=0;$attempt -lt 10;$attempt++){
  try {
   Copy-Item -LiteralPath $original -Destination $plugin -Force -ErrorAction Stop
   if((Get-FileHash $plugin -Algorithm SHA256).Hash -eq $originalHash){Write-Output 'P0_SANDBOX_PLUGIN_RESTORED=PASS';break}
  }catch{Start-Sleep -Seconds 1}
 }
 if((Get-FileHash $plugin -Algorithm SHA256).Hash -ne $originalHash){throw 'P0 ORIGINAL SANDBOX DLL RESTORE FAILED'}
}
