#pragma once

#include "CoreMinimal.h"
#include "Narrative/OGNarrativeService.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/OGUiViewModels.h"
#include "OGNarrativeSessionSubsystem.generated.h"

class AActor;
class UOGGameCoreSubsystem;

struct OFFLINEGAME_API FOGNarrativeJournalEntry
{
    FOGNarrativeQuestDefinition Definition;
    FOGNarrativeQuestState State;
    bool bExpired = false;
};

struct OFFLINEGAME_API FOGNarrativeMemoryEntry
{
    int64 WorldTick = 0;
    FString Summary;
};

struct OFFLINEGAME_API FOGNarrativeCodexEntry
{
    FOGCodexEntryViewModel Entry;
    FString Summary;
};

/** Native journal/conversation command boundary. Stores only transient selection/review.
 * All quest, choice, branch and resource truth remains in the existing WorldStore. */
UCLASS()
class OFFLINEGAME_API UOGNarrativeSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool CanOpenWorldSubject(const AActor& Subject, const AActor* Interactor, FString& Error) const;
    bool OpenWorldSubject(AActor* Subject, AActor* Interactor, FString& Error);
    void CloseSubject();
    bool HasOpenSubject() const { return SubjectActor.IsValid(); }
    bool ReadJournal(TArray<FOGNarrativeJournalEntry>& Out, FString& Error) const;
    bool ReadRecentMemories(TArray<FOGNarrativeMemoryEntry>& Out, FString& Error) const;
    bool ReadMemoryPage(const FOGWorldEventCursor& Before, TArray<FOGNarrativeMemoryEntry>& Out,
        FOGWorldEventCursor& Next, FString& Error) const;
    bool ReadCodexDiscoveries(const FString& SearchText, FName CategoryFilter,
        TArray<FOGNarrativeCodexEntry>& Out, FString& Error) const;
    bool ReadSubjectDialogues(TArray<FOGNarrativeDialogueDefinition>& Out, FString& Error) const;
    bool ReadConversation(const FOGContentId& DialogueId, FOGNarrativeConversationView& Out, FString& Error) const;
    bool ReviewQuest(const FOGContentId& QuestId, EOGNarrativeCommand Kind,
        FOGNarrativePreview& Out, FString& Error);
    bool ReviewDialogue(const FOGContentId& DialogueId, const FOGContentId& ChoiceId,
        FOGNarrativePreview& Out, FString& Error);
    bool ConfirmReview(const FString& Token, const FOGEntityId& RequestId, FString& Error);
    void ClearReview();
private:
    TWeakObjectPtr<AActor> SubjectActor;
    TWeakObjectPtr<AActor> SubjectInteractor;
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    FOGNarrativeCommand ReviewedCommand;
    FString ReviewedToken;
    FOGEntityId ReviewedRuler;
    bool bReviewedRequiresSubject = false;
    bool ResolveWorldSubject(const AActor& Subject, const AActor* Interactor,
        FOGNarrativeContext& Out, FString& Error) const;
    bool ResolveRuntime(UOGGameCoreSubsystem*& Core, IOGWorldStore*& Store,
        FOGNarrativeCatalog& Catalog, FOGNarrativeContext& Context, bool bUseSubject, FString& Error) const;
    bool PrepareQuestCommand(IOGWorldStore& Store, const FOGNarrativeCatalog& Catalog,
        FOGNarrativeContext Context, const FOGContentId& QuestId, EOGNarrativeCommand Kind,
        FOGNarrativeCommand& Out, FString& Error) const;
    void HandleRuntimeReleasing();
};
