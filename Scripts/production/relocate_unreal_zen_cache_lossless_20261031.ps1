# Relocate the disposable COMMON Zen derived cache, keep every byte.
# Refuses an active Zen server, conflicting destinations, or any hash mismatch.
$ErrorActionPreference='Stop'
$source=Join-Path $env:LOCALAPPDATA 'UnrealEngine\Common\Zen\Data'
$dest='D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Saved\RelocatedUECommonZenData_20261031'
$parent=Split-Path $source -Parent
$backup=Join-Path $parent 'Data_SourceVerifiedBackup_20261031'
$proof='D:\UnrealProjects\Gate12_WholeCharacter_Build_20261031\Saved\ZenCacheLosslessRelocation_20261031.json'
if(Get-Process zenserver -ErrorAction SilentlyContinue){throw 'Zen server is active; do not change its storage'}
if(!(Test-Path $source) -or (Get-Item $source).Attributes.ToString().Contains('ReparsePoint')){throw 'Original plain Zen data not found'}
if(Test-Path $backup){throw 'Zen backup already exists; refusing source mutation'}
$sourceFiles=@(Get-ChildItem -LiteralPath $source -File -Recurse)
$rows=@()
foreach($file in $sourceFiles){
 $rel=$file.FullName.Substring($source.Length).TrimStart('\')
 $rows+=@{relative=$rel;size=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
}
New-Item -ItemType Directory -Force -Path $dest|Out-Null
& robocopy $source $dest /E /COPY:DAT /DCOPY:DAT /R:1 /W:1 /NP /NFL /NDL | Out-Null
if($LASTEXITCODE -ge 8){throw 'Lossless Zen cache copy failed'}
foreach($row in $rows){
 $file=Join-Path $dest $row.relative
 if(!(Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).Length -ne $row.size -or
   (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $row.sha256){
  throw 'Disagreement during byte-for-byte source backup verification'
 }
}
$report=@{schema='noworld.unreal_zen_cache_lossless_relocation.v1';source=$source;
 destination=$dest;files_verified=$rows.Count;bytes_verified=[long](($sourceFiles|Measure-Object -Property Length -Sum).Sum);
 SHA256_verified=$true;source_assets_untouched=$true;source_bytes=$rows}
$report|ConvertTo-Json -Depth 7|Set-Content -LiteralPath $proof -Encoding UTF8
Rename-Item -LiteralPath $source -NewName (Split-Path $backup -Leaf)
try{
 New-Item -ItemType Junction -Path $source -Target $dest -ErrorAction Stop | Out-Null
 if(!(Test-Path $source) -or (Get-Item $source).Attributes.ToString() -notmatch 'ReparsePoint'){
  throw 'Zen new directory not a junction'
 }
 if((Get-ChildItem $source -File -Recurse).Count -ne $rows.Count){throw 'New junction does not expose all files'}
} catch {
 if(Test-Path $source){Remove-Item -LiteralPath $source -Force -ErrorAction SilentlyContinue}
 Rename-Item -LiteralPath $backup -NewName 'Data'
 throw
}
# Exact copied cache on D is verified above. Remove only the original cache duplicate,
# never game sources, project assets, user saves or unrelated app files.
Remove-Item -LiteralPath $backup -Recurse -Force
Write-Host 'UE_ZEN_DERIVED_CACHE_LOSSLESS_RELOCATED' $rows.Count $dest
Write-Host 'C_FREE_AFTER_RELOCATION_MB' ([math]::Round((Get-PSDrive C).Free/1MB))
