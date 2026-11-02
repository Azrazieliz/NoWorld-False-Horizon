param(
    [string]$EngineRoot = $env:UE_ROOT,
    [string]$ArchiveDirectory = "",
    [switch]$AndroidFirst,
    [switch]$InstallOnDevice
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$Project = Join-Path $Root "OfflineGame.uproject"
$GateStamp = Join-Path $Root "Saved\Automation\last_unreal_gate.json"
$SourceCommit = (& git -C $Root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $SourceCommit -notmatch '^[0-9a-fA-F]{40}$') {
    throw "Native Android package requires a recorded GitHub source commit."
}
$BuildStartUtc = [DateTime]::UtcNow
$PreferredSdkRoot = if ($env:OG_ANDROID_SDK_ROOT) {
    $env:OG_ANDROID_SDK_ROOT
} else {
    "D:\Tools\AndroidSdk"
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory=$true)][string]$FilePath,
        [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath"
    }
}

function Assert-AuthenticFirstWorld {
    # These are original source contracts, NOT random terrain fallbacks.
    $Map = Join-Path $Root "Content\Maps\StartingWorld.umap"
    if (-not (Test-Path -LiteralPath $Map -PathType Leaf)) {
        throw "The original tracked StartingWorld carrier map is missing."
    }
    $Source = Join-Path $Root "Content\NoWorld\FirstWorld\Source"
    $Required = @(
        "height_513_float32.raw", "soil_mask_513_float32.raw",
        "rock_mask_513_float32.raw", "river_mask_513_float32.raw",
        "baked_seed1412_scatter.jsonl"
    )
    $Missing = @($Required | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $Source $_) -PathType Leaf)
    })
    if ($Missing.Count -gt 0) {
        throw ("Original Gate02 R1 First World data missing: " +
            ($Missing -join ", ") +
            ". Recover the real authored source instead of packaging an empty World.")
    }
    $GridBytes = [long]513*513*4
    foreach ($Name in @($Required | Where-Object { $_ -like "*.raw" })) {
        if ((Get-Item -LiteralPath (Join-Path $Source $Name)).Length -ne $GridBytes) {
            throw "Original 513x513 float grid length mismatch: $Name"
        }
    }
    $Scatter = Join-Path $Source "baked_seed1412_scatter.jsonl"
    if ((Get-Content -LiteralPath $Scatter | Measure-Object -Line).Lines -ne 10454) {
        throw "Original Gate02 R1 ecology must have exactly 10454 genuine records."
    }
    $Motion = Join-Path $Root "Content\NoWorld\Animation\SharedMotionCatalog.json"
    if (-not (Test-Path -LiteralPath $Motion -PathType Leaf)) {
        throw "Original shared motion bank is missing."
    }
    $Catalog = Get-Content -LiteralPath $Motion -Raw | ConvertFrom-Json
    if ($Catalog.schema -ne "noworld.shared_motion_catalog.v1" -or
        $Catalog.motions.Count -lt 3222) {
        throw "Original 3222-entry native shared motion catalog missing or truncated."
    }
    Write-Host "Original source World, ecology and motion-bank preflight passed."
}

function Find-Unreal58 {
    param([string]$Preferred)

    $candidates = New-Object System.Collections.Generic.List[string]
    if ($Preferred) { $candidates.Add($Preferred) }

    foreach ($drive in Get-PSDrive -PSProvider FileSystem) {
        if (-not $drive.Root) { continue }
        $candidates.Add((Join-Path $drive.Root "Epic Games\UE_5.8"))
        $candidates.Add((Join-Path $drive.Root "UE_5.8"))
        $candidates.Add((Join-Path $drive.Root "Unreal\UE_5.8"))
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        $runUat = Join-Path $candidate "Engine\Build\BatchFiles\RunUAT.bat"
        if (Test-Path $runUat) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "Unreal Engine 5.8 with RunUAT.bat was not found."
}

# Android-first is an explicit developer build path, not a declaration that
# Win64 Editor automation passed. BuildCookRun still compiles, cooks, stages
# and packages real Unreal game code and source data before any installation.
if (-not $AndroidFirst) {
    if (-not (Test-Path $GateStamp)) {
        throw "Android packaging requires the Win64 compile/automation gate, or explicit -AndroidFirst for a native device-validation build."
    }
    $Gate = Get-Content $GateStamp -Raw | ConvertFrom-Json
    $Head = (& git -C $Root rev-parse HEAD).Trim()
    if ($Gate.commit -ne $Head -or
        $Gate.compile -ne "passed" -or
        $Gate.automation -ne "passed") {
        throw "Android packaging gate stamp is stale or does not match HEAD $Head."
    }
} else {
    Write-Host "Android-first build: Win64 automation is NOT marked passed." -ForegroundColor Yellow
}

Assert-AuthenticFirstWorld
$EngineRoot = Find-Unreal58 $EngineRoot
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"

if (Test-Path $PreferredSdkRoot) {
    $env:ANDROID_HOME = $PreferredSdkRoot
    $env:ANDROID_SDK_ROOT = $PreferredSdkRoot

    $NdkRoot = Join-Path $PreferredSdkRoot "ndk\27.2.12479018"
    if (Test-Path $NdkRoot) {
        $env:NDKROOT = $NdkRoot
    }
}

$AndroidStudioJdk = "C:\Program Files\Android\Android Studio\jbr"
if (Test-Path $AndroidStudioJdk) {
    $env:JAVA_HOME = $AndroidStudioJdk
}

if (-not $ArchiveDirectory) {
    $ArchiveDirectory = Join-Path $Root "Saved\AndroidBuild"
}

New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null
# Mandatory low-memory compilation and caches remain on the D: worktree.
$HostScratch = Join-Path $Root "Saved\HostBuildScratch"
New-Item -ItemType Directory -Force -Path $HostScratch | Out-Null
$env:TEMP = $HostScratch
$env:TMP = $HostScratch
$env:UE_LocalDataCachePath = Join-Path $Root "Saved\DerivedData\Local"
$env:UE_SharedDataCachePath = Join-Path $Root "Saved\DerivedData\Shared"
$env:OG_LOW_MEMORY_BUILD = "1"

Write-Host "=== Android SDK verification ===" -ForegroundColor Cyan
Invoke-Checked -FilePath $RunUAT -Arguments @(
    "Turnkey",
    "-command=VerifySdk",
    "-platform=Android",
    "-utf8output"
)

Write-Host "=== Android Development package ===" -ForegroundColor Cyan
Invoke-Checked -FilePath $RunUAT -Arguments @(
    "BuildCookRun",
    "-project=$Project",
    "-noP4",
    "-platform=Android",
    "-clientconfig=Development",
    "-build",
    "-cook",
    "-stage",
    "-package",
    "-archive",
    "-archivedirectory=$ArchiveDirectory",
    "-utf8output"
)

# A successful RunUAT exit is necessary but never sufficient: verify a real
# current generated APK before offering or attempting to install it.
$Artifact = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -Filter "*.apk" -File |
    Where-Object { $_.LastWriteTimeUtc -ge $BuildStartUtc -and $_.Length -gt 1000000 } |
    Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if (-not $Artifact) {
    throw "RunUAT exited but produced no NEW nontrivial Android APK for this exact build; older APKs are rejected."
}
$ReceiptPath = Join-Path $ArchiveDirectory "noworld_android_build_receipt.json"
$Receipt = [ordered]@{
    schema = "noworld.android.build_receipt.v1"
    source_commit = $SourceCommit
    build_started_utc = $BuildStartUtc.ToString("o")
    apk_path = $Artifact.FullName
    apk_sha256 = (Get-FileHash -LiteralPath $Artifact.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    apk_size_bytes = [long]$Artifact.Length
    native_build = "RunUAT BuildCookRun"
    owner_phone_accepted = $false
}
$Receipt | ConvertTo-Json | Set-Content -LiteralPath $ReceiptPath -Encoding UTF8
Write-Host "Android package generated with authentic first World and SHA256 source receipt." -ForegroundColor Green
Write-Host "Build receipt: $ReceiptPath"
Write-Host "APK: $($Artifact.FullName)" -ForegroundColor Green
Write-Host "APK size (bytes): $($Artifact.Length)"

if ($InstallOnDevice) {
    $Adb = Join-Path $PreferredSdkRoot "platform-tools\adb.exe"
    if (-not (Test-Path $Adb)) { throw "ADB missing from configured SDK." }
    $Connected = @(& $Adb devices | Where-Object { $_ -match '^\S+\s+device\s*$' })
    if ($Connected.Count -ne 1) {
        throw "Expected exactly one authorized Android device; got $($Connected.Count)."
    }
    Invoke-Checked -FilePath $Adb -Arguments @("install", "-r", $Artifact.FullName)
    Write-Host "Native APK installed on the connected Android device." -ForegroundColor Green
    Write-Host "Owner gameplay acceptance remains outstanding." -ForegroundColor Yellow
}
