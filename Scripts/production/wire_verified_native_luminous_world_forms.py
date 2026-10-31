"""Wire only 2026-10-31 verified original-card imports into real World pawns.
The same one character-wide motion controller remains attached; no per-form catalogs."""
from pathlib import Path
R=Path(__file__).resolve().parents[2]
h=R/"Source/OfflineGame/Public/Koikatsu/OGLuminousV3PlayableCharacter.h"
c=R/"Source/OfflineGame/Private/Koikatsu/OGLuminousV3PlayableCharacter.cpp"
catalog=R/"Source/OfflineGame/Private/UI/OGGate12CharacterVisualCatalog.cpp"
header=h.read_text("utf8");source=c.read_text("utf8");view=catalog.read_text("utf8")
assert header.count("class OFFLINEGAME_API AOGLuminousV3PlayableCharacter final")==1
header=header.replace("class OFFLINEGAME_API AOGLuminousV3PlayableCharacter final","class OFFLINEGAME_API AOGLuminousV3PlayableCharacter")
classes='''
/** True Koikatsu original-card World pawn: all 9 installed native parts, one motion controller. */
UCLASS()
class OFFLINEGAME_API AOGLuminousAwakenedOriginalWorldCharacter final
    : public AOGLuminousV3PlayableCharacter
{
    GENERATED_BODY()
public:
    AOGLuminousAwakenedOriginalWorldCharacter();
};

/** No unpaired historical 886-bone substitute source is admitted here. */
UCLASS()
class OFFLINEGAME_API AOGLuminousBerserkerOriginalWorldCharacter final
    : public AOGLuminousV3PlayableCharacter
{
    GENERATED_BODY()
public:
    AOGLuminousBerserkerOriginalWorldCharacter();
};
'''
assert "class OFFLINEGAME_API AOGLuminousAwakenedOriginalWorldCharacter" not in header
header+=classes
source=source.replace('#include "Koikatsu/OGLuminousV3PlayableCharacter.h"',
    '#include "Koikatsu/OGLuminousV3PlayableCharacter.h"\n#include "Koikatsu/OGExactOriginalLuminousParts.h"',1)
source=source.replace('Left.GetNum() != 975 && Left.GetNum() != 886 &&',
    'Left.GetNum() != 975 && Left.GetNum() != 886 &&\n         Left.GetNum() != 1073 && Left.GetNum() != 882 &&',1)
# Exact additional source assets use new true namespace, never the prior candidate.
before='''        const bool bNewOriginalSource = !VersionNamespace.IsEmpty() &&
            AssetPath.StartsWith(VersionNamespace);'''
after='''        const bool bVerifiedExactClothed =
            Desired == EOGLuminousV3Wardrobe::Clothed &&
            !Form.IsEmpty() &&
            (Form == TEXT("Awakened") || Form == TEXT("Berserker")) &&
            AssetPath.StartsWith(FString::Printf(
                TEXT("/Game/NoWorld/Characters/Luminous/%s/ExactOriginalClothed/"),
                *Form));
        const bool bNewOriginalSource = (!VersionNamespace.IsEmpty() &&
            AssetPath.StartsWith(VersionNamespace)) || bVerifiedExactClothed;'''
assert source.count(before)==1;source=source.replace(before,after)
# Bind exact approved source versions: constructor of subclasses fills all paths;
# an installed world actor always remains tied to its own form.
new='''AOGLuminousAwakenedOriginalWorldCharacter::AOGLuminousAwakenedOriginalWorldCharacter()
{
    CanonicalLuminousVersionId =
        TEXT("character:luminous_valentine.version_awakened_v4");
    int32 Bones = 0;
    if (FOGExactOriginalLuminousParts::GetClothed(
        CanonicalLuminousVersionId, ClothedNativeParts, Bones))
    {
        AdditionalVersionNativePartCounts = {ClothedNativeParts.Num(), 0, 0};
        AdditionalVersionNativeSkeletonBones = {Bones, 0, 0};
    }
}

AOGLuminousBerserkerOriginalWorldCharacter::AOGLuminousBerserkerOriginalWorldCharacter()
{
    CanonicalLuminousVersionId =
        TEXT("character:luminous_valentine.version_berserker_v4");
    int32 Bones = 0;
    if (FOGExactOriginalLuminousParts::GetClothed(
        CanonicalLuminousVersionId, ClothedNativeParts, Bones))
    {
        AdditionalVersionNativePartCounts = {ClothedNativeParts.Num(), 0, 0};
        AdditionalVersionNativeSkeletonBones = {Bones, 0, 0};
    }
}
'''
source += "\n"+new
# Accept strict original exporter joint count with only 0/1 root introduced by UE,
# and require all 9 parts to carry the SAME actual bone count.
needle='''    for (int32 Index = 0; Index < Paths.Num(); ++Index)
    {
        USkeletalMesh* NativeMesh = Cast<USkeletalMesh>(Paths[Index].TryLoad());'''
repl='''    int32 WholeFormBoneCount = INDEX_NONE;
    const bool bExactSourceClothed =
        Desired == EOGLuminousV3Wardrobe::Clothed &&
        (CanonicalLuminousVersionId ==
            TEXT("character:luminous_valentine.version_awakened_v4") ||
         CanonicalLuminousVersionId ==
            TEXT("character:luminous_valentine.version_berserker_v4"));
    for (int32 Index = 0; Index < Paths.Num(); ++Index)
    {
        USkeletalMesh* NativeMesh = Cast<USkeletalMesh>(Paths[Index].TryLoad());'''
assert source.count(needle)==1;source=source.replace(needle,repl)
old='''        if (!NativeMesh || !NativeMesh->GetSkeleton() ||
            NativeMesh->GetRefSkeleton().GetNum() != ExpectedNativeBoneCount(Desired))
        {
            Error = FString::Printf(TEXT("Luminous V3: missing/invalid source rig %d bones at index %d state %s"),
                ExpectedNativeBoneCount(Desired), Index, WardrobeName(Desired));
            return false;
        }
        Prepared.Add(NativeMesh);'''
new='''        const int32 ActualBones = NativeMesh ? NativeMesh->GetRefSkeleton().GetNum() : 0;
        const int32 Expected = ExpectedNativeBoneCount(Desired);
        const bool bRigMatches = bExactSourceClothed
            ? (ActualBones >= Expected && ActualBones <= Expected + 1 &&
               (WholeFormBoneCount == INDEX_NONE || WholeFormBoneCount == ActualBones))
            : (ActualBones == Expected);
        if (!NativeMesh || !NativeMesh->GetSkeleton() || !bRigMatches)
        {
            Error = FString::Printf(
                TEXT("Luminous: missing or mismatched authentic native source rig expected=%d actual=%d index=%d state=%s"),
                Expected, ActualBones, Index, WardrobeName(Desired));
            return false;
        }
        WholeFormBoneCount = ActualBones;
        Prepared.Add(NativeMesh);'''
assert source.count(old)==1;source=source.replace(old,new)
assert 'AOGLuminousBerserkerOriginalWorldCharacter::AOGLuminousBerserkerOriginalWorldCharacter' in source
h.write_text(header,"utf8");c.write_text(source,"utf8")
print("NATIVE_WORLD_RIGS_WIRED",h.name,c.name)
