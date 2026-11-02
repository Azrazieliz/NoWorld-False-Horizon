# Build the existing offline NoWorld Android test APK without running gameplay tests.
# Source/content ingestion is intentionally independent of this packaging step.
[CmdletBinding()]
param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path,
    [string]$EngineRoot = 'D:\Epic Games\UE_5.8',
    [string]$ArchiveRoot = '',
    [string]$TempRoot = '',
    [string]$CookFlavor = 'ASTC'
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path $ProjectRoot).Path
$ProjectFile = Join-Path $ProjectRoot 'OfflineGame.uproject'
$RunUAT = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
if (-not (Test-Path $ProjectFile -PathType Leaf)) { throw "Missing actual Unreal project: $ProjectFile" }
if (-not (Test-Path $RunUAT -PathType Leaf)) { throw "Missing installed UE BuildCookRun: $RunUAT" }
if (-not $ArchiveRoot) { $ArchiveRoot = Join-Path $ProjectRoot 'Saved/AndroidTestBuild' }
if (-not $TempRoot) { $TempRoot = Join-Path $ProjectRoot 'Saved/AndroidBuildTemp' }
New-Item -ItemType Directory -Force $ArchiveRoot, $TempRoot | Out-Null
$CacheDir = Join-Path $TempRoot 'UnrealDDC'
New-Item -ItemType Directory -Force $CacheDir | Out-Null
$LogPath = Join-Path $TempRoot 'noworld_android_build.log'
$BatchPath = Join-Path $TempRoot 'run_noworld_android_build.cmd'
# Batch wrapper keeps Unreal's native exit code and all compile/cook/packaging
# diagnostics. It does not modify the protected source projects or run QA suites.
$Lines = @(
    '@echo off',
    "set ""TEMP=$TempRoot""",
    "set ""TMP=$TempRoot""",
    "set ""UE-LocalDataCachePath=$CacheDir""",
    ('call "' + $RunUAT + '" BuildCookRun -project="' + $ProjectFile +
      '" -noP4 -platform=Android -cookflavor=' + $CookFlavor +
      ' -clientconfig=Development -build -cook -stage -pak -package -archive -archivedirectory="' +
      $ArchiveRoot + '" -unattended -utf8output -nocompileeditor > "' + $LogPath + '" 2>&1'),
    'exit /b %ERRORLEVEL%'
)
Set-Content -LiteralPath $BatchPath -Value $Lines -Encoding ASCII
Write-Host "Android BuildCookRun: $ProjectFile"
Write-Host "Archive destination: $ArchiveRoot"
Write-Host "Build log: $LogPath"
& cmd.exe /d /c $BatchPath
$Result = $LASTEXITCODE
if ($Result -ne 0) {
    if (Test-Path $LogPath) { Get-Content $LogPath -Tail 70 | Write-Host }
    throw "Android BuildCookRun failed with exit code $Result. See $LogPath"
}
$Packages = @(Get-ChildItem -Path $ArchiveRoot -Filter '*.apk' -Recurse -File -ErrorAction SilentlyContinue)
if ($Packages.Count -eq 0) { throw "BuildCookRun exited successfully but produced no APK under $ArchiveRoot" }
$Packages | ForEach-Object { Write-Host ("Android test APK: " + $_.FullName) }
