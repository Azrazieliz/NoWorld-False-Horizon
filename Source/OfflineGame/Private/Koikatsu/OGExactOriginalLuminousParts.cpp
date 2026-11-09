#include "Koikatsu/OGExactOriginalLuminousParts.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "UObject/UObjectGlobals.h"

namespace {
struct FExactForm { const TCHAR* Version; int32 SourceBones; const TCHAR* const* Parts; int32 Count; };
static const TCHAR* const GAwakenedParts[] = {
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_01/Luminous_Awakened_TrueOriginalPart_01/SkeletalMeshes/Luminous_Awakened_rueOriginalPart_01.Luminous_Awakened_rueOriginalPart_01"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_02/Luminous_Awakened_TrueOriginalPart_02/SkeletalMeshes/Luminous_Awakened_rueOriginalPart_02.Luminous_Awakened_rueOriginalPart_02"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_03/Luminous_Awakened_TrueOriginalPart_03/SkeletalMeshes/Luminous_Awakened_rueOriginalPart_03.Luminous_Awakened_rueOriginalPart_03"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_04/Luminous_Awakened_TrueOriginalPart_04/SkeletalMeshes/Luminous_Awakened_rueOriginalPart_04.Luminous_Awakened_rueOriginalPart_04"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_05/Luminous_Awakened_TrueOriginalPart_05/SkeletalMeshes/Luminous_Awakened_rueOriginalPart_05.Luminous_Awakened_rueOriginalPart_05"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_00/Group_00/Mesh_00_PrimGroup_00-02/SkeletalMeshes/Mesh_00_PrimGroup_00-02.Mesh_00_PrimGroup_00-02"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_00/Group_01/Mesh_00_PrimGroup_03-05/SkeletalMeshes/Mesh_00_PrimGroup_03-05.Mesh_00_PrimGroup_03-05"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_00/Group_02/Mesh_00_PrimGroup_06-08/SkeletalMeshes/Mesh_00_PrimGroup_06-08.Mesh_00_PrimGroup_06-08"),
    TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/Part_00/Group_03/Mesh_00_PrimGroup_09-10/SkeletalMeshes/Mesh_00_PrimGroup_09-10.Mesh_00_PrimGroup_09-10"),
};

static const TCHAR* const GBerserkerParts[] = {
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_01/Luminous_Berserker_TrueOriginalPart_01/SkeletalMeshes/Luminous_Berserk_ueOriginalPart_01.Luminous_Berserk_ueOriginalPart_01"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_02/Luminous_Berserker_TrueOriginalPart_02/SkeletalMeshes/Luminous_Berserk_ueOriginalPart_02.Luminous_Berserk_ueOriginalPart_02"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_03/Luminous_Berserker_TrueOriginalPart_03/SkeletalMeshes/Luminous_Berserk_ueOriginalPart_03.Luminous_Berserk_ueOriginalPart_03"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_04/Luminous_Berserker_TrueOriginalPart_04/SkeletalMeshes/Luminous_Berserk_ueOriginalPart_04.Luminous_Berserk_ueOriginalPart_04"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_05/Luminous_Berserker_TrueOriginalPart_05/SkeletalMeshes/Luminous_Berserk_ueOriginalPart_05.Luminous_Berserk_ueOriginalPart_05"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_00/Group_00/Mesh_00_PrimGroup_00-02/SkeletalMeshes/Mesh_00_PrimGroup_00-02.Mesh_00_PrimGroup_00-02"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_00/Group_01/Mesh_00_PrimGroup_03-05/SkeletalMeshes/Mesh_00_PrimGroup_03-05.Mesh_00_PrimGroup_03-05"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_00/Group_02/Mesh_00_PrimGroup_06-08/SkeletalMeshes/Mesh_00_PrimGroup_06-08.Mesh_00_PrimGroup_06-08"),
    TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/Part_00/Group_03/Mesh_00_PrimGroup_09-10/SkeletalMeshes/Mesh_00_PrimGroup_09-10.Mesh_00_PrimGroup_09-10"),
};

static const TCHAR* const GGrandParts[] = {
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_000.Grand_ExactRenderer_000"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_06/Luminous_Grand_ExactOriginalClothed_Part_06/SkeletalMeshes/Grand_ExactRenderer_001.Grand_ExactRenderer_001"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_05/Luminous_Grand_ExactOriginalClothed_Part_05/SkeletalMeshes/Grand_ExactRenderer_002.Grand_ExactRenderer_002"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_01/Luminous_Grand_ExactOriginalClothed_Part_01/SkeletalMeshes/Grand_ExactRenderer_003.Grand_ExactRenderer_003"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_08/Luminous_Grand_ExactOriginalClothed_Part_08/SkeletalMeshes/Grand_ExactRenderer_004.Grand_ExactRenderer_004"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_03/Luminous_Grand_ExactOriginalClothed_Part_03/SkeletalMeshes/Grand_ExactRenderer_005.Grand_ExactRenderer_005"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_006.Grand_ExactRenderer_006"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_05/Luminous_Grand_ExactOriginalClothed_Part_05/SkeletalMeshes/Grand_ExactRenderer_007.Grand_ExactRenderer_007"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_06/Luminous_Grand_ExactOriginalClothed_Part_06/SkeletalMeshes/Grand_ExactRenderer_008.Grand_ExactRenderer_008"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_009.Grand_ExactRenderer_009"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_02/Luminous_Grand_ExactOriginalClothed_Part_02/SkeletalMeshes/Grand_ExactRenderer_010.Grand_ExactRenderer_010"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_03/Luminous_Grand_ExactOriginalClothed_Part_03/SkeletalMeshes/Grand_ExactRenderer_011.Grand_ExactRenderer_011"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_01/Luminous_Grand_ExactOriginalClothed_Part_01/SkeletalMeshes/Grand_ExactRenderer_012.Grand_ExactRenderer_012"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_013.Grand_ExactRenderer_013"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_02/Luminous_Grand_ExactOriginalClothed_Part_02/SkeletalMeshes/Grand_ExactRenderer_014.Grand_ExactRenderer_014"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_08/Luminous_Grand_ExactOriginalClothed_Part_08/SkeletalMeshes/Grand_ExactRenderer_015.Grand_ExactRenderer_015"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_05/Luminous_Grand_ExactOriginalClothed_Part_05/SkeletalMeshes/Grand_ExactRenderer_016.Grand_ExactRenderer_016"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_06/Luminous_Grand_ExactOriginalClothed_Part_06/SkeletalMeshes/Grand_ExactRenderer_017.Grand_ExactRenderer_017"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_00/Luminous_Grand_ExactOriginalClothed_Part_00/SkeletalMeshes/Luminous_Grand_E_lClothed_Part_00.Luminous_Grand_E_lClothed_Part_00"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_019.Grand_ExactRenderer_019"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_01/Luminous_Grand_ExactOriginalClothed_Part_01/SkeletalMeshes/Grand_ExactRenderer_020.Grand_ExactRenderer_020"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_021.Grand_ExactRenderer_021"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_03/Luminous_Grand_ExactOriginalClothed_Part_03/SkeletalMeshes/Grand_ExactRenderer_022.Grand_ExactRenderer_022"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_06/Luminous_Grand_ExactOriginalClothed_Part_06/SkeletalMeshes/Grand_ExactRenderer_023.Grand_ExactRenderer_023"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_024.Grand_ExactRenderer_024"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_03/Luminous_Grand_ExactOriginalClothed_Part_03/SkeletalMeshes/Grand_ExactRenderer_025.Grand_ExactRenderer_025"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_05/Luminous_Grand_ExactOriginalClothed_Part_05/SkeletalMeshes/Grand_ExactRenderer_027.Grand_ExactRenderer_027"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_06/Luminous_Grand_ExactOriginalClothed_Part_06/SkeletalMeshes/Grand_ExactRenderer_028.Grand_ExactRenderer_028"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_08/Luminous_Grand_ExactOriginalClothed_Part_08/SkeletalMeshes/Grand_ExactRenderer_029.Grand_ExactRenderer_029"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_01/Luminous_Grand_ExactOriginalClothed_Part_01/SkeletalMeshes/Grand_ExactRenderer_030.Grand_ExactRenderer_030"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_031.Grand_ExactRenderer_031"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_05/Luminous_Grand_ExactOriginalClothed_Part_05/SkeletalMeshes/Grand_ExactRenderer_032.Grand_ExactRenderer_032"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_033.Grand_ExactRenderer_033"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_037.Grand_ExactRenderer_037"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_038.Grand_ExactRenderer_038"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_03/Luminous_Grand_ExactOriginalClothed_Part_03/SkeletalMeshes/Grand_ExactRenderer_039.Grand_ExactRenderer_039"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_01/Luminous_Grand_ExactOriginalClothed_Part_01/SkeletalMeshes/Grand_ExactRenderer_040.Grand_ExactRenderer_040"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_044.Grand_ExactRenderer_044"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_06/Luminous_Grand_ExactOriginalClothed_Part_06/SkeletalMeshes/Grand_ExactRenderer_047.Grand_ExactRenderer_047"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_02/Luminous_Grand_ExactOriginalClothed_Part_02/SkeletalMeshes/Grand_ExactRenderer_048.Grand_ExactRenderer_048"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_08/Luminous_Grand_ExactOriginalClothed_Part_08/SkeletalMeshes/Grand_ExactRenderer_049.Grand_ExactRenderer_049"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_02/Luminous_Grand_ExactOriginalClothed_Part_02/SkeletalMeshes/Grand_ExactRenderer_050.Grand_ExactRenderer_050"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_01/Luminous_Grand_ExactOriginalClothed_Part_01/SkeletalMeshes/Grand_ExactRenderer_054.Grand_ExactRenderer_054"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_03/Luminous_Grand_ExactOriginalClothed_Part_03/SkeletalMeshes/Grand_ExactRenderer_057.Grand_ExactRenderer_057"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_02/Luminous_Grand_ExactOriginalClothed_Part_02/SkeletalMeshes/Grand_ExactRenderer_058.Grand_ExactRenderer_058"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_060.Grand_ExactRenderer_060"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_08/Luminous_Grand_ExactOriginalClothed_Part_08/SkeletalMeshes/Grand_ExactRenderer_062.Grand_ExactRenderer_062"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_08/Luminous_Grand_ExactOriginalClothed_Part_08/SkeletalMeshes/Grand_ExactRenderer_066.Grand_ExactRenderer_066"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_04/Luminous_Grand_ExactOriginalClothed_Part_04/SkeletalMeshes/Grand_ExactRenderer_067.Grand_ExactRenderer_067"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_02/Luminous_Grand_ExactOriginalClothed_Part_02/SkeletalMeshes/Grand_ExactRenderer_068.Grand_ExactRenderer_068"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_071.Grand_ExactRenderer_071"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_05/Luminous_Grand_ExactOriginalClothed_Part_05/SkeletalMeshes/Grand_ExactRenderer_072.Grand_ExactRenderer_072"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_06/Luminous_Grand_ExactOriginalClothed_Part_06/SkeletalMeshes/Grand_ExactRenderer_074.Grand_ExactRenderer_074"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_07/Luminous_Grand_ExactOriginalClothed_Part_07/SkeletalMeshes/Grand_ExactRenderer_075.Grand_ExactRenderer_075"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_03/Luminous_Grand_ExactOriginalClothed_Part_03/SkeletalMeshes/Grand_ExactRenderer_076.Grand_ExactRenderer_076"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_08/Luminous_Grand_ExactOriginalClothed_Part_08/SkeletalMeshes/Grand_ExactRenderer_080.Grand_ExactRenderer_080"),
    TEXT("/Game/NoWorld/Characters/Luminous/Grand/ExactOriginalClothed/Part_05/Luminous_Grand_ExactOriginalClothed_Part_05/SkeletalMeshes/Grand_ExactRenderer_081.Grand_ExactRenderer_081"),
};

static const FExactForm GForms[] = {
 { TEXT("character:luminous_valentine.version_awakened_v4"), 1073, GAwakenedParts, UE_ARRAY_COUNT(GAwakenedParts) },
 { TEXT("character:luminous_valentine.version_berserker_v4"), 882, GBerserkerParts, UE_ARRAY_COUNT(GBerserkerParts) },
 { TEXT("character:luminous_valentine.version_grand_v4"), 561, GGrandParts, UE_ARRAY_COUNT(GGrandParts) },
};
}

bool FOGExactOriginalLuminousParts::GetClothed(const FString& VersionId,
    TArray<FSoftObjectPath>& OutPaths, int32& OutSourceBones)
{
    OutPaths.Reset(); OutSourceBones = 0;
    for (const FExactForm& Form : GForms)
        if (VersionId == Form.Version)
        {
            for (int32 I = 0; I < Form.Count; ++I)
                OutPaths.Add(FSoftObjectPath(Form.Parts[I]));
            OutSourceBones = Form.SourceBones;
            return Form.Count == 9 || (VersionId == TEXT("character:luminous_valentine.version_grand_v4") && Form.Count == 57);
        }
    return false;
}

bool FOGExactOriginalLuminousParts::IsInstalled(const FString& VersionId)
{
    TArray<FSoftObjectPath> Paths; int32 SourceBones = 0;
    if (!GetClothed(VersionId, Paths, SourceBones)) return false;
    for (const FSoftObjectPath& Path : Paths)
    {
        USkeletalMesh* Mesh = Cast<USkeletalMesh>(Path.TryLoad());
        if (!Mesh || !Mesh->GetSkeleton() ||
            Mesh->GetRefSkeleton().GetNum() < SourceBones)
            return false;
    }
    return true;
}
