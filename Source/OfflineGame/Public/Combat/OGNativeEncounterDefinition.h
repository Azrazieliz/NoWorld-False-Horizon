#pragma once
#include "CoreMinimal.h"
#include "Combat/OGTurnBattle.h"
#include "Content/OGSystemBindingComponent.h"
#include "OGNativeEncounterDefinition.generated.h"

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGEncounterRankBand
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Channel;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MinimumRankGap=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumRankGap=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MinimumSourceLevel=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumSourceLevel=MAX_int32;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MinimumTargetLevel=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumTargetLevel=MAX_int32;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MultiplierBps=10000;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGEncounterRankRules
{
    GENERATED_BODY()
    /** Exact source order; never infer ordering from names or lexical comparison. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGContentId> OrderedRanks;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGEncounterRankBand> Bands;
    bool Validate(FString& Error) const;
    bool Resolve(const FOGRankSuppressionQuery& Query,int32& Multiplier,FString& Error) const;
};
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGEncounterRankDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGEncounterRankDefinition() { System=TEXT("rank_suppression"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEncounterRankRules Rules;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGEncounterRolePlan
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Role;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 TeamIndex=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MinimumParticipants=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumParticipants=-1;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGResolvedEncounterParticipant
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Role;
    /** Already resolved by the canonical character/stat systems. No new entity is created. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FOGCombatUnitState Unit;
    /** Turn formation: lane=index%6, succession depth=index/6. Independent of character identity. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 FormationIndex=0;
    /** Fresh encounter timeline; retained party snapshots cannot carry an old battle's turn position. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int64 OpeningActionValue=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bWorldActive=true;
};
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGNativeEncounterDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGNativeEncounterDefinition() { System=TEXT("encounter"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGCombatExecutionMode Mode=EOGCombatExecutionMode::Turn;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGEncounterRolePlan> Roles;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGDefinitionReference RankRules;
    /** Canonical NPC fields; an installed AI policy supplies its own exact field contract when present. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName CanonicalIdentityField=FName(TEXT("identity_id"));
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName CanonicalVersionField=FName(TEXT("version_id"));
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
    bool Assemble(const TArray<FOGResolvedEncounterParticipant>& Participants,const FOGEntityId& BattleId,
        FOGTurnBattleState& Out,FString& Error) const;
};
/** A reusable World trigger/actor receiver. The caller supplies existing resolved participants. */
UCLASS(ClassGroup=(OfflineGame),meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGNativeEncounterReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGNativeEncounterReceiver() { AcceptedSystem=TEXT("encounter"); }
    UPROPERTY(BlueprintReadOnly) FString Status;
    UPROPERTY(BlueprintReadOnly) FOGEntityId LastStartedBattle;
    virtual bool PrepareDefinition_Implementation(UDataAsset*,const FOGDefinitionBindingContext&,FString&) override;
    virtual void ActivateDefinition_Implementation() override { SetActive(true); }
    virtual void ReleaseDefinition_Implementation() override { SetActive(false); }
    UFUNCTION(BlueprintCallable,Category="Encounter")
    bool StartEncounter(const TArray<FOGResolvedEncounterParticipant>& Participants,FString& Error);
};
