param()
$ErrorActionPreference='Stop'
$root='D:\UnrealProjects\OfflineGame_BlenderMastery_20261015\Software_Automation\Blender_4.5.10\MATERIALS\PolyHaven_CC0_1k'
New-Item -ItemType Directory -Path $root -Force | Out-Null
$ua=@{'User-Agent'='NoWorld-BlenderMastery-AssetAudit/0.5'}
$results=@()
foreach($id in @('granite_wall','rough_block_wall','stony_dirt_path')){
 $meta=Invoke-RestMethod -Uri ('https://api.polyhaven.com/files/'+$id) -Headers $ua -TimeoutSec 25
 foreach($map in @('Diffuse','Rough','nor_gl')){
  $variants=$meta.$map.'1k'
  $data=$variants.jpg
  if(-not $data){$data=$variants.png}
  if(-not $data){throw "No expected 1k JPG or PNG map for $id $map"}
  $ext=if($data.url.EndsWith('.png')){'png'}else{'jpg'}
  $dest=Join-Path $root ("$id" + "_"+$map+"."+$ext)
  if(-not (Test-Path $dest) -or (Get-Item $dest).Length -ne $data.size){
   Invoke-WebRequest -Uri $data.url -Headers $ua -OutFile $dest -TimeoutSec 90
  }
  $bytes=(Get-Item $dest).Length
  $md5=(Get-FileHash -Path $dest -Algorithm MD5).Hash.ToLowerInvariant()
  if($md5 -ne $data.md5){throw "MD5 mismatch for $id $map"}
  $results += [ordered]@{asset=$id;map=$map;file=Split-Path $dest -Leaf;bytes=$bytes;md5=$md5;url=$data.url;license='CC0';author_credit='Poly Haven';verified=$true}
  Write-Output ('PBR_VERIFIED='+$id+'/'+$map+':'+$bytes)
 }
}
$manifest=[ordered]@{source='https://polyhaven.com';source_api='https://api.polyhaven.com';license='CC0';api_user_agent='NoWorld-BlenderMastery-AssetAudit/0.5';revisions='API validated on 2026-10-15';maps=$results}
$manifest|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'SOURCE_PROVENANCE.json') -Encoding UTF8
Write-Output ('PBR_FILES_VERIFIED='+$results.Count)
