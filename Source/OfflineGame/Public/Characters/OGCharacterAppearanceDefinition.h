#pragma once

#include "CoreMinimal.h"
#include "Content/OGContentId.h"
#include "Engine/DataAsset.h"
#include "OGCharacterAppearanceDefinition.generated.h"

class USkeletalMesh;
class USkeleton;
class UOGSharedMotionLibrary;

/** Exact imported native contract, independently authored for each wardrobe part. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGCharacterAppearancePart
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FName PartId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    TSoftObjectPtr<USkeletalMesh> Mesh;

    /** Exact native Skeleton object, not a compatible-skeleton alias. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    TSoftObjectPtr<USkeleton> NativeSkeleton;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Rig")
    TArray<FName> BoneNames;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Rig")
    TArray<int32> BoneParentIndices;

    /** Empty = no authored rest-pose assertion; otherwise one local transform per bone. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Rig")
    TArray<FTransform> ReferenceLocalTransforms;

    /** Complete native morph inventory: exact count and existence are checked. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Morphs")
    TArray<FName> MorphTargets;

    /** Relative to the component's authored attachment root. No implicit yaw/offset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FTransform ComponentTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGCharacterExpressionMorph
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Expression")
    FName PartId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Expression")
    FName Morph = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Expression", meta=(ClampMin="-1", ClampMax="1"))
    float Scale = 1.f;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGCharacterSemanticExpression
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Expression")
    FName Semantic = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Expression")
    TArray<FOGCharacterExpressionMorph> Morphs;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGCharacterAppearanceWardrobe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FName WardrobeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance", meta=(ClampMin="1"))
    int32 ExpectedPartCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    TArray<FOGCharacterAppearancePart> Parts;

    /** Presentation classification only; never defines canonical maturity/ownership. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Privacy")
    bool bMatureAppearance = false;

    /** Required safe row in this same Wardrobes array; safe rows may point to themselves. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Privacy")
    FName SafePrivacyFallback = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Expression")
    TArray<FOGCharacterSemanticExpression> Expressions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Motion")
    FName RigProfile = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Motion")
    FName AnatomyProfile = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Motion")
    FName OutfitProfile = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Motion")
    TArray<FName> Capabilities;

    /** Opt-in only; actual complete topology/rest poses must also match across parts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Motion")
    bool bUseIdenticalTopologyLeaderPose = false;
};

/** One authored Identity/Version. Reflected typed soft references participate in cooking. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGCharacterAppearanceDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FOGContentId IdentityId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FOGContentId VersionId;

    /** Must match the existing UI catalog's authored OutfitStateJson key. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FName WardrobeStateJsonField = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FName SafeDefaultWardrobe = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    TArray<FOGCharacterAppearanceWardrobe> Wardrobes;

    /** Required exact shared library, never a guessed per-character animation catalog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance|Motion")
    TSoftObjectPtr<UOGSharedMotionLibrary> SharedMotionLibrary;

    /** Data-only checks. Does not load meshes, mutate assets or touch canonical state. */
    bool ValidateContract(FString& OutError) const;
    const FOGCharacterAppearanceWardrobe* FindWardrobe(FName Id) const;
    const FOGCharacterAppearanceWardrobe* ResolveRenderedWardrobe(
        FName Requested, bool bSfw, FString& OutError) const;
};
