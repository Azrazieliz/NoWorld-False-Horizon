using UnrealBuildTool;
using System.IO;

public class OfflineGame : ModuleRules
{
    public OfflineGame(ReadOnlyTargetRules Target) : base(Target)
    {
        // This 8 GB Android build workstation cannot compile Unreal's shared PCH
        // without paging into a full system drive. Leave normal developer builds
        // untouched; use truly single-file incremental compilation when requested.
        bool LowMemoryBuild = System.Environment.GetEnvironmentVariable("OG_LOW_MEMORY_BUILD") == "1";
        PCHUsage = LowMemoryBuild ? PCHUsageMode.NoPCHs : PCHUsageMode.UseExplicitOrSharedPCHs;
        if (LowMemoryBuild)
        {
            // Small incremental unity batches amortize header parsing without
            // a monolithic shared PCH or large all-module unity translation unit.
            bUseUnity = true;
            NumIncludedBytesPerUnityCPPOverride = 128 * 1024;
        }

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "ProceduralMeshComponent"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AIModule",
            "AndroidPermission",
            "ApplicationCore",
            "AssetRegistry",
            "InputCore",
            "Json",
            "JsonUtilities",
            "PakFile",
            "PlatformCrypto",
            "PlatformCryptoContext",
            "PlatformCryptoTypes",
            "SQLiteCore",
            "Slate",
            "SlateCore",
            "UMG"
        });

        RuntimeDependencies.Add(
            "$(ProjectDir)/Content/Data/FoundationClockPolicy.json",
            StagedFileType.UFS);

        RuntimeDependencies.Add(
            "$(ProjectDir)/Config/Foundation/CharacterVisualDiagnostic.json",
            StagedFileType.UFS);

        // Shared native animation definitions must be available offline in
        // packaged Win64 and Android builds, not only inside the Editor.
        RuntimeDependencies.Add(
            "$(ProjectDir)/Content/NoWorld/Animation/SharedMotionCatalog.json",
            StagedFileType.UFS);

        // Shared source packets are ordinary offline UFS content, never per-character assets.
        string SharedMotionRoot = Path.GetFullPath(Path.Combine(ModuleDirectory,
            "../../Content/NoWorld/Animation/SharedSource"));
        if (Directory.Exists(SharedMotionRoot))
            foreach (string MotionFile in Directory.EnumerateFiles(SharedMotionRoot, "*", SearchOption.AllDirectories))
                RuntimeDependencies.Add(MotionFile, StagedFileType.UFS);

        // Original authored Gate02 R1 terrain/mask/scatter files are loose
        // offline World inputs read with FFileHelper, not discoverable via UObject
        // cooker references. Require all originals for Android and stage as UFS.
        string R1SourceDir = Path.GetFullPath(Path.Combine(ModuleDirectory,
            "../../Content/NoWorld/FirstWorld/Source"));
        string[] R1Inputs = {
            "height_513_float32.raw",
            "soil_mask_513_float32.raw",
            "rock_mask_513_float32.raw",
            "river_mask_513_float32.raw",
            "baked_seed1412_scatter.jsonl"
        };
        foreach (string Name in R1Inputs)
        {
            string OriginalInput = Path.Combine(R1SourceDir, Name);
            if (Target.Platform == UnrealTargetPlatform.Android && !File.Exists(OriginalInput))
                throw new BuildException("Original Gate02 R1 input is missing: " + OriginalInput);
            if (File.Exists(OriginalInput))
                RuntimeDependencies.Add(OriginalInput, StagedFileType.UFS);
        }

        // The exact five Gate01 accepted original 2D PNG files are loaded at
        // runtime as transient Texture2D resources. Cooked UAssets cannot
        // replace their original immutable source hashes.
        string AcceptedArtRoot = Path.GetFullPath(Path.Combine(ModuleDirectory,
            "../../Content/NoWorld/Characters/Luminous/Art/Accepted2D"));
        string[] AcceptedArtFiles = {
            "01_BASE.png",
            "02_ALTER.png",
            "03_AWAKENED.png",
            "04_BERSERKER.png",
            "05_GRAND_CONVERGENCE.png",
            "APPROVED_FIVE_FORMS_MANIFEST.json"
        };
        foreach (string ArtFile in AcceptedArtFiles)
        {
            string AbsolutePath = Path.Combine(AcceptedArtRoot, ArtFile);
            if (!File.Exists(AbsolutePath))
                throw new BuildException("Missing accepted Gate01 original: " + AbsolutePath);
            RuntimeDependencies.Add(AbsolutePath, StagedFileType.UFS);
        }
        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            AdditionalPropertiesForReceipt.Add(
                "AndroidPlugin",
                Path.Combine(ModuleDirectory, "OfflineGame_APL.xml"));
        }
    }
}
