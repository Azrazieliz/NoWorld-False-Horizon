param([int]$First = 1, [int]$Last = 11)
$ErrorActionPreference = 'Stop'
$r = 'D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010'
$root = Join-Path $r 'Saved\Gate12Readiness\FirstWorldR1_MergedSource\eastern_market_original_object_shards'
$engine = 'D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$script = Join-Path $r 'Scripts\Gate12World\gate12_import_one_original_market_section_ue583.py'
$manifest = Join-Path $root 'eastern_market_original_object_shards_receipt.json'
$source = Get-Content $manifest -Raw | ConvertFrom-Json
if ($source.status -ne 'ORIGINAL_EASTERN_MARKET_SOURCE_PRESERVED_12_FBXS_PENDING_NATIVE_UE' -or $source.shards.Count -ne 12) {
    throw 'Source artist geometry is not fully validated'
}
if ($First -lt 1 -or $Last -gt 11 -or $First -gt $Last) {
    throw 'Invalid section range'
}
$ledgerPath = Join-Path $root 'gate12_incremental_native_market_checkpoint.json'
$ledger = [ordered]@{
    schema = 'noworld.gate12.easternmarket.unreal.nativeincremental.v1'
    source_sha256 = $source.source_sha256
    current_section = -1
    attempted_sections = @()
    status = 'IN_PROGRESS'
    message = 'Each section independently saved by UE commandlet; no original art overwritten'
}
function Save-Ledger {
    $ledger | ConvertTo-Json -Depth 12 | Set-Content -Path $ledgerPath -Encoding utf8
}
Save-Ledger
for ($i = $First; $i -le $Last; $i++) {
    $env:GATE12_MARKET_SECTION = [string]$i
    $tag = '{0:D2}' -f $i
    $receiptPath = Join-Path $root "market_native_intake_section_$tag.json"
    $logPath = Join-Path $root "UE_market_section_$tag.log"
    $ledger.current_section = $i
    Save-Ledger
    $rpt = $null
    if (Test-Path $receiptPath) {
        $rpt = Get-Content $receiptPath -Raw | ConvertFrom-Json
        if ($rpt.status -eq 'REAL_SOURCE_MARKET_SECTION_NATIVE_IMPORTED_NOT_MOBILE_OR_ART_APPROVED' -and $rpt.ue_triangles -gt 0) {
            Write-Output "PRESERVED_ALREADY_VERIFIED_SECTION_$tag"
            $ledger.attempted_sections += [ordered]@{section=$i; status='PRESERVED_VERIFIED'; triangles=$rpt.ue_triangles}
            Save-Ledger
            continue
        }
    }
    & $engine "$r\OfflineGame.uproject" '-run=pythonscript' "-script=$script" '-unattended' '-nop4' '-nosplash' '-NoSound' '-NullRHI' '-log' > $logPath 2>&1
    $exit = $LASTEXITCODE
    $errors = @(Select-String -Path $logPath -Pattern 'LogInterchangeEngine: Error:|LogPython: Error:|LogInit: Display: Failure' -ErrorAction SilentlyContinue).Count
    if (Test-Path $receiptPath) {
        $rpt = Get-Content $receiptPath -Raw | ConvertFrom-Json
    }
    $status = if ($rpt) { $rpt.status } else { 'NO_RECEIPT' }
    $ledger.attempted_sections += [ordered]@{section=$i; status=$status; triangles=$(if($rpt){$rpt.ue_triangles}else{0});exit=$exit;error_lines=$errors}
    Save-Ledger
    Write-Output "MARKET_SECTION_$tag STATUS=$status EXIT=$exit ERRORS=$errors"
    if ($exit -ne 0 -or $errors -gt 0 -or $status -ne 'REAL_SOURCE_MARKET_SECTION_NATIVE_IMPORTED_NOT_MOBILE_OR_ART_APPROVED') {
        $ledger.status = 'STOPPED_ON_SOURCE_QUALITY_OR_COMMANDLET_FAILURE'
        Save-Ledger
        throw "Native market section $tag rejected; original source and prior sections protected"
    }
}
$ledger.status = 'ALL_REQUESTED_MARKET_SOURCE_SECTIONS_NATIVE_IMPORTED_UNBUILT'
Save-Ledger
Write-Output "MARKET_NATIVE_IMPORT_CHECKPOINT=$($ledger.status)"
