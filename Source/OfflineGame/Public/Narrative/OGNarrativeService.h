#pragma once

#include "CoreMinimal.h"
#include "Persistence/OGWorldStore.h"

struct OFFLINEGAME_API FOGNarrativeContext
{
    FOGEntityId RulerId;
    FOGEntityId InteractorEntityId; // optional: defaults to ruler; otherwise active owned manifestation
    FOGEntityId SubjectId;
    FOGEntityId LocationId;
};

/** Exactly one selector: canonical entities.state_json.narrative_actor_id,
 * or character_manifestations.identity_content_id. Never an Actor pointer. */
struct OFFLINEGAME_API FOGNarrativeSubjectRule
{
    FName EntityKind;
    FOGContentId ActorId;
    FOGContentId IdentityId;
    FName LifecycleState; // optional exact manifestation lifecycle state
    TMap<FString, FString> StateEquals; // exact canonical entity JSON strings
};

struct OFFLINEGAME_API FOGNarrativeEventRule
{
    FName EventType;
    bool bSubjectIsRuler = true;
};

struct OFFLINEGAME_API FOGNarrativeBranch
{
    FOGContentId BranchId;
    FOGContentId ValueId;
};

struct OFFLINEGAME_API FOGNarrativeConditions
{
    TArray<FOGContentId> CompletedQuests;
    TArray<FOGNarrativeEventRule> RequiredEvents;
    TArray<FOGNarrativeEventRule> ExcludedEvents;
    TArray<FOGNarrativeBranch> RequiredBranches;
};

struct OFFLINEGAME_API FOGNarrativeResource
{
    FOGContentId ResourceId;
    FString DisplayName;
    int64 Amount = 0;
};

struct OFFLINEGAME_API FOGNarrativeEffects
{
    TArray<FOGNarrativeResource> Costs;
    TArray<FOGNarrativeResource> Rewards;
    TArray<FOGNarrativeBranch> BranchLocks; // immutable, shared per ruler
};

struct OFFLINEGAME_API FOGNarrativeObjective
{
    FOGContentId ObjectiveId;
    FString DisplayName;
    FOGNarrativeEventRule Event;
    int32 RequiredCount = 1;
    bool bAllowHistoricalEvents = false;
};

struct OFFLINEGAME_API FOGNarrativeQuestDefinition
{
    FOGContentId QuestId;
    FString DisplayName;
    FString Description;
    FOGNarrativeSubjectRule Subject; // empty permits ruler-only quest commands
    bool bAllowRemoteCompletion = false; // acceptance remains local when subject-bound
    FOGNarrativeConditions Conditions;
    FOGNarrativeEffects AcceptEffects;
    FOGNarrativeEffects CompletionEffects;
    TArray<FOGNarrativeObjective> Objectives;
};

struct OFFLINEGAME_API FOGNarrativeChoiceDefinition
{
    FOGContentId ChoiceId;
    FString Text;
    FOGContentId NextNodeId;
    FOGNarrativeConditions Conditions;
    FOGNarrativeEffects Effects;
};

struct OFFLINEGAME_API FOGNarrativeNodeDefinition
{
    FOGContentId NodeId;
    FString Text;
    bool bTerminal = false;
    TArray<FOGNarrativeChoiceDefinition> Choices;
};

struct OFFLINEGAME_API FOGNarrativeDialogueDefinition
{
    FOGContentId DialogueId;
    FString DisplayName;
    FOGNarrativeSubjectRule Subject; // mandatory authored canonical selector
    FOGNarrativeConditions Conditions;
    FOGContentId EntryNodeId;
    TArray<FOGNarrativeNodeDefinition> Nodes;
};

struct OFFLINEGAME_API FOGNarrativeCatalog
{
    TArray<FOGNarrativeQuestDefinition> Quests;
    TArray<FOGNarrativeDialogueDefinition> Dialogues;

    static bool LoadInstalled(FOGNarrativeCatalog& Out, FString& Error);
    static bool Parse(const FString& Json, FOGNarrativeCatalog& Out, FString& Error);
    static bool Validate(const FOGNarrativeCatalog& Catalog, FString& Error);
};

enum class EOGNarrativeCommand : uint8
{
    AcceptQuest, AdvanceQuest, CompleteQuest, StartDialogue, ChooseDialogue
};

struct OFFLINEGAME_API FOGNarrativeCommand
{
    FOGNarrativeContext Context;
    EOGNarrativeCommand Kind = EOGNarrativeCommand::AcceptQuest;
    FOGContentId ContentId;
    FOGContentId ChoiceId;
    /** AdvanceQuest only: up to 256 IDs of events already in WorldStore.
     * Evidence is checked against persisted acceptance time and bound subject. */
    TArray<FOGEntityId> EvidenceEventIds;
};

struct OFFLINEGAME_API FOGNarrativeQuestState
{
    bool bExists = false;
    bool bCompleted = false;
    FOGEntityId SubjectId;
    int64 StartedWorldTick = 0;
    int64 UpdatedWorldTick = 0;
    TMap<FOGContentId, TArray<FOGEntityId>> CreditedEvents;
    /** Canonical events already present at acceptance's exact tick. These do
     * not count as new evidence; later appends at that same tick can count. */
    TMap<FOGContentId, TArray<FOGEntityId>> AcceptanceBaseline;
};

struct OFFLINEGAME_API FOGNarrativeDialogueState
{
    bool bExists = false;
    bool bCompleted = false;
    FOGContentId NodeId;
    int64 UpdatedWorldTick = 0;
};

struct OFFLINEGAME_API FOGNarrativeResourcePreview
{
    FOGContentId ResourceId;
    FString DisplayName;
    int64 BalanceBefore = 0;
    int64 Cost = 0;
    int64 Reward = 0;
    int64 BalanceAfter = 0;
};

struct OFFLINEGAME_API FOGNarrativePreview
{
    bool bCanApply = false;
    FString DisplayName;
    FString ConfirmationToken;
    TArray<FString> MissingRequirements;
    FOGNarrativeQuestState Quest; // proposed state
    FOGNarrativeDialogueState Dialogue; // proposed state
    TArray<FOGNarrativeResourcePreview> Resources;
};

struct OFFLINEGAME_API FOGNarrativeChoiceView
{
    FOGContentId ChoiceId;
    FString Text;
    bool bEligible = false;
    FString ConfirmationToken;
    TArray<FString> MissingRequirements;
};

struct OFFLINEGAME_API FOGNarrativeConversationView
{
    FString DisplayName;
    FString Text;
    FOGNarrativeDialogueState State;
    TArray<FOGNarrativeChoiceView> Choices;
};

/** No Actors, UI, migrations or secondary database. Commands atomically update
 * existing projects/resources and append request-ID receipts to world_events.
 * Read-only previews are re-evaluated under the write transaction. */
class OFFLINEGAME_API FOGNarrativeService
{
public:
    explicit FOGNarrativeService(IOGWorldStore& InStore) : Store(InStore) {}

    bool Preview(const FOGNarrativeCatalog& Catalog, const FOGNarrativeCommand& Command,
        int64 WorldTick, FOGNarrativePreview& Out, FString& Error) const;
    bool Apply(const FOGNarrativeCatalog& Catalog, const FOGNarrativeCommand& Command,
        const FString& ConfirmationToken, const FOGEntityId& RequestId, int64 WorldTick,
        bool& bAlreadyApplied, FString& Error);
    bool ReadQuest(const FOGEntityId& RulerId, const FOGContentId& QuestId,
        FOGNarrativeQuestState& Out, FString& Error) const;
    bool ReadDialogue(const FOGNarrativeContext& Context, const FOGContentId& DialogueId,
        FOGNarrativeDialogueState& Out, FString& Error) const;
    bool GetConversation(const FOGNarrativeCatalog& Catalog, const FOGNarrativeContext& Context,
        const FOGContentId& DialogueId, int64 WorldTick,
        FOGNarrativeConversationView& Out, FString& Error) const;

private:
    IOGWorldStore& Store;
};
