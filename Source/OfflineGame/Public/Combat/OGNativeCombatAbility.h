#pragma once
#include "CoreMinimal.h"
#include "Combat/OGCombatMath.h"
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
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeDefenseReference DefenseReference=EOGNativeDefenseReference::Unspecified;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGBaseDamageType DamageType=EOGBaseDamageType::Physical;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Repeats=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bCanCrit=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bCanBeBlocked=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bAllowHitOverflow=false;
    bool operator==(const FOGNativeAbilityPacket& V) const;
};
/** Complete ordinary damage/restoration definition, bound to an exact real kit.
 * Special mechanics retain their own native executors, including every Luminous form. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeAbilityDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId IdentityId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId VersionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId SkillId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    /** Existing global semantic motion key; independent of actor/wardrobe clip assets. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName MotionSemantic;
    /** Must be declared by the complete source definition; paid abilities need their resource executor. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bNoResourceCost=false;
    /** Number of intervening own actions; no arbitrary timer or global team cooldown. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 CooldownOwnerActions=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeAbilityTargets Targets=EOGNativeAbilityTargets::SingleEnemy;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bIncludeSelf=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeAbilityPacket> Packets;
    bool Validate(FString& Error) const;
    bool operator==(const FOGNativeAbilityDefinition& V) const;
};

UCLASS(BlueprintType)
class OFFLINEGAME_API UOGNativeCombatAbility : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGNativeAbilityDefinition Definition;
    bool ValidateInstalled(FString& Error) const;
};
