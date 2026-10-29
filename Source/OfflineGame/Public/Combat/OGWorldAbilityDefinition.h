#pragma once
#include "CoreMinimal.h"
#include "Combat/OGNativeCombatAbility.h"
#include "Content/OGDefinitionLibrary.h"
#include "Combat/OGWorldActionRuntime.h"
#include "Engine/EngineTypes.h"
#include "OGWorldAbilityDefinition.generated.h"

UENUM(BlueprintType)
enum class EOGCombatExecutionMode : uint8 { Turn, World };
UENUM(BlueprintType)
enum class EOGWorldAbilityDelivery : uint8 { Direct, Sphere, MeleeSweep, Projectile };
UENUM(BlueprintType)
enum class EOGWorldAbilityCostTiming : uint8 { Unspecified, AcceptedStart, FirstContact };

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGWorldAbilityStep
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 OpensAtMilliseconds=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 ClosesAtMilliseconds=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGWorldAbilityDelivery Delivery=EOGWorldAbilityDelivery::Direct;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeAbilityPacketSlice> Packets;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float RangeCentimeters=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float RadiusCentimeters=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bRequireLineOfSight=true;
    /** Empty selects the character's native mesh; explicit names must resolve uniquely. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName TraceComponentName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName StartSocket;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName EndSocket;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TEnumAsByte<ECollisionChannel> TraceChannel=ECC_Pawn;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TEnumAsByte<ECollisionChannel> VisibilityChannel=ECC_Visibility;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName ProjectileCollisionProfile;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float ProjectileSpeed=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float ProjectileGravityScale=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TSoftObjectPtr<class UStaticMesh> ProjectileMesh;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FTransform ProjectileMeshTransform;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGWorldAbilitySpecification
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId SkillId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId IdentityId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId VersionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName MotionSemantic;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGWorldAbilityCostTiming CostTiming=EOGWorldAbilityCostTiming::Unspecified;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 DurationMilliseconds=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 CooldownMilliseconds=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bRefundCostsOnCancel=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bCancelledCastCountsAsOwnerAction=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGWorldActionState TraversalAndCancel;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGWorldAbilityStep> Steps;
    bool Matches(const FOGCombatUnitState& Unit) const;
    bool Validate(FString& Error) const;
    bool ValidatePackets(const FOGNativeAbilityDefinition& Ability,FString& Error) const;
};
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGWorldAbilityDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGWorldAbilityDefinition() { System=TEXT("world_ability"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGWorldAbilitySpecification Specification;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGWorldAbilityCastView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGEntityId CastId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId SourceUnitId;
    UPROPERTY(BlueprintReadOnly) FOGContentId SkillId;
    UPROPERTY(BlueprintReadOnly) int64 StartedAtMilliseconds=0;
    UPROPERTY(BlueprintReadOnly) int64 EndsAtMilliseconds=0;
    UPROPERTY(BlueprintReadOnly) int32 CompletedSteps=0;
    UPROPERTY(BlueprintReadOnly) bool bCostsPaid=false;
};
