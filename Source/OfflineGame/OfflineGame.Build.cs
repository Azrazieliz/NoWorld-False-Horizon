using UnrealBuildTool;
using System.IO;

public class OfflineGame : ModuleRules
{
    public OfflineGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
            AdditionalPropertiesForReceipt.Add(
                "AndroidPlugin",
                Path.Combine(ModuleDirectory, "OfflineGame_APL.xml"));
        }
    }
}
