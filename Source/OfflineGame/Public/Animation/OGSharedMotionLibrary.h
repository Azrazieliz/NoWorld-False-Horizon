#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OGSharedMotionLibrary.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;

/** Canonical semantic source identity. Retargeted clips share a SourceClipId. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGSharedMotionEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Identity")
    FName MotionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Identity")
    FName SourceClipId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Identity")
    FString SourceObjectHash;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Identity")
    FString SourceUnityBundle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Semantic")
    FName GameplayAction = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Semantic")
    FName MotionCategory = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Semantic")
    FName GameplayContext = NAME_None;

    // These identify rigs / anatomy, never character Versions or owned Manifestations.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Rig")
    FName CompatibleRigProfile = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Rig")
    FName AnatomyProfile = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Rig")
    FName RetargetProfile = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Rig")
    FName OutfitProfile = NAME_None; // None = applicable across wardrobes if skeleton matches.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Rig")
    TArray<FName> RequiredCapabilities;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Native")
    TSoftObjectPtr<UAnimSequence> NativeSequence;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Native")
    float MovementSpeedCmPerSecond = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Native")
    float ContactSeconds = -1.f; // -1 means no authored contact marker.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Native")
    FName ContactNotify = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Native")
    bool bUsesRootMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Native")
    bool bLoop = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Eligibility")
    bool bApprovedForGameplay = false; // Rejected/QA clips cannot resolve into gameplay.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Eligibility")
    bool bAdultRestricted = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Eligibility", meta=(ClampMin="1", ClampMax="13"))
    int32 RequiredParticipants = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion|Eligibility")
    int32 Priority = 0;
};

/** Shared native clip definitions. One semantic action, many exact-skeleton derivatives. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGSharedMotionLibrary : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Motion")
    TArray<FOGSharedMotionEntry> Motions;
};

/** Pure presentation resolution: does not authorize gameplay or change actor state. */
class OFFLINEGAME_API FOGSharedMotionResolver
{
public:
    /** Ready-to-use native clips already present in this production checkout. */
    static const UOGSharedMotionLibrary* GetInstalledLibrary();

    static UAnimSequence* Resolve(
        const UOGSharedMotionLibrary* Library,
        FName Action,
        FName RigProfile,
        FName AnatomyProfile,
        FName OutfitProfile,
        FName Context,
        const TArray<FName>& Capabilities,
        const TArray<TObjectPtr<USkeletalMeshComponent>>& ActiveParts,
        bool bAdultContextAuthorized,
        int32 ParticipantCount,
        const FOGSharedMotionEntry** OutEntry = nullptr);
};
