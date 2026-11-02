#pragma once

#include "CoreMinimal.h"
#include "Narrative/OGNarrativeService.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OGNarrativeSessionSubsystem.generated.h"

class AOGNarrativeWorldActor;
class UOGGameCoreSubsystem;

struct OFFLINEGAME_API FOGNarrativeJournalEntry
{
    FOGNarrativeQuestDefinition Definition;
    FOGNarrativeQuestState State;
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
    bool CanOpenWorldSubject(const AOGNarrativeWorldActor& Subject, const AActor* Interactor, FString& Error) const;
    bool OpenWorldSubject(AOGNarrativeWorldActor* Subject, AActor* Interactor, FString& Error);
    void CloseSubject();
    bool HasOpenSubject() const { return SubjectActor.IsValid(); }
    bool ReadJournal(TArray<FOGNarrativeJournalEntry>& Out, FString& Error) const;
    bool ReadSubjectDialogues(TArray<FOGNarrativeDialogueDefinition>& Out, FString& Error) const;
    bool ReadConversation(const FOGContentId& DialogueId, FOGNarrativeConversationView& Out, FString& Error) const;
    bool ReviewQuest(const FOGContentId& QuestId, EOGNarrativeCommand Kind,
        FOGNarrativePreview& Out, FString& Error);
    bool ReviewDialogue(const FOGContentId& DialogueId, const FOGContentId& ChoiceId,
        FOGNarrativePreview& Out, FString& Error);
    bool ConfirmReview(const FString& Token, const FOGEntityId& RequestId, FString& Error);
    void ClearReview();
private:
    TWeakObjectPtr<AOGNarrativeWorldActor> SubjectActor;
    TWeakObjectPtr<AActor> SubjectInteractor;
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    FOGNarrativeCommand ReviewedCommand;
    FString ReviewedToken;
    FOGEntityId ReviewedRuler;
    bool bReviewedRequiresSubject = false;
    bool ResolveRuntime(UOGGameCoreSubsystem*& Core, IOGWorldStore*& Store,
        FOGNarrativeCatalog& Catalog, FOGNarrativeContext& Context, bool bUseSubject, FString& Error) const;
    bool PrepareQuestCommand(IOGWorldStore& Store, const FOGNarrativeCatalog& Catalog,
        FOGNarrativeContext Context, const FOGContentId& QuestId, EOGNarrativeCommand Kind,
        FOGNarrativeCommand& Out, FString& Error) const;
    void HandleRuntimeReleasing();
};
