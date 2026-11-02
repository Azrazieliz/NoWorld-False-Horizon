#pragma once
#include "CoreMinimal.h"
#include "Combat/OGCombatMath.h"
#include "Effects/OGNativeEffectTypes.h"
#include "Engine/DataAsset.h"
#include "OGNativeCombatAbility.generated.h"

UENUM(BlueprintType)
enum class EOGNativeAbilityTargets : uint8 { SingleEnemy, SingleAlly, Self, AllEnemies, AllAllies };
UENUM(BlueprintType)
enum class EOGNativeAbilityEffect : uint8 { Damage, Healing };
UENUM(BlueprintType)
enum class EOGNativeScaleStat : uint8 { Attack, MaxHp, Defense };
UENUM(BlueprintType)
enum class EOGNativeDefenseReference : uint8 { Unspecified, Attack, MaxHp, Defense };

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeAbilityScale
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeScaleStat Stat=EOGNativeScaleStat::Attack;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MultiplierBps=0;
    bool operator==(const FOGNativeAbilityScale& V) const { return Stat==V.Stat && MultiplierBps==V.MultiplierBps; }
};
/** An authored ordinary packet; no guessed scale/reference, status or resurrection. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeAbilityPacket
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeAbilityEffect Effect=EOGNativeAbilityEffect::Damage;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeAbilityScale> Scaling;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGLargeNumber FlatAmount;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeDefenseReference DefenseReference=EOGNativeDefenseReference::Unspecified;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGBaseDamageType DamageType=EOGBaseDamageType::Physical;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Repeats=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bCanCrit=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bCanBeBlocked=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bAllowHitOverflow=false;
    bool operator==(const FOGNativeAbilityPacket& V) const;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeAbilityResourceAmount
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ResourceId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 Amount=0;
    bool operator==(const FOGNativeAbilityResourceAmount& V) const { return ResourceId==V.ResourceId && Amount==V.Amount; }
};
/** Exact repeat interval from one shared packet; staging never copies or rescales skill numbers. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeAbilityPacketSlice
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 PacketIndex=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 FirstRepeat=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 RepeatCount=1;
};
/** Shared skill mechanics. Empty scope accepts any real kit owning this skill ID.
 * Special mechanics retain their own native executors, including every Luminous form. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeAbilityDefinition
{
    GENERATED_BODY()
    /** Optional source restrictions; a shared skill need not be duplicated per character. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId IdentityId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId VersionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId SkillId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    /** Existing global semantic motion key; independent of actor/wardrobe clip assets. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName MotionSemantic;
    /** Explicit no-cost contract, or exact typed costs on this source unit's battle resources. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bNoResourceCost=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeAbilityResourceAmount> ResourceCosts;
    /** Applied once after successful resolution, clamped to the source's admitted resource capacity. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeAbilityResourceAmount> ResourceChanges;
    /** Number of intervening own actions; no arbitrary timer or global team cooldown. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 CooldownOwnerActions=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeAbilityTargets Targets=EOGNativeAbilityTargets::SingleEnemy;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bIncludeSelf=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeAbilityPacket> Packets;
    /** Ordered effects share the global library; no per-character effect assets. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeAbilityEffectOperation> Effects;
    bool MatchesSource(const FOGContentId& Identity,const FOGContentId& Version) const;
    bool Validate(FString& Error) const;
    bool operator==(const FOGNativeAbilityDefinition& V) const;
};

UCLASS(BlueprintType)
class OFFLINEGAME_API UOGNativeCombatAbility : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGNativeAbilityDefinition Definition;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(MultiLine=true)) FText Description;
    bool ValidateInstalled(FString& Error) const;
};
