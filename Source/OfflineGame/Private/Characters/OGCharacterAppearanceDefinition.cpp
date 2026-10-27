#include "Characters/OGCharacterAppearanceDefinition.h"

#include "Misc/PackageName.h"

namespace
{
bool FailAppearanceContract(FString& Error, const TCHAR* Message)
{
    Error = Message;
    return false;
}

bool IsGameAsset(const FSoftObjectPath& Path)
{
    const FString Package = Path.GetLongPackageName();
    return Path.IsValid() && Path.GetSubPathString().IsEmpty() &&
        Package.StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive) &&
        FPackageName::IsValidLongPackageName(Package) && !Package.Contains(TEXT("..")) &&
        Path.GetAssetName() == FPackageName::GetShortName(Package);
}

bool ValidTransform(const FTransform& Value)
{
    const FVector Scale = Value.GetScale3D();
    return !Value.ContainsNaN() && Value.GetRotation().IsNormalized() &&
        FMath::Abs(Scale.X) > SMALL_NUMBER && FMath::Abs(Scale.Y) > SMALL_NUMBER &&
        FMath::Abs(Scale.Z) > SMALL_NUMBER;
}
}

const FOGCharacterAppearanceWardrobe* UOGCharacterAppearanceDefinition::FindWardrobe(FName Id) const
{
    const FOGCharacterAppearanceWardrobe* Match = nullptr;
    for (const FOGCharacterAppearanceWardrobe& Row : Wardrobes)
    {
        if (Row.WardrobeId != Id) continue;
        if (Match) return nullptr;
        Match = &Row;
    }
    return Match;
}

bool UOGCharacterAppearanceDefinition::ValidateContract(FString& OutError) const
{
    OutError.Reset();
    if (!IdentityId.IsValid() || !VersionId.IsValid() || WardrobeStateJsonField.IsNone() ||
        Wardrobes.IsEmpty() || !IsGameAsset(SharedMotionLibrary.ToSoftObjectPath()))
        return FailAppearanceContract(OutError, TEXT("Appearance needs exact Identity/Version, JSON field, wardrobes and a /Game/ motion library."));

    TSet<FName> WardrobeIds;
    for (const FOGCharacterAppearanceWardrobe& Row : Wardrobes)
    {
        if (Row.WardrobeId.IsNone() || WardrobeIds.Contains(Row.WardrobeId) ||
            Row.ExpectedPartCount <= 0 || Row.Parts.Num() != Row.ExpectedPartCount ||
            Row.RigProfile.IsNone() || Row.AnatomyProfile.IsNone() || Row.OutfitProfile.IsNone())
            return FailAppearanceContract(OutError, TEXT("Duplicate/invalid wardrobe, exact part count or authored motion profiles."));
        WardrobeIds.Add(Row.WardrobeId);
        TSet<FName> Capabilities;
        for (FName Capability : Row.Capabilities)
        {
            if (Capability.IsNone() || Capabilities.Contains(Capability))
                return FailAppearanceContract(OutError, TEXT("Motion capabilities must be explicit, nonempty names without duplicates."));
            Capabilities.Add(Capability);
        }
        TSet<FName> PartIds;
        TSet<FSoftObjectPath> MeshPaths;
        for (const FOGCharacterAppearancePart& Part : Row.Parts)
        {
            if (Part.PartId.IsNone() || PartIds.Contains(Part.PartId) ||
                !IsGameAsset(Part.Mesh.ToSoftObjectPath()) ||
                !IsGameAsset(Part.NativeSkeleton.ToSoftObjectPath()) ||
                MeshPaths.Contains(Part.Mesh.ToSoftObjectPath()) || !ValidTransform(Part.ComponentTransform) ||
                Part.BoneNames.IsEmpty() || Part.BoneNames.Num() != Part.BoneParentIndices.Num() ||
                (!Part.ReferenceLocalTransforms.IsEmpty() && Part.ReferenceLocalTransforms.Num() != Part.BoneNames.Num()))
                return FailAppearanceContract(OutError, TEXT("Invalid part ID, native asset path, transform or complete bone contract."));
            PartIds.Add(Part.PartId);
            MeshPaths.Add(Part.Mesh.ToSoftObjectPath());
            TSet<FName> Bones;
            for (int32 Index = 0; Index < Part.BoneNames.Num(); ++Index)
            {
                const int32 Parent = Part.BoneParentIndices[Index];
                if (Part.BoneNames[Index].IsNone() || Bones.Contains(Part.BoneNames[Index]) ||
                    (Index == 0 ? Parent != INDEX_NONE : Parent < 0 || Parent >= Index) ||
                    (!Part.ReferenceLocalTransforms.IsEmpty() && !ValidTransform(Part.ReferenceLocalTransforms[Index])))
                    return FailAppearanceContract(OutError, TEXT("Bone names/parents/rest transforms do not form an exact ordered native hierarchy."));
                Bones.Add(Part.BoneNames[Index]);
            }
            TSet<FName> Morphs;
            for (FName Morph : Part.MorphTargets)
            {
                if (Morph.IsNone() || Morphs.Contains(Morph))
                    return FailAppearanceContract(OutError, TEXT("Part morph inventory contains an empty or duplicate name."));
                Morphs.Add(Morph);
            }
        }
        TSet<FName> Semantics;
        for (const FOGCharacterSemanticExpression& Expression : Row.Expressions)
        {
            if (Expression.Semantic.IsNone() || Semantics.Contains(Expression.Semantic) || Expression.Morphs.IsEmpty())
                return FailAppearanceContract(OutError, TEXT("Expression semantics require unique names and explicit mappings."));
            Semantics.Add(Expression.Semantic);
            TMap<FName, TSet<FName>> Targets;
            for (const FOGCharacterExpressionMorph& Binding : Expression.Morphs)
            {
                const FOGCharacterAppearancePart* Part = Row.Parts.FindByPredicate(
                    [&Binding](const FOGCharacterAppearancePart& Candidate) { return Candidate.PartId == Binding.PartId; });
                if (!Part || !Part->MorphTargets.Contains(Binding.Morph) ||
                    !FMath::IsFinite(Binding.Scale) || Binding.Scale < -1.f || Binding.Scale > 1.f ||
                    Targets.FindOrAdd(Binding.PartId).Contains(Binding.Morph))
                    return FailAppearanceContract(OutError, TEXT("Expression mapping references a missing/duplicate exact part morph or invalid scale."));
                Targets.FindOrAdd(Binding.PartId).Add(Binding.Morph);
            }
        }
    }
    const FOGCharacterAppearanceWardrobe* Default = FindWardrobe(SafeDefaultWardrobe);
    if (!Default || Default->bMatureAppearance)
        return FailAppearanceContract(OutError, TEXT("Safe default must identify a non-mature row in the same wardrobe array."));
    for (const FOGCharacterAppearanceWardrobe& Row : Wardrobes)
    {
        const FOGCharacterAppearanceWardrobe* Fallback = FindWardrobe(Row.SafePrivacyFallback);
        if (!Fallback || Fallback->bMatureAppearance)
            return FailAppearanceContract(OutError, TEXT("Every wardrobe needs a direct non-mature privacy fallback in the same array."));
    }
    return true;
}

const FOGCharacterAppearanceWardrobe* UOGCharacterAppearanceDefinition::ResolveRenderedWardrobe(
    FName Requested, bool bSfw, FString& OutError) const
{
    if (!ValidateContract(OutError)) return nullptr;
    const FOGCharacterAppearanceWardrobe* Row = FindWardrobe(Requested);
    if (!Row)
    {
        OutError = TEXT("Requested wardrobe was not authored for this exact Identity/Version.");
        return nullptr;
    }
    return bSfw && Row->bMatureAppearance ? FindWardrobe(Row->SafePrivacyFallback) : Row;
}
