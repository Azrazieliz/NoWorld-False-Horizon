#pragma once
#include "CoreMinimal.h"
#include "Objects/OGObjectActionDefinition.h"
#include "Combat/OGTurnBattle.h"
#include "Persistence/OGWorldStore.h"
#include "Content/OGSystemBindingComponent.h"
#include "OGCombatRewardDefinition.generated.h"

/** Exact source grants, attached by combat_reward_definition_id on the actual enemy entity. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGCombatRewardDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGCombatRewardDefinition() { System=TEXT("combat_reward"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName EnemyEntityKind;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGObjectActionOutput> Objects;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGObjectActionResourceChange> Resources;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
};
/** Optional NPC profile binding. Reads source mechanics without modifying canonical state on actor load. */
UCLASS(ClassGroup=(OfflineGame))
class OFFLINEGAME_API UOGCombatRewardDefinitionReceiver : public UOGDefinitionReceiverComponent
{
    GENERATED_BODY()
public:
    UOGCombatRewardDefinitionReceiver() { AcceptedSystem=TEXT("combat_reward"); ExclusiveChannels.Add(TEXT("combat.victory_rewards")); }
    virtual bool PrepareDefinition_Implementation(UDataAsset* Asset,const FOGDefinitionBindingContext& Context,FString& Error) override;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeBattleOutcome
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGEntityId BattleId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId CompletionEventId;
    UPROPERTY(BlueprintReadOnly) bool bHasPlayerTeam=false;
    UPROPERTY(BlueprintReadOnly) bool bVictory=false;
    UPROPERTY(BlueprintReadOnly) int32 PlayerTeamIndex=INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) TArray<FOGEntityId> RewardEventIds;
    UPROPERTY(BlueprintReadOnly) TArray<FOGEntityId> CreatedItemIds;
    UPROPERTY(BlueprintReadOnly) TArray<FString> RewardLines;
};
/** Called by GameCore within the same write transaction as the actual finishing action. */
class OFFLINEGAME_API FOGCombatRewardService
{
public:
    explicit FOGCombatRewardService(IOGWorldStore& InStore):Store(InStore) {}
    bool ApplyCommittedVictory(const FOGTurnBattleState& State,const FOGEntityId& Ruler,
        const FOGWorldEvent& Completion,FOGNativeBattleOutcome& Out,FString& Error);
private:
    IOGWorldStore& Store;
};
