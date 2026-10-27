#include "Interaction/OGFoundationInteractionHost.h"
#include "Interaction/OGInteractionParticipantComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Narrative/OGNarrativeWorldActor.h"
#include "Runtime/OGCanonicalCharacterPresentation.h"
#include "EngineUtils.h"
#include "Combat/OGDiagnosticCombatComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGDiagnosticHumanoid.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "World/OGStartingRegionPresentation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/OGSharedMotionLibrary.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Animation/OGSharedMotionSceneComponent.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Misc/CoreDelegates.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace
{
    const FName InteractionClip(TEXT("FoundationInteraction.Optional"));
    bool IsEntry(FName Id)
    {
        return Id == TEXT("world") || Id == TEXT("ruler") || Id == TEXT("roster") ||
            Id == TEXT("npc") || Id == TEXT("event") || Id == TEXT("post_combat");
    }
}

UOGFoundationInteractionHost::UOGFoundationInteractionHost()
{
    PrimaryComponentTick.bCanEverTick = true;
}
UOGFoundationInteractionHost::~UOGFoundationInteractionHost() = default;

bool UOGFoundationInteractionHost::Initialize()
{
    if (!GetOwner() || !GetWorld() || !GetWorld()->GetGameInstance()) return false;
    UOGGameCoreSubsystem* Current = GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    if (Core.Get() != Current)
    {
        Cancel(TEXT("core_rebind")); Runtime.Reset(); BoundStore = nullptr;
        if (Core.IsValid()) Core->OnCanonicalRuntimeReleasing.Remove(ReleaseHandle);
        ReleaseHandle.Reset(); Core = Current;
        if (Current) ReleaseHandle = Current->OnCanonicalRuntimeReleasing.AddUObject(
            this, &UOGFoundationInteractionHost::HandleCanonicalRuntimeReleasing);
    }
    Combat = GetOwner()->FindComponentByClass<UOGDiagnosticCombatComponent>();
    if (!DeactivateHandle.IsValid()) DeactivateHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(
        this, &UOGFoundationInteractionHost::HandleApplicationDeactivated);
    if (!BackgroundHandle.IsValid()) BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(
        this, &UOGFoundationInteractionHost::HandleApplicationDeactivated);
    if (!EnsureRuntime()) return false;
    if (!IsActive()) RefreshProductionSelection();
    return true;
}

bool UOGFoundationInteractionHost::EnsureRuntime()
{
    const UOGGate12RulerSessionSubsystem* Session =
        GetWorld() && GetWorld()->GetGameInstance()
            ? GetWorld()->GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>()
            : nullptr;
    if (!Core.IsValid() || !Core->IsCoreReady() || !Core->GetWorldStore() ||
        Core->GetCanonicalWorldTick() < 0 || !Session ||
        !Session->IsBoundToStoredRuler() || !Session->BoundRulerId.IsValid())
    { Status = TEXT("Canonical world, clock and persisted Ruler are not ready."); return false; }
    if (Runtime && (BoundStore != Core->GetWorldStore() || BoundRulerId != Session->BoundRulerId))
    { Cancel(TEXT("canonical_binding_changed")); Runtime.Reset(); SelectedParticipants.Reset(); }
    if (!Runtime)
    {
        BoundRulerId = Session->BoundRulerId;
        BoundStore = Core->GetWorldStore();
        Runtime = MakeUnique<FOGSystemicInteractionRuntime>(
            *BoundStore, Session->BoundRulerId, Core->GetOptionalPackageHost());
        Runtime->SetPresentationBridge(this);
    }
    // The release delegate clears the runtime before a Store can be destroyed.
    if (BoundStore != Core->GetWorldStore())
    { Status = TEXT("Canonical runtime changed; reinitialize the interaction host."); return false; }
    if (!Runtime->IsActive())
    {
        FString HostError;
        if (!Runtime->SetOptionalPackageHost(Core->GetOptionalPackageHost(), HostError))
        { Status = HostError; return false; }
    }
    LastTick = Core->GetCanonicalWorldTick();
    return true;
}

bool UOGFoundationInteractionHost::IsActive() const
{ return Runtime && Runtime->IsActive(); }

FString UOGFoundationInteractionHost::GetStatus() const
{
    if (!IsActive()) return Status;
    return FString::Printf(TEXT("%s | %s | %d participants | %.2fx | tick %lld"), *Status,
        Runtime->IsAwaitingAction() ? TEXT("Choose / ") : *Runtime->GetCurrentNode().ToString(),
        Runtime->GetParticipants().Num(), Runtime->GetPacing(), LastTick);
}

void UOGFoundationInteractionHost::HandleApplicationDeactivated()
{ Cancel(TEXT("application_deactivated")); }

void UOGFoundationInteractionHost::HandleCanonicalRuntimeReleasing()
{
    Cancel(TEXT("canonical_world_releasing"));
    Runtime.Reset(); BoundStore = nullptr; SelectedParticipants.Reset();
    Status = TEXT("Canonical world released; interaction restored ordinary gameplay.");
}

void UOGFoundationInteractionHost::SetSelectedParticipants(const TArray<FOGInteractionParticipant>& Selection)
{
    if (IsActive()) { Status = TEXT("Exit the current interaction before changing participant bindings."); return; }
    SelectedParticipants = Selection;
    Status = FString::Printf(TEXT("%d selected participant bindings."), SelectedParticipants.Num());
}

AActor* UOGFoundationInteractionHost::FindBoundActor(const FOGEntityId& Entity) const
{
    const TWeakObjectPtr<AActor>* Actor = BoundActors.Find(Entity);
    return Actor ? Actor->Get() : nullptr;
}

void UOGFoundationInteractionHost::BuildDiagnosticSelection(bool bAll)
{
    if (!Combat.IsValid() || !Combat->IsActive() || !BoundStore) return;
    TArray<FOGInteractionParticipant> Candidates;
    FOGEntityId Controlled; FOGContentId Identity; FOGCharacterManifestationRecord Copy;
    bool bCopy = false; FString Error;
    Combat->TryGetControlledContext(Controlled, Identity, bCopy, Copy, Error);
    for (const FOGCombatUnitState& Unit : Combat->GetWorldParticipantsForInteraction())
    {
        if (Unit.UnitEntityId == Combat->GetRulerId() || !Unit.UnitEntityId.IsValid()) continue;
        FOGInteractionParticipant Binding;
        Binding.Role = TEXT("participant"); Binding.Character.EntityId = Unit.UnitEntityId;
        Binding.Character.OwnerEntityId = Combat->GetRulerId();
        bool bFoundCopy = false; FOGCharacterManifestationRecord Manifestation;
        if (!BoundStore->TryReadCharacterManifestation(Unit.UnitEntityId, bFoundCopy, Manifestation, Error)) continue;
        if (bFoundCopy) Binding.Character.ManifestationId = Manifestation.ManifestationId;
        else Binding.Character.OwnerEntityId = Unit.UnitEntityId;
        if (Unit.UnitEntityId == Controlled) Binding.Actor = GetOwner();
        else for (TWeakObjectPtr<AActor> Actor : Combat->GetScenarioActors())
            if (AOGDiagnosticHumanoid* Humanoid = Cast<AOGDiagnosticHumanoid>(Actor.Get()))
                if (Humanoid->GetCombatState()->GetSnapshot().UnitEntityId == Unit.UnitEntityId)
                { Binding.Actor = Humanoid; break; }
        if (ACharacter* Character = Cast<ACharacter>(Binding.Actor.Get()))
            if (Character->GetMesh() && Character->GetMesh()->GetSkeletalMeshAsset() &&
                Character->GetMesh()->GetSkeletalMeshAsset()->GetSkeleton())
                Binding.RigFamilyId = FOGContentId(TEXT("rig:") + Character->GetMesh()->GetSkeletalMeshAsset()->GetSkeleton()->GetPathName().ToLower());
        Candidates.Add(MoveTemp(Binding));
    }
    if (bAll) SelectedParticipants = MoveTemp(Candidates);
    else if (!Candidates.IsEmpty())
    {
        SelectionIndex %= Candidates.Num(); SelectedParticipants = {Candidates[SelectionIndex]};
        SelectionIndex = (SelectionIndex + 1) % Candidates.Num();
    }
    else SelectedParticipants.Reset();
    Status = FString::Printf(TEXT("Selected / "), SelectedParticipants.Num());
}

void UOGFoundationInteractionHost::RefreshProductionSelection()
{
    AvailableParticipants.Reset(); AvailableDefinitions.Reset();
    if (!BoundStore || !GetWorld()) return;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed()) continue;
        auto* Component = Actor->FindComponentByClass<UOGInteractionParticipantComponent>();
        FOGInteractionParticipant Participant; FString Error;
        if (!Component || !Component->BuildParticipant(TEXT("participant"), Participant, Error))
        {
            // Reuse existing factual identity evidence. Never create an NPC,
            // infer identity from an actor name, or relabel the possessed pawn.
            FOGEntityId Entity;
            if (const auto* Canonical = Actor->FindComponentByClass<UOGCanonicalCharacterPresentation>();
                Canonical && Canonical->HasProjection()) Entity = Canonical->GetProjection().Context.EntityId;
            else if (const auto* Body = Actor->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
                Body && Core->GetOwnedLuminousTurnBattle() &&
                Body->IsBoundTo(Core->GetOwnedLuminousTurnBattle()->GetState().BattleId, Body->GetBoundUnitId()))
                Entity = Body->GetBoundUnitId();
            else if (const auto* Subject = Cast<AOGNarrativeWorldActor>(Actor)) Entity = Subject->CanonicalSubjectId;
            if (!Entity.IsValid()) continue;
            TArray<USkeletalMeshComponent*> Parts; UOGSharedMotionControllerComponent* Motion = nullptr;
            if (!UOGInteractionParticipantComponent::DiscoverNativeBinding(Actor, Parts, Motion, Error)) continue;
            FString Rig = Motion && !Motion->RigProfile.IsNone() ? Motion->RigProfile.ToString().ToLower() :
                Parts[0]->GetSkeletalMeshAsset()->GetSkeleton()->GetPathName().ToLower();
            const FOGContentId Family = FOGContentId(Rig).IsValid() ? FOGContentId(Rig) : FOGContentId(TEXT("rig:") + Rig);
            const bool bCreated = !Component;
            if (!Component)
            {
                Component = NewObject<UOGInteractionParticipantComponent>(Actor);
                Actor->AddInstanceComponent(Component); Component->RegisterComponent();
            }
            if (!Component->BindActualNativeParticipant(Entity, Family, Error) ||
                !Component->BuildParticipant(TEXT("participant"), Participant, Error))
            { if (bCreated) Component->DestroyComponent(); continue; }
        }
        if (Participant.Character.EntityId != BoundRulerId) AvailableParticipants.Add(MoveTemp(Participant));
    }
    AvailableParticipants.Sort([](const auto& A, const auto& B)
        { return A.Character.EntityId.ToString() < B.Character.EntityId.ToString(); });
    SelectedParticipants.RemoveAll([this](const auto& P)
        { return !AvailableParticipants.ContainsByPredicate([&](const auto& A)
            { return A.Character.EntityId == P.Character.EntityId && A.Actor == P.Actor; }); });
    UOGInteractionCatalog* Content = Catalog.IsNull()
        ? LoadObject<UOGInteractionCatalog>(nullptr, TEXT("/Game/NoWorld/Interactions/DA_NoWorld_InteractionCatalog.DA_NoWorld_InteractionCatalog"))
        : Catalog.LoadSynchronous();
    if (Content)
    {
        TSet<FOGContentId> Ids;
        for (const auto& Reference : Content->Definitions)
            if (auto* Definition = Reference.LoadSynchronous())
            {
                FOGInteractionActionGraph Checked; FString Error;
                if (!Definition->BuildGraph(Checked, Error) || Ids.Contains(Definition->GraphId))
                { Status = TEXT("Installed interaction catalog contains an invalid or duplicate definition."); AvailableDefinitions.Reset(); SelectedDefinition = nullptr; return; }
                Ids.Add(Definition->GraphId); AvailableDefinitions.Add(Definition);
            }
    }
    if (!AvailableDefinitions.Contains(SelectedDefinition))
        SelectedDefinition = AvailableDefinitions.IsEmpty() ? nullptr : AvailableDefinitions[0];
    if (!SelectedDefinition && !(Combat.IsValid() && Combat->IsActive()))
        Status = TEXT("No original interaction definitions installed. Real participant bindings are available; no substitute actions created.");
}

bool UOGFoundationInteractionHost::BeginContext(FName Context,
    const TArray<FOGInteractionParticipant>& Participants, const FOGEntityId& ContextEvent)
{
    if (!Initialize() || IsActive()) return false;
    if (const auto* Battle = Core->GetOwnedLuminousTurnBattle(); Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running)
    { Status = TEXT("Finish the active combat encounter before entering an interaction."); return false; }
    FOGInteractionParticipant Ruler; FString Error;
    if (!UOGInteractionParticipantComponent::ResolveActualRulerParticipant(
        GetWorld(), *BoundStore, BoundRulerId, Ruler, Error))
    { Status = Error; return false; }
    FOGInteractionEntry Entry; Entry.Context = Context; Entry.ContextEventId = ContextEvent;
    Entry.World = GetWorld(); Entry.PreferredLocation = Ruler.Actor->GetActorLocation();
    if (APawn* Pawn = Cast<APawn>(GetOwner())) Entry.ReturnController = Cast<APlayerController>(Pawn->GetController());
    if (const auto* Player = Cast<AOGWorldPrototypeCharacter>(GetOwner())) Entry.bPrivacyPresentation = Player->GetSfwPresentation();
    bool bPresence = false; FOGWorldPresenceRecord Presence;
    if (!BoundStore->TryReadWorldPresence(BoundRulerId, bPresence, Presence, Error)) { Status = Error; return false; }
    if (bPresence) Entry.LocationId = Presence.LocationId;
    Entry.Participants.Add(Ruler);
    const auto& Others = Participants.IsEmpty() ? SelectedParticipants : Participants;
    for (const auto& Binding : Others) if (Binding.Character.EntityId != BoundRulerId) Entry.Participants.Add(Binding);
    FOGInteractionActionGraph Graph;
    if (SelectedDefinition)
    {
        TArray<FOGInteractionParticipant> Bound;
        if (!SelectedDefinition->BuildGraph(Graph, Error) || !SelectedDefinition->BindRoles(Entry.Participants, Bound, Error))
        { Status = Error; return false; }
        Entry.Participants = MoveTemp(Bound);
        // Source lore classification only. General mature/violence graphs do not consume this check.
        if (SelectedDefinition->bContainsAdultSource)
            for (const auto& P : Entry.Participants)
            {
                auto* Source = P.Actor->FindComponentByClass<UOGInteractionParticipantComponent>();
                if (!Source || !Source->HasCanonicalAdultSource(Error))
                { Status = Error.IsEmpty() ? TEXT("Original adult source metadata is unavailable for this participant.") : Error; return false; }
            }
    }
    else if (Combat.IsValid() && Combat->IsActive()) Graph = FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph();
    else { Status = TEXT("Install the approved original action definition before starting interaction."); return false; }
    return EnterAuthored(Entry, Graph);
}

FOGEntityId UOGFoundationInteractionHost::GetAcceptedContextEvent(bool bCompletedBattle) const
{
    if (Core.IsValid())
    {
        const auto Event = Core->GetLastNativeCombatEventProvenance();
        if (Event.EventId.IsValid() && Event.OwningRulerId == BoundRulerId &&
            Event.WorldTick >= 0 && Event.WorldTick <= Core->GetCanonicalWorldTick() &&
            (!bCompletedBattle || Event.BattleStatus == EOGTurnBattleStatus::Completed))
            return Event.EventId;
    }
    return Combat.IsValid() && Combat->IsActive() ? Combat->GetResolvedEncounterEventId() : FOGEntityId();
}

bool UOGFoundationInteractionHost::EnterAuthored(const FOGInteractionEntry& Entry,
    const FOGInteractionActionGraph& Graph)
{
    if (!Initialize() || IsActive()) return false;
    if (Entry.World.IsValid())
    {
        FOGInteractionParticipant RealRuler; FString Error;
        if (Entry.World.Get() != GetWorld() || !UOGInteractionParticipantComponent::ResolveActualRulerParticipant(
            GetWorld(), *BoundStore, BoundRulerId, RealRuler, Error)) { Status = Error; return false; }
        for (const auto& Binding : Entry.Participants)
            if (Binding.Role == TEXT("player") && Binding.Actor != RealRuler.Actor)
            { Status = TEXT("Player-side participant must be the actual canonical Ruler body."); return false; }
    }
    if (auto* Controller = Entry.ReturnController.Get())
        if (auto* Hud = Cast<AOGWorldPresentationHud>(Controller->GetHUD()))
        {
            InteractionHud = Hud; SavedHudSurface = Hud->GetInteractionReturnSurface();
            Hud->SetInteractionSurface(TEXT("world"));
        }
    // Reveal ordinary World presentation before native entry captures visibility.
    // Otherwise a Ruler-hidden participant would be saved as permanently hidden,
    // and Privacy toggles/exit could restore that suspension inside the scene.
    // Failure and every native exit route restore the previous presentation mode.
    if (Entry.World.IsValid() && Entry.World->GetGameInstance())
    {
        PresentationMode = Entry.World->GetGameInstance()->GetSubsystem<UOGPresentationModeSubsystem>();
        if (PresentationMode.IsValid())
        {
            SavedPresentationMode = PresentationMode->GetMode();
            bOwnsPresentationMode = SavedPresentationMode != EOGPresentationMode::World;
            if (bOwnsPresentationMode) PresentationMode->SetMode(EOGPresentationMode::World);
        }
    }
    BoundActors.Reset(); MeshRestores.Reset(); ActiveGraph = Graph;
    for (const auto& Binding : Entry.Participants) BoundActors.Add(Binding.Character.EntityId, Binding.Actor);
    bPrivacy = Entry.bPrivacyPresentation;
    if (const AOGWorldPrototypeCharacter* Player = Cast<AOGWorldPrototypeCharacter>(GetOwner()))
        bLastProfilePrivacy = Player->GetSfwPresentation();
    FString Error;
    APlayerController* Controller = Entry.ReturnController.Get();
    APawn* RulerPawn = Cast<APawn>(FindBoundActor(BoundRulerId));
    TWeakObjectPtr<APawn> PreviousPawn = Controller ? Controller->GetPawn() : nullptr;
    if (Controller && (!RulerPawn || !Controller->IsLocalController()))
    { RestoreGameplay(); Status = TEXT("The real Ruler pawn must be locally controllable."); return false; }
    const bool bSwitch = Controller && PreviousPawn.Get() != RulerPawn;
    if (bSwitch)
    {
        if (auto* Hud = Cast<AOGWorldPresentationHud>(Controller->GetHUD()))
            Hud->RememberEmbodiedOwnedCharacter(Cast<AOGWorldPrototypeCharacter>(PreviousPawn.Get()));
        Controller->Possess(RulerPawn);
        if (Controller->GetPawn() != RulerPawn)
        { if (PreviousPawn.IsValid()) Controller->Possess(PreviousPawn.Get()); RestoreGameplay(); Status = TEXT("Ruler control could not be transferred; previous character retained."); return false; }
    }
    if (!Runtime->Enter(Entry, Graph, LastTick, Error))
    {
        if (bSwitch && PreviousPawn.IsValid()) Controller->Possess(PreviousPawn.Get());
        RestoreGameplay(); Status = Error; return false;
    }
    if (Controller && GetWorld()->GetGameInstance())
        if (auto* Session = GetWorld()->GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>())
            if (!Session->SelectRulerForWorld(Error))
            {
                Cancel(TEXT("ruler_selection_failed"));
                if (bSwitch && PreviousPawn.IsValid()) Controller->Possess(PreviousPawn.Get());
                Status = Error; return false;
            }
    Status = FString::Printf(TEXT("%s: %d canonical participants. World time continues."),
        *Entry.Context.ToString(), Entry.Participants.Num());
    return true;
}

TArray<FOGFoundationInteractionRow> UOGFoundationInteractionHost::GetRows() const
{
    TArray<FOGFoundationInteractionRow> Rows;
    if (!IsActive())
    {
        const bool bReady = Core.IsValid() && Core->IsCoreReady() && BoundRulerId.IsValid();
        for (UOGInteractionDefinition* Definition : AvailableDefinitions)
            if (Definition) Rows.Add({FName(*(TEXT("definition.") + Definition->GraphId.ToString())),
                (Definition == SelectedDefinition ? TEXT("Selected / ") : TEXT("Choose / ")) + Definition->DisplayName.ToString(), true});
        for (int32 I = 0; I < AvailableParticipants.Num(); ++I)
        {
            const auto& P = AvailableParticipants[I];
            const bool Selected = SelectedParticipants.ContainsByPredicate([&](const auto& S) { return S.Character.EntityId == P.Character.EntityId; });
            const auto* Source = P.Actor.IsValid() ? P.Actor->FindComponentByClass<UOGInteractionParticipantComponent>() : nullptr;
            const FString Name = Source && !Source->DisplayName.IsEmpty() ? Source->DisplayName.ToString() : FString::Printf(TEXT("Character %d"), I + 1);
            Rows.Add({FName(*(TEXT("participant.") + P.Character.EntityId.ToString())),
                FString(Selected ? TEXT("Remove / ") : TEXT("Include / ")) + Name, bReady});
        }
        Rows.Add({TEXT("refresh"), TEXT("Refresh nearby character bindings"), bReady});
        Rows.Add({TEXT("clear_participants"), TEXT("Clear participant selection"), !SelectedParticipants.IsEmpty()});
        const bool bContent = SelectedDefinition || (Combat.IsValid() && Combat->IsActive());
        Rows.Add({TEXT("world"), TEXT("Begin World interaction"), bReady && bContent});
        Rows.Add({TEXT("ruler"), TEXT("Begin Ruler interaction"), bReady && bContent});
        Rows.Add({TEXT("roster"), TEXT("Begin character interaction"), bReady && bContent});
        Rows.Add({TEXT("npc"), TEXT("Begin selected NPC interaction"), bReady && bContent && !SelectedParticipants.IsEmpty()});
        const bool bEvent = GetAcceptedContextEvent(false).IsValid();
        Rows.Add({TEXT("event"), TEXT("Event interaction"), bContent && bEvent});
        Rows.Add({TEXT("post_combat"), TEXT("Post-combat interaction"), bContent && GetAcceptedContextEvent(true).IsValid()});
        if (Combat.IsValid() && Combat->IsActive())
        {
            Rows.Add({TEXT("select_next"), TEXT("Diagnostic: next canonical participant"), true});
            Rows.Add({TEXT("select_all"), TEXT("Diagnostic: available group"), true});
        }
    }
    else
    {
        TArray<FName> Next = Runtime->GetAvailableActions();
        const auto& Queue = Runtime->GetQueuedActions();
        if (!Queue.IsEmpty())
            if (const auto* Tail = ActiveGraph.Nodes.FindByPredicate([&](const auto& N) { return N.NodeId == Queue.Last(); })) Next = Tail->NextNodes;
        for (FName Id : Next)
            if (const auto* Node = ActiveGraph.Nodes.FindByPredicate([Id](const auto& N) { return N.NodeId == Id; }))
                Rows.Add({FName(*(TEXT("action.") + Id.ToString())),
                    FString(Runtime->IsAwaitingAction() && Queue.IsEmpty() ? TEXT("Play / ") : TEXT("Queue / ")) +
                    (Node->DisplayName.IsEmpty() ? Id.ToString() : Node->DisplayName), true});
        Rows.Add({TEXT("clear_queue"), FString::Printf(TEXT("Clear sequence (%d queued)"), Queue.Num()), !Queue.IsEmpty()});
        Rows.Add({TEXT("pacing"), FString::Printf(TEXT("Pacing %.2fx: change"), Runtime->GetPacing()), true});
        Rows.Add({TEXT("camera"), TEXT("Orbit camera"), true});
        Rows.Add({TEXT("restage"), TEXT("Adapt staging to nearby space"), true});
        Rows.Add({TEXT("privacy"), bPrivacy ? TEXT("Presentation: masked") : TEXT("Presentation: visible"), true});
        Rows.Add({TEXT("exit"), TEXT("Return to ordinary gameplay"), true});
    }
    return Rows;
}

bool UOGFoundationInteractionHost::ActivateRow(FName Id)
{
    if (!Initialize()) return false;
    const auto Rows = GetRows();
    const auto* Row = Rows.FindByPredicate([Id](const auto& Value) { return Value.Id == Id; });
    if (!Row || !Row->bEnabled) { Status = TEXT("This control is unavailable in the current state."); return false; }
    const FString Key = Id.ToString();
    if (Key.StartsWith(TEXT("definition.")))
    {
        for (UOGInteractionDefinition* Definition : AvailableDefinitions)
            if (Definition && Key == TEXT("definition.") + Definition->GraphId.ToString())
            { SelectedDefinition = Definition; Status = Definition->Description.ToString(); return true; }
        return false;
    }
    if (Key.StartsWith(TEXT("participant.")))
    {
        const auto* P = AvailableParticipants.FindByPredicate([&](const auto& A) { return Key == TEXT("participant.") + A.Character.EntityId.ToString(); });
        if (!P) return false;
        const int32 Index = SelectedParticipants.IndexOfByPredicate([&](const auto& A) { return A.Character.EntityId == P->Character.EntityId; });
        if (Index == INDEX_NONE) SelectedParticipants.Add(*P); else SelectedParticipants.RemoveAt(Index);
        Status = FString::Printf(TEXT("%d actual participants selected, plus the canonical Ruler."), SelectedParticipants.Num()); return true;
    }
    if (Id == TEXT("refresh")) { RefreshProductionSelection(); return true; }
    if (Id == TEXT("clear_participants")) { SelectedParticipants.Reset(); return true; }
    if (IsEntry(Id)) return BeginContext(Id, {}, (Id == TEXT("event") || Id == TEXT("post_combat")) ? GetAcceptedContextEvent(Id == TEXT("post_combat")) : FOGEntityId());
    if (Id == TEXT("select_next") || Id == TEXT("select_all")) { BuildDiagnosticSelection(Id == TEXT("select_all")); return true; }
    if (Id == TEXT("exit")) { Cancel(); return true; }
    FString Error; bool bResult = false;
    if (Key.StartsWith(TEXT("action.")))
    {
        const FName Node(*Key.Mid(7));
        if (Runtime->IsAwaitingAction() && Runtime->GetQueuedActions().IsEmpty()) bResult = Runtime->SelectAction(Node, Error);
        else { auto Queue = Runtime->GetQueuedActions(); Queue.Add(Node); bResult = Runtime->SetSequence(Queue, Error); }
    }
    else if (Id == TEXT("clear_queue")) bResult = Runtime->SetSequence({}, Error);
    else if (Id == TEXT("pacing"))
    {
        const double Rate = Runtime->GetPacing();
        bResult = Runtime->SetPacing(Rate == 0 ? 0.5 : Rate < 1 ? 1.0 : Rate < 2 ? 2.0 : 0.0, Error); UpdateAnimationPacing();
    }
    else if (Id == TEXT("camera"))
    {
        AActor* Ruler = FindBoundActor(BoundRulerId);
        const FVector Center = Ruler ? Ruler->GetActorLocation() : GetOwner()->GetActorLocation();
        CameraAngle += PI / 4;
        const FVector Camera = Center + FVector(FMath::Cos(CameraAngle) * 450, FMath::Sin(CameraAngle) * 450, 180);
        bResult = Runtime->SetCamera(FTransform((Center - Camera).Rotation(), Camera), Error);
    }
    else if (Id == TEXT("restage")) bResult = Runtime->Restage(GetOwner()->GetActorLocation() + GetOwner()->GetActorRightVector() * 150, Error);
    else if (Id == TEXT("privacy")) { bPrivacy = !bPrivacy; Runtime->SetPrivacyPresentation(bPrivacy); bResult = true; }
    Status = bResult ? FString::Printf(TEXT("%s accepted."), *Row->Label) : Error; return bResult;
}

void UOGFoundationInteractionHost::Cancel(FName Reason)
{
    if (Runtime && Runtime->IsActive())
    {
        FString Error;
        const int64 Tick = Core.IsValid() && Core->GetCanonicalWorldTick() >= 0 ? Core->GetCanonicalWorldTick() : LastTick;
        const bool bSaved = Runtime->Exit(Reason, Tick, Error);
        Status = bSaved ? TEXT("Returned to ordinary gameplay.") : Error;
    }
    RestoreGameplay();
}

void UOGFoundationInteractionHost::TickComponent(float Delta, ELevelTick Tick,
    FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Tick, Function);
    if (!IsActive()) return;
    if (!EnsureRuntime()) { Cancel(TEXT("core_unavailable")); return; }
    if (const auto* Battle = Core->GetOwnedLuminousTurnBattle();
        Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running)
    {
        TWeakObjectPtr<AOGWorldPresentationHud> Hud = InteractionHud;
        Cancel(TEXT("combat_started"));
        if (Hud.IsValid()) Hud->CloseInteractionForCombat();
        return;
    }
    if (const AOGWorldPrototypeCharacter* Player = Cast<AOGWorldPrototypeCharacter>(GetOwner()))
        if (Player->GetSfwPresentation() != bLastProfilePrivacy)
        {
            bLastProfilePrivacy = Player->GetSfwPresentation();
            Runtime->SetPrivacyPresentation(bLastProfilePrivacy);
        }
    FString Error;
    // Only action progress uses pacing. The independent Core ticker remains the
    // sole canonical time authority, including when selected pacing is zero.
    if (!Runtime->Advance(Delta, LastTick, Error))
    { Cancel(TEXT("runtime_error")); Status = Error; }
    UpdateAnimationPacing();
}

void UOGFoundationInteractionHost::ApplyCharacter(const FOGInteractionParticipantState& State)
{
    ACharacter* Actor = Cast<ACharacter>(FindBoundActor(State.Binding.Character.EntityId));
    if (Actor && Actor->GetMesh() && !MeshRestores.Contains(State.Binding.Character.EntityId))
    {
        FMeshRestore Saved; Saved.Mesh = Actor->GetMesh();
        Saved.TickPolicy = static_cast<uint8>(Saved.Mesh->VisibilityBasedAnimTickOption);
        Saved.bUpdateRateOptimization = Saved.Mesh->bEnableUpdateRateOptimizations;
        Saved.Animation = Actor->FindComponentByClass<UOGDiagnosticAnimationPresentation>();
        if (Saved.Animation.IsValid())
            if (const auto* Previous = Saved.Animation->Clips.Find(InteractionClip))
            {
                Saved.bHadClip = true; Saved.PreviousClip = *Previous;
                Saved.PreviousSequence = TStrongObjectPtr<UAnimSequence>(Previous->Sequence.Get());
            }
        MeshRestores.Add(State.Binding.Character.EntityId, Saved);
        // Embodied staging takes ownership from ordinary diagnostic locomotion/actions.
        if (Saved.Animation.IsValid()) Saved.Animation->ResetPresentation();
    }
    // Character refresh marks an action boundary: optional clips from the
    // completed action must not leak into a subsequent unbound action.
    if (FMeshRestore* Saved = MeshRestores.Find(State.Binding.Character.EntityId);
        Saved && Saved->bOwnsClip && Saved->Animation.IsValid())
    { Saved->Animation->SetPresentedState(NAME_None); Saved->bOwnsClip = false; }
    if (AActor* Physical = FindBoundActor(State.Binding.Character.EntityId))
        if (auto* Participant = Physical->FindComponentByClass<UOGInteractionParticipantComponent>())
        { FString Error; if (!Participant->RefreshFromCanonical(Error)) Status = Error; }
    OnCharacterPresentation.Broadcast(State);
}

void UOGFoundationInteractionHost::SetSignificance(const FOGEntityId& Entity, bool bAction, bool bCamera)
{
    if (AActor* Physical = FindBoundActor(Entity))
        if (auto* Participant = Physical->FindComponentByClass<UOGInteractionParticipantComponent>())
            Participant->SetInteractionSignificance(bAction, bCamera);
    if (FMeshRestore* Saved = MeshRestores.Find(Entity); Saved && Saved->Mesh.IsValid())
    {
        Saved->Mesh->bEnableUpdateRateOptimizations = !(bAction || bCamera);
        Saved->Mesh->VisibilityBasedAnimTickOption = bAction || bCamera ?
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    }
}

void UOGFoundationInteractionHost::ApplyConstraint(const FOGInteractionConstraint& Constraint,
    const TArray<FOGInteractionParticipantState>& Participants)
{ OnConstraintPresentation.Broadcast(Constraint, Participants); }

UOGSharedMotionControllerComponent* UOGFoundationInteractionHost::FindMotionController(AActor* Actor) const
{
    if (!Actor) return nullptr;
    if (auto* Participant = Actor->FindComponentByClass<UOGInteractionParticipantComponent>())
        return Participant->GetMotionController();
    TArray<USkeletalMeshComponent*> Parts; UOGSharedMotionControllerComponent* Motion = nullptr; FString Error;
    return UOGInteractionParticipantComponent::DiscoverNativeBinding(Actor, Parts, Motion, Error) ? Motion : nullptr;
}

void UOGFoundationInteractionHost::OwnMotion(UOGSharedMotionControllerComponent* Motion)
{
    if (!Motion || OwnedMotions.ContainsByPredicate([Motion](const auto& M) { return M.Motion.Get() == Motion; })) return;
    FMotionRestore Saved; Saved.Motion = Motion;
    Saved.bRootMotion = Motion->bPermitAuthoritativeRootMotion; Saved.Pacing = Motion->GetExternalPacing();
    OwnedMotions.Add(Saved); Motion->bPermitAuthoritativeRootMotion = false;
}

bool UOGFoundationInteractionHost::PlaySharedAction(const FOGEntityId& Entity, FName Action, FName Context)
{
    auto* Motion = FindMotionController(FindBoundActor(Entity));
    if (!Motion || !Motion->CanPlayAction(Action, Context, false, 1)) return false;
    OwnMotion(Motion);
    const bool Played = Motion->PlayAction(Action, Context, false, 1);
    if (Played && Runtime) Motion->SetExternalPacing(static_cast<float>(Runtime->GetPacing()));
    return Played;
}

bool UOGFoundationInteractionHost::PlaySharedScene(
    const FOGInteractionActionNode& Node,
    const TArray<FOGInteractionParticipantState>& SourceParticipants)
{
    if (!GetWorld() || !GetOwner() || Node.SharedSceneAction.IsNone() ||
        Node.SharedSceneContext.IsNone() || SourceParticipants.Num() < 2) return false;

    TArray<FOGSharedMotionParticipant> Roles;
    Roles.Reserve(SourceParticipants.Num());
    for (const FOGInteractionParticipantState& Participant : SourceParticipants)
    {
        // Observers retain canonical state and ordinary LOD; only active original tracks animate.
        if (!Node.RequiredRoles.Contains(Participant.Binding.Role)) continue;
        const FOGInteractionAnimationBinding* Authored = nullptr;
        int32 BestSpecificity = -1; bool bAmbiguous = false;
        for (const FOGInteractionAnimationBinding& Binding : Node.AnimationBindings)
        {
            // Shared scene assets declare a real source role. The global
            // controller tests actual native Skeletons, so no synthetic
            // per-Version RigFamilyId is required to discover this role.
            if (Binding.Role != Participant.Binding.Role ||
                (Binding.RigFamilyId.IsValid() &&
                 Binding.RigFamilyId != Participant.Binding.RigFamilyId))
                continue;
            const FOGEntityId RealManifestation =
                Participant.Character.bHasManifestation
                ? Participant.Character.Manifestation.ManifestationId
                : Participant.Binding.Character.ManifestationId;
            if (Binding.ManifestationId.IsValid() &&
                Binding.ManifestationId != RealManifestation) continue;
            if (!Binding.VersionId.IsEmpty() &&
                (!Participant.Character.bHasManifestation ||
                 Binding.VersionId !=
                    Participant.Character.Manifestation.ActiveVersionId))
                continue;
            const int32 Specificity = (Binding.ManifestationId.IsValid() ? 4 : 0) +
                (!Binding.VersionId.IsEmpty() ? 2 : 0) + (Binding.RigFamilyId.IsValid() ? 1 : 0);
            if (Specificity == BestSpecificity) bAmbiguous = true;
            if (Specificity > BestSpecificity) { Authored = &Binding; BestSpecificity = Specificity; bAmbiguous = false; }
        }
        if (bAmbiguous || !Authored || Authored->SharedSceneRole.IsNone()) return false;
        AActor* Actor = FindBoundActor(Participant.Binding.Character.EntityId);
        if (!Actor || Actor->GetWorld() != GetWorld()) return false;
        UOGSharedMotionControllerComponent* Controller = FindMotionController(Actor);
        if (!Controller) return false;
        FOGSharedMotionParticipant& Role = Roles.AddDefaulted_GetRef();
        Role.Controller = Controller;
        Role.SourceRole = Authored->SharedSceneRole;
    }

    if (Roles.Num() < 2 || Roles.Num() != Node.RequiredRoles.Num()) return false;
    if (!IsValid(ActiveSharedScene))
    {
        ActiveSharedScene = NewObject<UOGSharedMotionSceneComponent>(GetOwner());
        if (!ActiveSharedScene) return false;
        ActiveSharedScene->RegisterComponent();
    }
    ActiveSharedScene->Participants = MoveTemp(Roles);
    // Existing shared scene component preflights ALL authored source roles,
    // native Skeletons and grouped original scene IDs atomically. A rejected
    // clip is not silently substituted, including for mature interactions.
    for (const auto& Role : ActiveSharedScene->Participants) OwnMotion(Role.Controller);
    const bool Played = ActiveSharedScene->StartSynchronizedScene(Node.SharedSceneAction, Node.SharedSceneContext, false);
    if (Played) UpdateAnimationPacing();
    return Played;
}

void UOGFoundationInteractionHost::StopSharedScene()
{
    if (IsValid(ActiveSharedScene))
        ActiveSharedScene->StopSynchronizedScene();
    for (const auto& Saved : OwnedMotions) if (auto* Motion = Saved.Motion.Get())
    {
        Motion->StopAction(); Motion->bPermitAuthoritativeRootMotion = Saved.bRootMotion;
        Motion->SetExternalPacing(Saved.Pacing);
    }
    OwnedMotions.Reset();
}

void UOGFoundationInteractionHost::ApplyAnimation(const FOGEntityId& Entity, UObject* Asset)
{
    UAnimSequence* Sequence = Cast<UAnimSequence>(Asset);
    // Existing authored optional source-clip bindings also run through the
    // central multi-part rig controller when their exact native source is
    // already gameplay accepted; preserve legacy animation as fallback.
    if (Sequence)
        if (const UOGSharedMotionLibrary* Library =
                FOGSharedMotionResolver::GetInstalledLibrary())
            for (const FOGSharedMotionEntry& Entry : Library->Motions)
                if (Entry.bApprovedForGameplay &&
                    Entry.NativeSequence.ToSoftObjectPath().ToString() ==
                        Sequence->GetPathName() &&
                    PlaySharedAction(Entity, Entry.GameplayAction,
                        Entry.GameplayContext))
                    return;
    FMeshRestore* Saved = MeshRestores.Find(Entity);
    if (!Saved || !Saved->Mesh.IsValid() || !Saved->Animation.IsValid() || !Sequence ||
        !Saved->Mesh->GetSkeletalMeshAsset() ||
        Sequence->GetSkeleton() != Saved->Mesh->GetSkeletalMeshAsset()->GetSkeleton()) return;
    FOGDiagnosticAnimationClip Clip; Clip.Sequence = Sequence; Clip.bLoop = true;
    Saved->Animation->Clips.Add(InteractionClip, Clip); Saved->bOwnsClip = true;
    Saved->Animation->SetPresentedState(NAME_None);
    Saved->Animation->SetPresentedState(InteractionClip);
    UpdateAnimationPacing();
}

void UOGFoundationInteractionHost::EndAction()
{
    for (auto& Pair : MeshRestores)
        if (Pair.Value.bOwnsClip && Pair.Value.Animation.IsValid())
        { Pair.Value.Animation->SetPresentedState(NAME_None); Pair.Value.bOwnsClip = false; }
}

void UOGFoundationInteractionHost::ApplyReaction(const FOGEntityId& Entity, UObject* Expression, UObject* Reaction)
{ OnReactionPresentation.Broadcast(Entity, Expression, Reaction); }

void UOGFoundationInteractionHost::SetPrivacy(bool Privacy)
{
    bPrivacy = Privacy;
    // The runtime owns masking and restores each actor's original visibility.
    // No canonical state command is issued by this presentation callback.
}

void UOGFoundationInteractionHost::UpdateAnimationPacing()
{
    if (!Runtime) return;
    for (const auto& Saved : OwnedMotions)
        if (auto* Motion = Saved.Motion.Get()) Motion->SetExternalPacing(static_cast<float>(Runtime->GetPacing()));
    for (auto& Pair : MeshRestores)
        if (Pair.Value.bOwnsClip && Pair.Value.Mesh.IsValid() && Pair.Value.Animation.IsValid())
            if (const auto* Clip = Pair.Value.Animation->Clips.Find(InteractionClip))
                if (auto* Instance = Pair.Value.Mesh->GetSingleNodeInstance();
                    Instance && Instance->GetCurrentAsset() == Clip->Sequence)
                    Pair.Value.Mesh->SetPlayRate(static_cast<float>(Runtime->GetPacing()));
}

void UOGFoundationInteractionHost::RestoreGameplay()
{
    StopSharedScene();
    for (const auto& Pair : BoundActors) if (auto* Actor = Pair.Value.Get())
        if (auto* Participant = Actor->FindComponentByClass<UOGInteractionParticipantComponent>()) Participant->RestoreSignificance();
    // Restore ALL participant native motion tracks before the canonical
    // interaction returns the characters to ordinary World control.
    if (IsValid(ActiveSharedScene))
    {
        ActiveSharedScene->StopSynchronizedScene();
        ActiveSharedScene->Participants.Reset();
    }
    for (auto& Pair : MeshRestores)
    {
        FMeshRestore& Saved = Pair.Value;
        if (Saved.Animation.IsValid())
        {
            if (Saved.bOwnsClip) Saved.Animation->SetPresentedState(NAME_None);
            if (Saved.bHadClip) Saved.Animation->Clips.Add(InteractionClip, Saved.PreviousClip);
            else Saved.Animation->Clips.Remove(InteractionClip);
        }
        if (Saved.Mesh.IsValid())
        {
            Saved.Mesh->VisibilityBasedAnimTickOption = static_cast<EVisibilityBasedAnimTickOption>(Saved.TickPolicy);
            Saved.Mesh->bEnableUpdateRateOptimizations = Saved.bUpdateRateOptimization;
        }
    }
    MeshRestores.Reset(); BoundActors.Reset();
    if (bOwnsPresentationMode)
    {
        bOwnsPresentationMode = false;
        if (PresentationMode.IsValid()) PresentationMode->SetMode(SavedPresentationMode);
    }
    PresentationMode.Reset();
    if (InteractionHud.IsValid()) InteractionHud->SetInteractionSurface(SavedHudSurface);
    InteractionHud.Reset(); SavedHudSurface = NAME_None;
}

void UOGFoundationInteractionHost::EndPlay(const EEndPlayReason::Type Reason)
{
    Cancel(TEXT("host_end_play")); Runtime.Reset(); BoundStore = nullptr;
    if (Core.IsValid()) Core->OnCanonicalRuntimeReleasing.Remove(ReleaseHandle);
    ReleaseHandle.Reset(); Core.Reset();
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateHandle);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
    DeactivateHandle.Reset(); BackgroundHandle.Reset();
    Super::EndPlay(Reason);
}
