#include "Narrative/OGNarrativeSessionSubsystem.h"

#include "Combat/OGTurnBattle.h"
#include "Combat/OGWorldActionRuntime.h"
#include "Characters/OGContentCharacterActors.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Narrative/OGNarrativeWorldActor.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "UI/OGGate12OwnedWorldCharacterBinding.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGCanonicalCharacterPresentation.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"

void UOGNarrativeSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UOGGameCoreSubsystem>();
    Collection.InitializeDependency<UOGGate12RulerSessionSubsystem>();
    BoundCore = GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    if (BoundCore.IsValid()) BoundCore->OnCanonicalRuntimeReleasing.AddUObject(this, &ThisClass::HandleRuntimeReleasing);
}

void UOGNarrativeSessionSubsystem::Deinitialize()
{
    if (BoundCore.IsValid()) BoundCore->OnCanonicalRuntimeReleasing.RemoveAll(this);
    HandleRuntimeReleasing();
    Super::Deinitialize();
}

void UOGNarrativeSessionSubsystem::HandleRuntimeReleasing() { CloseSubject(); }
void UOGNarrativeSessionSubsystem::ClearReview()
{
    ReviewedCommand = FOGNarrativeCommand();
    ReviewedToken.Reset();
    ReviewedRuler = FOGEntityId();
    bReviewedRequiresSubject = false;
}
void UOGNarrativeSessionSubsystem::CloseSubject()
{
    ClearReview();
    SubjectActor.Reset();
    SubjectInteractor.Reset();
}

bool UOGNarrativeSessionSubsystem::CanOpenWorldSubject(const AOGNarrativeWorldActor& Subject,
    const AActor* Interactor, FString& Error) const
{
    Error.Reset();
    const APawn* Pawn = Cast<APawn>(Interactor);
    const UWorld* World = Subject.GetWorld();
    if (!IsInGameThread() || !World || !World->IsGameWorld() || !Pawn ||
        Pawn->GetWorld() != World || !Pawn->IsLocallyControlled() ||
        !Cast<APlayerController>(Pawn->GetController()) || Subject.IsActorBeingDestroyed() ||
        !Subject.CanonicalSubjectId.IsValid() || !Subject.CanonicalLocationId.IsValid() ||
        Subject.InteractionPrompt.IsEmpty() || !FMath::IsFinite(Subject.InteractionRangeCm) ||
        Subject.InteractionRangeCm <= 0.f ||
        FVector::DistSquared(Subject.GetActorLocation(), Pawn->GetActorLocation()) > FMath::Square(Subject.InteractionRangeCm))
    { Error = TEXT("An actual local character and nearby authored canonical subject are required."); return false; }
    const UGameInstance* Game = GetGameInstance();
    const UOGGameCoreSubsystem* Core = Game ? Game->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const UOGGate12RulerSessionSubsystem* Ruler = Game ? Game->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() || !Ruler || !Ruler->IsBoundToStoredRuler())
    { Error = TEXT("Canonical narrative context is unavailable."); return false; }
    const FOGTurnBattle* Battle = Core->GetOwnedLuminousTurnBattle();
    if (Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running)
    { Error = TEXT("Resolve the current battle before starting a conversation."); return false; }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(OGNarrativeInteraction), false, Pawn);
    FHitResult Hit;
    if (World->LineTraceSingleByChannel(Hit, Pawn->GetActorLocation(), Subject.GetActorLocation(), ECC_Visibility, Query) &&
        Hit.GetActor() != &Subject)
    { Error = TEXT("The canonical subject is not within clear interaction reach."); return false; }
    const FOGEntityId Observer = Ruler->SelectedManifestationId.IsValid() ? Ruler->SelectedManifestationId : Ruler->BoundRulerId;
    bool bFound = false;
    if (Ruler->SelectedManifestationId.IsValid())
    {
        FOGCharacterManifestationRecord Owned;
        if (!Store->TryReadCharacterManifestation(Observer, bFound, Owned, Error) || !bFound ||
            Owned.OwningRulerId != Ruler->BoundRulerId || Owned.LifecycleState != TEXT("active"))
        { if (Error.IsEmpty()) Error = TEXT("The interacting character is no longer an active owned copy."); return false; }
        const AOGContentPlayableCharacter* Content = Cast<AOGContentPlayableCharacter>(Pawn);
        const AOGLuminousV3PlayableCharacter* Legacy = Cast<AOGLuminousV3PlayableCharacter>(Pawn);
        const UOGWorldPartyRuntimeComponent* Party = Pawn->FindComponentByClass<UOGWorldPartyRuntimeComponent>();
        const bool bPartyMatches = Party && Party->GetSlots().IsValidIndex(Party->GetControlledSlot()) &&
            Party->GetSlots()[Party->GetControlledSlot()].Unit.UnitEntityId == Observer &&
            Party->GetSlots()[Party->GetControlledSlot()].Unit.SkillSet.CurrentForm == Owned.ActiveVersionId &&
            !Party->GetSlots()[Party->GetControlledSlot()].bDefeated;
        if (!(Content && Content->IsBoundToOwnedCombatUnit(Ruler->BoundRulerId, Observer, Owned.ActiveVersionId)) &&
            !(Legacy && Legacy->IsBoundToOwnedLuminousCombatUnit(Ruler->BoundRulerId, Observer, Owned.ActiveVersionId)) && !bPartyMatches)
        { Error = TEXT("The actual controlled World body does not match the selected owned character."); return false; }
    }
    else
    {
        const UOGCanonicalCharacterPresentation* Presentation = Pawn->FindComponentByClass<UOGCanonicalCharacterPresentation>();
        if (!Cast<AOGWorldPrototypeCharacter>(Pawn) ||
            Pawn->GetClass()->ImplementsInterface(UOGGate12OwnedWorldCharacterBinding::StaticClass()) ||
            !Presentation || !Presentation->HasProjection() ||
            Presentation->GetProjection().Context.EntityId != Ruler->BoundRulerId ||
            Presentation->GetProjection().Context.OwnerEntityId != Ruler->BoundRulerId ||
            Presentation->GetProjection().bHasManifestation)
        { Error = TEXT("The actual controlled World body is not bound to the selected Ruler."); return false; }
    }
    FOGWorldPresenceRecord SubjectPresence, ObserverPresence;
    if (!Store->TryReadWorldPresence(Subject.CanonicalSubjectId, bFound, SubjectPresence, Error) || !bFound ||
        SubjectPresence.LocationId != Subject.CanonicalLocationId ||
        !Store->TryReadWorldPresence(Observer, bFound, ObserverPresence, Error) || !bFound ||
        ObserverPresence.LocationId != SubjectPresence.LocationId)
    { if (Error.IsEmpty()) Error = TEXT("Canonical participants do not share the authored location."); return false; }
    return true;
}

bool UOGNarrativeSessionSubsystem::OpenWorldSubject(AOGNarrativeWorldActor* Subject, AActor* Interactor, FString& Error)
{
    if (!Subject || !CanOpenWorldSubject(*Subject, Interactor, Error)) return false;
    CloseSubject();
    SubjectActor = Subject;
    SubjectInteractor = Interactor;
    UOGGate12RulerSessionSubsystem* Ruler = GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>();
    const APawn* Pawn = Cast<APawn>(Interactor);
    APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    AOGWorldPresentationHud* Hud = Controller ? Cast<AOGWorldPresentationHud>(Controller->GetHUD()) : nullptr;
    if (!Hud || !Ruler->Navigate(EOGRulerPrimaryDestination::Records, Error) || !Hud->ShowNativeRulerSurface(Error))
    { CloseSubject(); return false; }
    return true;
}

bool UOGNarrativeSessionSubsystem::ResolveRuntime(UOGGameCoreSubsystem*& Core, IOGWorldStore*& Store,
    FOGNarrativeCatalog& Catalog, FOGNarrativeContext& Context, bool bUseSubject, FString& Error) const
{
    Error.Reset();
    Core = GetGameInstance() ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    Store = Core ? Core->GetWorldStore() : nullptr;
    const UOGGate12RulerSessionSubsystem* Ruler = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() || !Ruler || !Ruler->IsBoundToStoredRuler())
    { Error = TEXT("No canonical Ruler is available for the journal."); return false; }
    Context = FOGNarrativeContext();
    Context.RulerId = Ruler->BoundRulerId;
    Context.InteractorEntityId = Ruler->SelectedManifestationId.IsValid() ? Ruler->SelectedManifestationId : Ruler->BoundRulerId;
    bool bPresenceFound = false;
    FOGWorldPresenceRecord Presence;
    if (!Store->TryReadWorldPresence(Context.InteractorEntityId, bPresenceFound, Presence, Error)) return false;
    if (bPresenceFound) Context.LocationId = Presence.LocationId;
    if (bUseSubject && SubjectActor.IsValid())
    {
        if (!CanOpenWorldSubject(*SubjectActor.Get(), SubjectInteractor.Get(), Error)) return false;
        Context.SubjectId = SubjectActor->CanonicalSubjectId;
        Context.LocationId = SubjectActor->CanonicalLocationId;
    }
    if (!FOGNarrativeCatalog::LoadInstalled(Catalog, Error)) return false;
    return true;
}

bool UOGNarrativeSessionSubsystem::ReadJournal(TArray<FOGNarrativeJournalEntry>& Out, FString& Error) const
{
    Out.Reset();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    FOGNarrativeCatalog Catalog;
    FOGNarrativeContext Context;
    if (!ResolveRuntime(Core, Store, Catalog, Context, false, Error)) return false;
    FOGNarrativeService Service(*Store);
    FOGNarrativeContext AvailableContext = Context;
    FString SubjectError;
    if (SubjectActor.IsValid() && CanOpenWorldSubject(*SubjectActor.Get(), SubjectInteractor.Get(), SubjectError))
    {
        AvailableContext.SubjectId = SubjectActor->CanonicalSubjectId;
        AvailableContext.LocationId = SubjectActor->CanonicalLocationId;
    }
    for (const FOGNarrativeQuestDefinition& Definition : Catalog.Quests)
    {
        FOGNarrativeJournalEntry Entry;
        Entry.Definition = Definition;
        if (!Service.ReadQuest(Context.RulerId, Definition.QuestId, Entry.State, Error)) return false;
        // Unaccepted story text is revealed only when its authored conditions
        // actually permit acceptance in this context. A catalog is not a Codex.
        if (!Entry.State.bExists)
        {
            FOGNarrativeCommand Command;
            Command.Context = AvailableContext;
            Command.Kind = EOGNarrativeCommand::AcceptQuest;
            Command.ContentId = Definition.QuestId;
            FOGNarrativePreview Preview;
            FString AvailabilityError;
            if (!Service.Preview(Catalog, Command, Core->GetCanonicalWorldTick(), Preview, AvailabilityError) || !Preview.bCanApply)
                continue;
        }
        Out.Add(MoveTemp(Entry));
    }
    return true;
}

bool UOGNarrativeSessionSubsystem::ReadSubjectDialogues(TArray<FOGNarrativeDialogueDefinition>& Out, FString& Error) const
{
    Out.Reset();
    if (!SubjectActor.IsValid()) return true;
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    FOGNarrativeCatalog Catalog;
    FOGNarrativeContext Context;
    if (!ResolveRuntime(Core, Store, Catalog, Context, true, Error)) return false;
    for (const FOGNarrativeDialogueDefinition& Definition : Catalog.Dialogues)
    {
        FOGNarrativeCommand Command;
        Command.Context = Context;
        Command.ContentId = Definition.DialogueId;
        Command.Kind = EOGNarrativeCommand::StartDialogue;
        FOGNarrativePreview Preview;
        FOGNarrativeDialogueState State;
        if (!FOGNarrativeService(*Store).ReadDialogue(Context, Definition.DialogueId, State, Error)) return false;
        FString PreviewError;
        if (State.bExists || (FOGNarrativeService(*Store).Preview(Catalog, Command, Core->GetCanonicalWorldTick(), Preview, PreviewError) && Preview.bCanApply))
            Out.Add(Definition);
    }
    return true;
}

bool UOGNarrativeSessionSubsystem::ReadConversation(const FOGContentId& DialogueId,
    FOGNarrativeConversationView& Out, FString& Error) const
{
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    FOGNarrativeCatalog Catalog;
    FOGNarrativeContext Context;
    return ResolveRuntime(Core, Store, Catalog, Context, true, Error) && Context.SubjectId.IsValid() &&
        FOGNarrativeService(*Store).GetConversation(Catalog, Context, DialogueId, Core->GetCanonicalWorldTick(), Out, Error);
}

bool UOGNarrativeSessionSubsystem::PrepareQuestCommand(IOGWorldStore& Store,
    const FOGNarrativeCatalog& Catalog, FOGNarrativeContext Context, const FOGContentId& QuestId,
    EOGNarrativeCommand Kind, FOGNarrativeCommand& Out, FString& Error) const
{
    const FOGNarrativeQuestDefinition* Definition = Catalog.Quests.FindByPredicate(
        [&QuestId](const FOGNarrativeQuestDefinition& Q) { return Q.QuestId == QuestId; });
    if (!Definition || (Kind != EOGNarrativeCommand::AcceptQuest && Kind != EOGNarrativeCommand::AdvanceQuest && Kind != EOGNarrativeCommand::CompleteQuest))
    { Error = TEXT("Select an installed quest command."); return false; }
    FOGNarrativeQuestState State;
    if (!FOGNarrativeService(Store).ReadQuest(Context.RulerId, QuestId, State, Error)) return false;
    if (State.bExists) Context.SubjectId = State.SubjectId;
    if (Kind == EOGNarrativeCommand::CompleteQuest && State.SubjectId.IsValid() && !Definition->bAllowRemoteCompletion)
    {
        if (!SubjectActor.IsValid() || SubjectActor->CanonicalSubjectId != State.SubjectId ||
            !CanOpenWorldSubject(*SubjectActor.Get(), SubjectInteractor.Get(), Error))
        { if (Error.IsEmpty()) Error = TEXT("Return to this quest's subject to complete it."); return false; }
        Context.LocationId = SubjectActor->CanonicalLocationId;
    }
    Out = FOGNarrativeCommand();
    Out.Context = Context;
    Out.Kind = Kind;
    Out.ContentId = QuestId;
    if (Kind == EOGNarrativeCommand::AdvanceQuest)
    {
        TArray<TArray<FOGEntityId>> CandidateEvents;
        const int64 Tick = BoundCore.IsValid() ? BoundCore->GetCanonicalWorldTick() : -1;
        for (const FOGNarrativeObjective& Objective : Definition->Objectives)
        {
            const TArray<FOGEntityId>* Credits = State.CreditedEvents.Find(Objective.ObjectiveId);
            if (Credits && Credits->Num() >= Objective.RequiredCount) continue;
            const TArray<FOGEntityId>* Baseline = State.AcceptanceBaseline.Find(Objective.ObjectiveId);
            const FOGEntityId Subject = Objective.Event.bSubjectIsRuler ? Context.RulerId : Context.SubjectId;
            TArray<FOGEntityId> Eligible;
            FOGWorldEventCursor Cursor;
            const int32 Needed = FMath::Min(256, Objective.RequiredCount - (Credits ? Credits->Num() : 0));
            bool bReachedStart = false;
            while (Eligible.Num() < Needed && !bReachedStart)
            {
                TArray<FOGWorldEvent> Events;
                FOGWorldEventCursor Next;
                if (!Store.ListWorldEventsPage(Subject, Objective.Event.EventType, false, 256, Cursor, Events, Next, Error)) return false;
                for (const FOGWorldEvent& Event : Events)
                {
                    if (Event.WorldTick > Tick) { Error = TEXT("Canonical time precedes the saved quest evidence."); return false; }
                    if (!Objective.bAllowHistoricalEvents && Event.WorldTick < State.StartedWorldTick)
                    { bReachedStart = true; break; }
                    if ((Credits && Credits->Contains(Event.EventId)) ||
                        (!Objective.bAllowHistoricalEvents && Baseline && Baseline->Contains(Event.EventId))) continue;
                    Eligible.Add(Event.EventId);
                    if (Eligible.Num() >= Needed) break;
                }
                if (!Next.EventId.IsValid()) break;
                if (Cursor.EventId.IsValid() && (Next.WorldTick > Cursor.WorldTick ||
                    (Next.WorldTick == Cursor.WorldTick && Next.EventId.ToString() >= Cursor.EventId.ToString())))
                { Error = TEXT("Canonical event history did not advance its page cursor."); return false; }
                Cursor = Next;
            }
            if (!Eligible.IsEmpty()) CandidateEvents.Add(MoveTemp(Eligible));
        }
        // Each unfinished objective receives evidence before another consumes
        // the shared batch. Subsequent reviews skip all already credited IDs.
        for (int32 Round = 0; Round < 256 && Out.EvidenceEventIds.Num() < 256; ++Round)
            for (const TArray<FOGEntityId>& Events : CandidateEvents)
                if (Events.IsValidIndex(Round) && Out.EvidenceEventIds.Num() < 256) Out.EvidenceEventIds.AddUnique(Events[Round]);
        if (Out.EvidenceEventIds.IsEmpty())
        { Error = TEXT("No new canonical progress is available for this quest."); return false; }
    }
    return true;
}

bool UOGNarrativeSessionSubsystem::ReviewQuest(const FOGContentId& QuestId, EOGNarrativeCommand Kind,
    FOGNarrativePreview& Out, FString& Error)
{
    ClearReview();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    FOGNarrativeCatalog Catalog;
    FOGNarrativeContext Context;
    FOGNarrativeCommand Command;
    if (!ResolveRuntime(Core, Store, Catalog, Context, Kind == EOGNarrativeCommand::AcceptQuest, Error) ||
        !PrepareQuestCommand(*Store, Catalog, Context, QuestId, Kind, Command, Error) ||
        !FOGNarrativeService(*Store).Preview(Catalog, Command, Core->GetCanonicalWorldTick(), Out, Error)) return false;
    if (Out.bCanApply)
    {
        const FOGNarrativeQuestDefinition* Definition = Catalog.Quests.FindByPredicate(
            [&QuestId](const FOGNarrativeQuestDefinition& Q) { return Q.QuestId == QuestId; });
        bReviewedRequiresSubject = Command.Context.SubjectId.IsValid() &&
            (Kind == EOGNarrativeCommand::AcceptQuest ||
             (Kind == EOGNarrativeCommand::CompleteQuest && Definition && !Definition->bAllowRemoteCompletion));
        ReviewedCommand = MoveTemp(Command); ReviewedToken = Out.ConfirmationToken; ReviewedRuler = Context.RulerId;
    }
    return true;
}

bool UOGNarrativeSessionSubsystem::ReviewDialogue(const FOGContentId& DialogueId,
    const FOGContentId& ChoiceId, FOGNarrativePreview& Out, FString& Error)
{
    ClearReview();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    FOGNarrativeCatalog Catalog;
    FOGNarrativeContext Context;
    if (!ResolveRuntime(Core, Store, Catalog, Context, true, Error) || !Context.SubjectId.IsValid()) return false;
    FOGNarrativeCommand Command;
    Command.Context = Context;
    Command.ContentId = DialogueId;
    Command.ChoiceId = ChoiceId;
    Command.Kind = ChoiceId.IsValid() ? EOGNarrativeCommand::ChooseDialogue : EOGNarrativeCommand::StartDialogue;
    if (!FOGNarrativeService(*Store).Preview(Catalog, Command, Core->GetCanonicalWorldTick(), Out, Error)) return false;
    if (Out.bCanApply)
    { ReviewedCommand = MoveTemp(Command); ReviewedToken = Out.ConfirmationToken; ReviewedRuler = Context.RulerId; bReviewedRequiresSubject = true; }
    return true;
}

bool UOGNarrativeSessionSubsystem::ConfirmReview(const FString& Token, const FOGEntityId& RequestId, FString& Error)
{
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    FOGNarrativeCatalog Catalog;
    FOGNarrativeContext Context;
    if (Token.IsEmpty() || Token != ReviewedToken || !RequestId.IsValid() ||
        !ResolveRuntime(Core, Store, Catalog, Context, bReviewedRequiresSubject, Error) ||
        Context.RulerId != ReviewedRuler || Context.InteractorEntityId != ReviewedCommand.Context.InteractorEntityId)
    { if (Error.IsEmpty()) Error = TEXT("Narrative context changed; review the choice again."); ClearReview(); return false; }
    const FOGTurnBattle* Battle = Core->GetOwnedLuminousTurnBattle();
    if (Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running)
    { Error = TEXT("Resolve the current battle before changing narrative state."); return false; }
    if (bReviewedRequiresSubject && (!Context.SubjectId.IsValid() || Context.SubjectId != ReviewedCommand.Context.SubjectId))
    { Error = TEXT("Conversation subject changed; review again."); ClearReview(); return false; }
    bool bAlreadyApplied = false;
    if (!FOGNarrativeService(*Store).Apply(Catalog, ReviewedCommand, Token, RequestId,
        Core->GetCanonicalWorldTick(), bAlreadyApplied, Error)) return false;
    ClearReview();
    return true;
}
