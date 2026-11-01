# NoWorld 5★: compile/activate our own Maker queue service, not user's mods.
param([string]$Game='D:\Tools\Steam\steamapps\common\Koikatsu Party')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source=Join-Path $repo 'Tools\Koikatsu\NoworldContentFactory\NoworldCardExportQueue.cs'
$compiler='C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe'
if(!(Test-Path $compiler) -or !(Test-Path $source)){throw 'Real .NET C# compiler or factory plugin source missing'}
$names=@('BepInEx.dll','Assembly-CSharp.dll','UnityEngine.dll','KKAPI.dll')
$refs=@()
foreach($name in $names){
 $matches=@(Get-ChildItem -LiteralPath $Game -File -Filter $name -Recurse -ErrorAction SilentlyContinue)
 if($matches.Count -eq 0){throw ('Required original Koikatsu DLL absent: '+$name)}
 $refs+=($matches |Sort-Object FullName|Select-Object -First 1).FullName
}
$dir=Join-Path $Game 'BepInEx\plugins\NoWorldFactory'
New-Item -ItemType Directory -Force $dir|Out-Null
$target=Join-Path $dir 'NoworldCardExportQueue.dll'
$staging=Join-Path $dir 'NoworldCardExportQueue.new.dll'
& $compiler /nologo /target:library /optimize+ ('/out:'+$staging) @($refs|ForEach-Object{'/reference:'+$_}) $source
if($LASTEXITCODE -ne 0 -or !(Test-Path $staging)){throw 'Native Maker queue plugin compilation blocked'}
if(Test-Path $target){
 $a=(Get-FileHash $target -Algorithm SHA256).Hash
 $b=(Get-FileHash $staging -Algorithm SHA256).Hash
 if($a -ne $b){
  $backup=Join-Path $dir ('NoworldCardExportQueue.bak.'+[DateTime]::UtcNow.ToString('yyyyMMddHHmmss')+'.dll')
  Copy-Item -LiteralPath $target -Destination $backup -ErrorAction Stop
  Move-Item -LiteralPath $staging -Destination $target -Force
 }else{Remove-Item -LiteralPath $staging -Force}
}else{Move-Item -LiteralPath $staging -Destination $target}
Write-Output ('NOWORLD_FACTORY_API_PLUGIN='+$target)
