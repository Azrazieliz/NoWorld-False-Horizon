#pragma once
#include "CoreMinimal.h"
#include "Core/OGEntityId.h"
#include "Math/OGLargeNumber.h"
#include "Effects/OGEffectDefinition.h"
#include "OGNativeEffectTypes.generated.h"

UENUM(BlueprintType)
enum class EOGNativeEffectMechanic : uint8 { StatModifier, Barrier, PeriodicDamage, PeriodicHealing };
UENUM(BlueprintType)
enum class EOGNativeEffectStacking : uint8 { Refresh, AddAndRefresh, Independent };
UENUM(BlueprintType)
enum class EOGNativeEffectClock : uint8 { SourceActions, TargetActions };
UENUM(BlueprintType)
enum class EOGNativeEffectStat : uint8
{
    Attack, Defense, Speed, CritRate, CritDamage, CritRateResistance, CritDamageResistance,
    Hit, Dodge, BlockRate, BlockReduction, DefensePenetration, DamageDealt, DamageTaken, TrueDamageResistance
};
UENUM(BlueprintType)
enum class EOGNativeEffectOperation : uint8 { Apply, DispelEffect, DispelFamily };
UENUM(BlueprintType)
enum class EOGNativeDispelSelection : uint8 { Any, Beneficial, Harmful };

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeEffectModifier
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeEffectStat Stat=EOGNativeEffectStat::Attack;
    /** Flat points (basis points for percentage stats), then a multiplier relative to the unmodified stat. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Flat=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 AdditiveMultiplierBps=0;
    bool operator==(const FOGNativeEffectModifier& V) const
    { return Stat==V.Stat && Flat==V.Flat && AdditiveMultiplierBps==V.AdditiveMultiplierBps; }
};

/** Executable mechanics shared by every source kit. No implicit elemental or character rules. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeEffectDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEffectDefinition Effect;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeEffectMechanic Mechanic=EOGNativeEffectMechanic::StatModifier;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeEffectStacking Stacking=EOGNativeEffectStacking::Refresh;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeEffectClock Clock=EOGNativeEffectClock::TargetActions;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bPerSource=true;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bBeneficial=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bDispellable=true;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bRemoveOnDefeat=true;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bRefillBarrierOnRefresh=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGNativeEffectModifier> Modifiers;
    /** Captured at application from effective source stats; periodic packets never reroll scaling. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGLargeNumber FlatMagnitude;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 AttackScalingBps=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 DefenseScalingBps=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaxHpScalingBps=0;
    /** Periodic damage is an explicitly authored HP-loss packet, rank-scaled, then barriers. */
    bool Validate(FString& Error) const;
    bool operator==(const FOGNativeEffectDefinition& V) const;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeAbilityEffectOperation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeEffectOperation Operation=EOGNativeEffectOperation::Apply;
    /** Effect ID for Apply/DispelEffect; family ID for DispelFamily. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId Id;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bToSource=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bBeforePackets=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Stacks=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 ApplicationChanceBps=10000;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumRemovals=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bOnlyOwnSource=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGNativeDispelSelection DispelSelection=EOGNativeDispelSelection::Any;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGRulePriority RemovalPriority;
    bool Validate(FString& Error) const;
    bool operator==(const FOGNativeAbilityEffectOperation& V) const;
};

/** Concrete source-owned state; IDs and clocks survive atomic candidate copies and durable receipts. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeEffectInstance
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGEntityId InstanceId;
    UPROPERTY(BlueprintReadOnly) FOGContentId EffectId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId SourceUnitId;
    UPROPERTY(BlueprintReadOnly) FOGContentId SourceSkillId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId ClockUnitId;
    UPROPERTY(BlueprintReadOnly) int32 Stacks=1;
    UPROPERTY(BlueprintReadOnly) int64 RemainingActions=-1;
    UPROPERTY(BlueprintReadOnly) int64 AppliedAtOwnerAction=0;
    UPROPERTY(BlueprintReadOnly) FOGLargeNumber MagnitudePerStack;
    UPROPERTY(BlueprintReadOnly) FOGLargeNumber BarrierRemaining;
    bool operator==(const FOGNativeEffectInstance& V) const;
};
