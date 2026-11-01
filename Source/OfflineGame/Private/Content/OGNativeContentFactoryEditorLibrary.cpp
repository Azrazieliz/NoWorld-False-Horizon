#include "Content/OGNativeContentFactoryEditorLibrary.h"
#include "Animation/OGSharedMotionLibrary.h"
#include "Characters/OGCharacterAppearanceDefinition.h"
#include "Animation/Skeleton.h"
#include "Animation/MorphTarget.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#endif

bool UOGNativeContentFactoryEditorLibrary::AuthorNativeAppearance(
    const FString& IdentityId, const FString& VersionId,
    const FString& LongAssetPath, const FString& SharedMotionLibraryPath,
    const TArray<FString>& RealSkeletalMeshPaths, const FString& WardrobeId,
    const FString& RigProfile, const FString& AnatomyProfile,
    const FString& WardrobeStateField, const TArray<FString>& Capabilities)
{
#if WITH_EDITOR
    auto Fail=[](const FString& Reason) {
        UE_LOG(LogTemp,Error,TEXT("NoWorld 5star native authoring blocked: %s"),*Reason);
        return false;
    };
    const FOGContentId Identity(IdentityId), Version(VersionId);
    if(!Identity.IsValid() || !Version.IsValid() ||
       !LongAssetPath.StartsWith(TEXT("/Game/NoWorld/ContentFactory/Appearances/")) ||
       !FPackageName::IsValidLongPackageName(LongAssetPath) ||
       RealSkeletalMeshPaths.IsEmpty() || WardrobeId.IsEmpty() ||
       RigProfile.IsEmpty() || AnatomyProfile.IsEmpty() ||
       WardrobeStateField.IsEmpty() || !SharedMotionLibraryPath.StartsWith(TEXT("/Game/")))
        return Fail(TEXT("Invalid canonical IDs, native source binding or authored output."));
    if(FindObject<UObject>(nullptr,*LongAssetPath) ||
       FPackageName::DoesPackageExist(LongAssetPath))
        return Fail(TEXT("Refusing to replace an existing approved appearance definition."));
    UOGSharedMotionLibrary* Shared=LoadObject<UOGSharedMotionLibrary>(
        nullptr,*SharedMotionLibraryPath);
    if(!Shared || Shared->HasAnyFlags(RF_Transient))
        return Fail(TEXT("Real cook-reachable shared motion DataAsset is missing."));
    FOGCharacterAppearanceWardrobe Outfit;
    Outfit.WardrobeId=FName(*WardrobeId);
    Outfit.SafePrivacyFallback=Outfit.WardrobeId;
    Outfit.RigProfile=FName(*RigProfile);
    Outfit.AnatomyProfile=FName(*AnatomyProfile);
    Outfit.OutfitProfile=Outfit.WardrobeId;
    Outfit.bMatureAppearance=false; // Only this supplied clothed safe source.
    for(const FString& Capability:Capabilities)
    {
        if(Capability.IsEmpty() || Outfit.Capabilities.Contains(FName(*Capability)))
            return Fail(TEXT("Duplicate/empty source capability."));
        Outfit.Capabilities.Add(FName(*Capability));
    }
    TSet<FString> Already;
    for(int32 I=0;I<RealSkeletalMeshPaths.Num();++I)
    {
        const FString& Path=RealSkeletalMeshPaths[I];
        if(!Path.StartsWith(TEXT("/Game/")) || Already.Contains(Path))
            return Fail(TEXT("Duplicate or outside-/Game genuine skeletal source."));
        Already.Add(Path);
        USkeletalMesh* Mesh=LoadObject<USkeletalMesh>(nullptr,*Path);
        if(!Mesh || !Mesh->GetSkeleton() ||
           Mesh->GetRefSkeleton().GetNum()<1)
            return Fail(TEXT("A source part is not an actual native SkeletalMesh."));
        FOGCharacterAppearancePart Part;
        Part.PartId=FName(*FString::Printf(TEXT("source_part_%03d"),I));
        Part.Mesh=Mesh;
        Part.NativeSkeleton=Mesh->GetSkeleton();
        Part.ComponentTransform=FTransform::Identity;
        const FReferenceSkeleton& Bones=Mesh->GetRefSkeleton();
        for(int32 J=0;J<Bones.GetNum();++J)
        {
            Part.BoneNames.Add(Bones.GetBoneName(J));
            Part.BoneParentIndices.Add(Bones.GetParentIndex(J));
        }
        for(const UMorphTarget* Morph:Mesh->GetMorphTargets())
            if(Morph) Part.MorphTargets.Add(Morph->GetFName());
        Outfit.Parts.Add(MoveTemp(Part));
    }
    Outfit.ExpectedPartCount=Outfit.Parts.Num();
    UPackage* Package=CreatePackage(*LongAssetPath);
    if(!Package)return Fail(TEXT("Native appearance package could not be created."));
    const FString Name=FPackageName::GetShortName(LongAssetPath);
    UOGCharacterAppearanceDefinition* Definition=
        NewObject<UOGCharacterAppearanceDefinition>(
            Package,FName(*Name),RF_Public|RF_Standalone);
    if(!Definition)return Fail(TEXT("Could not instantiate source-native appearance UAsset."));
    Definition->IdentityId=Identity;
    Definition->VersionId=Version;
    Definition->WardrobeStateJsonField=FName(*WardrobeStateField);
    Definition->SafeDefaultWardrobe=Outfit.WardrobeId;
    Definition->Wardrobes.Add(MoveTemp(Outfit));
    Definition->SharedMotionLibrary=Shared;
    FString Error;
    if(!Definition->ValidateContract(Error)) return Fail(Error);
    FAssetRegistryModule::AssetCreated(Definition);
    Definition->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags=RF_Public|RF_Standalone;
    Args.SaveFlags=SAVE_None;
    const FString Filename=FPackageName::LongPackageNameToFilename(
        LongAssetPath,FPackageName::GetAssetPackageExtension());
    if(!UPackage::SavePackage(Package,Definition,*Filename,Args))
        return Fail(TEXT("UE editor failed saving actual native UAsset."));
    UE_LOG(LogTemp,Display,
       TEXT("NOWORLD_FACTORY_NATIVE_APPEARANCE_SAVED=%s original_parts=%d"),
       *LongAssetPath,RealSkeletalMeshPaths.Num());
    return true;
#else
    return false;
#endif
}
