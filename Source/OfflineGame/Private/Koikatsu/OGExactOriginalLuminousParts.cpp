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

static const FExactForm GForms[] = {
 { TEXT("character:luminous_valentine.version_awakened_v4"), 1073, GAwakenedParts, UE_ARRAY_COUNT(GAwakenedParts) },
 { TEXT("character:luminous_valentine.version_berserker_v4"), 882, GBerserkerParts, UE_ARRAY_COUNT(GBerserkerParts) },
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
            return Form.Count == 9;
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
