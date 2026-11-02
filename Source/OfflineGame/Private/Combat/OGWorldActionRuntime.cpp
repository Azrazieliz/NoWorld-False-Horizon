#include "Combat/OGWorldActionRuntime.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGCombatVitalityService.h"
#include "Combat/OGActionCombatAdapter.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Progression/OGCharacterDevelopmentService.h"
#include "World/OGOwnedEquipmentService.h"
#include "World/OGTraversalFramework.h"

namespace
{
bool ProjectLiveDevelopment(UWorld* World, FOGCombatUnitState& Unit, FString& Error)
{
    UOGGameCoreSubsystem* Core = World && World->GetGameInstance()
        ? World->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    return !Core || !Core->IsCoreReady() || !Core->GetWorldStore() ||
        (FOGOwnedEquipmentService(*Core->GetWorldStore()).ProjectCombatUnit(Unit, Error) &&
         FOGCharacterDevelopmentService(*Core->GetWorldStore()).ProjectCombatUnit(Unit, Error));
}
}

bool FOGActionCancelWindow::Allows(float ElapsedSeconds, EOGActionCancelDestination Destination) const
{
    return ElapsedSeconds >= OpensAtSeconds &&
        ElapsedSeconds <= ClosesAtSeconds &&
        Destinations.Contains(Destination);
}

UOGWorldActionRuntimeComponent::UOGWorldActionRuntimeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UOGWorldActionRuntimeComponent::BeginAuthoredAction(const FOGWorldActionState& State)
{
    ActiveAction = State;
    ActiveAction.StartedAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    bHasActiveAction = true;
}

bool UOGWorldActionRuntimeComponent::RestoreAuthoredAction(const FOGWorldActionState& State,float ElapsedSeconds)
{
    if (!GetWorld() || State.ActionId.IsNone() || !FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds<0) return false;
    ActiveAction=State; ActiveAction.StartedAtSeconds=GetWorld()->GetTimeSeconds()-ElapsedSeconds; bHasActiveAction=true;
    return true;
}

void UOGWorldActionRuntimeComponent::EndAuthoredAction()
{
    const bool WasActive=bHasActiveAction;
    bHasActiveAction = false;
    ActiveAction = FOGWorldActionState();
    if (WasActive) OnAuthoredActionEnded.Broadcast();
}

bool UOGWorldActionRuntimeComponent::CanCancelTo(EOGActionCancelDestination Destination) const
{
    if (!bHasActiveAction || !GetWorld())
    {
        return true;
    }

    const float Elapsed = GetWorld()->GetTimeSeconds() - ActiveAction.StartedAtSeconds;
    for (const FOGActionCancelWindow& Window : ActiveAction.CancelWindows)
    {
        if (Window.Allows(Elapsed, Destination))
        {
            return true;
        }
    }
    return false;
}

bool UOGWorldActionRuntimeComponent::CanContinueInTraversalMode(uint8 TraversalMode) const
{
    if (!bHasActiveAction)
    {
        return true;
    }

    switch (static_cast<EOGTraversalMode>(TraversalMode))
    {
    case EOGTraversalMode::Climbing: return ActiveAction.bAllowWhileClimbing;
    case EOGTraversalMode::Swimming: return ActiveAction.bAllowWhileSwimming;
    case EOGTraversalMode::Diving: return ActiveAction.bAllowWhileDiving;
    case EOGTraversalMode::Flying: return ActiveAction.bAllowWhileFlying;
    case EOGTraversalMode::Mounted:
    case EOGTraversalMode::Vehicle: return ActiveAction.bAllowWhileMounted;
    default: return ActiveAction.bAllowWhileAirborne;
    }
}

UOGWorldPartyRuntimeComponent::UOGWorldPartyRuntimeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UOGWorldPartyRuntimeComponent::RefreshDevelopmentStats(FString& Error)
{
    Error.Reset();
    UOGGameCoreSubsystem* Core = GetWorld() && GetWorld()->GetGameInstance()
        ? GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Core->GetWorldStore())
    { Error = TEXT("Canonical development service is unavailable."); return false; }
    TArray<FOGWorldPartySlot> Candidate = Slots;
    for (FOGWorldPartySlot& Slot : Candidate)
        if (!ProjectLiveDevelopment(GetWorld(), Slot.Unit, Error)) return false;
    Slots = MoveTemp(Candidate);
    return true;
}

bool UOGWorldPartyRuntimeComponent::RefreshEquipmentStats(FString& Error)
{
    return RefreshDevelopmentStats(Error);
}

bool UOGWorldPartyRuntimeComponent::ConfigureParty(const TArray<FOGCombatUnitState>& Units, FString& OutError)
{
    if (!FOGActionCombatAdapter::ValidateSwitchParty(Units, OutError)) return false;
    TArray<FOGCombatUnitState> Developed = Units;
    UOGGameCoreSubsystem* Core = GetWorld() && GetWorld()->GetGameInstance()
        ? GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    if (Core && Core->IsCoreReady() && Core->GetWorldStore())
        for (FOGCombatUnitState& Unit : Developed)
            if (!ProjectLiveDevelopment(GetWorld(), Unit, OutError) ||
                !FOGCombatVitalityService(*Core->GetWorldStore()).Project(Unit,OutError)) return false;
    Slots.Reset(); ControlledSlot = INDEX_NONE; LastBattleReceipt=FOGEntityId();
    for (const FOGCombatUnitState& Unit : Developed)
    {
        FOGWorldPartySlot& Slot = Slots.AddDefaulted_GetRef();
        Slot.Unit = FOGActionCombatAdapter::MakeActionSnapshot(Unit);
        Slot.bDefeated = !Slot.Unit.IsAlive();
        Slot.bAvailable = !Slot.bDefeated;
        Slot.Unit.Presence = Slot.bDefeated ? EOGCombatPresence::Defeated : EOGCombatPresence::Reserve;
        if (ControlledSlot == INDEX_NONE && Slot.bAvailable) ControlledSlot = Slots.Num() - 1;
    }
    if (Slots.IsValidIndex(ControlledSlot)) Slots[ControlledSlot].Unit.Presence = EOGCombatPresence::Active;
    if (Core && Core->IsCoreReady()) BindBattleProjection(Core);
    return true;
}

bool UOGWorldPartyRuntimeComponent::TrySwitchTo(int32 SlotIndex, bool bStateAllowsSwitch)
{
    if (!bStateAllowsSwitch || !Slots.IsValidIndex(SlotIndex) || SlotIndex == ControlledSlot ||
        !Slots[SlotIndex].bAvailable || Slots[SlotIndex].bDefeated || !Slots[SlotIndex].Unit.IsAlive()) return false;
    const int32 Previous = ControlledSlot;
    if (Slots.IsValidIndex(Previous) && !Slots[Previous].bDefeated)
        Slots[Previous].Unit.Presence = EOGCombatPresence::Reserve;
    ControlledSlot = SlotIndex;
    Slots[ControlledSlot].Unit.Presence = EOGCombatPresence::Active;
    Slots[ControlledSlot].bQteReady = false;
    OnControlTransferRequested(Previous, ControlledSlot);
    return true;
}

bool UOGWorldPartyRuntimeComponent::TryTriggerQte(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(ControlledSlot) || !Slots[ControlledSlot].Unit.IsAlive() ||
        !Slots.IsValidIndex(SlotIndex) || SlotIndex == ControlledSlot ||
        !Slots[SlotIndex].bAvailable || Slots[SlotIndex].bDefeated ||
        !Slots[SlotIndex].Unit.IsAlive() || !Slots[SlotIndex].bQteReady) return false;
    Slots[SlotIndex].bQteReady = false;
    OnQteRequested(ControlledSlot, SlotIndex);
    return true;
}

int32 UOGWorldPartyRuntimeComponent::MarkDefeatedAndResolveFallback(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex)) return ControlledSlot;
    Slots[SlotIndex].Unit.CurrentHp = FOGLargeNumber();
    Slots[SlotIndex].Unit.Presence = EOGCombatPresence::Defeated;
    Slots[SlotIndex].bDefeated = true; Slots[SlotIndex].bAvailable = false;
    Slots[SlotIndex].bQteReady = false;
    if (ControlledSlot != SlotIndex) return ControlledSlot;
    const int32 Previous = ControlledSlot;
    ControlledSlot = INDEX_NONE;
    for (int32 Index = 0; Index < Slots.Num(); ++Index)
        if (Slots[Index].bAvailable && !Slots[Index].bDefeated && Slots[Index].Unit.IsAlive())
        { ControlledSlot = Index; Slots[Index].Unit.Presence = EOGCombatPresence::Active; Slots[Index].bQteReady = false; break; }
    OnControlTransferRequested(Previous, ControlledSlot);
    return ControlledSlot;
}

void UOGWorldPartyRuntimeComponent::SetQteReady(int32 SlotIndex, bool bReady)
{
    if (Slots.IsValidIndex(SlotIndex))
        Slots[SlotIndex].bQteReady = bReady && SlotIndex != ControlledSlot &&
            Slots[SlotIndex].bAvailable && !Slots[SlotIndex].bDefeated && Slots[SlotIndex].Unit.IsAlive();
}


// REVIEW FRAGMENT: add declaration in UOGWorldPartyRuntimeComponent public:
// bool ApplyResolvedHp(int32 SlotIndex, const FOGEntityId& ExpectedUnitId,
//     const FOGLargeNumber& NewHp, FString& OutError);
// No UFUNCTION is needed: only the native canonical combat integration calls it.
// This mutation does not own damage math, progression, or authored revive rules.

bool UOGWorldPartyRuntimeComponent::ApplyResolvedHp(int32 SlotIndex,
    const FOGEntityId& ExpectedUnitId, const FOGLargeNumber& NewHp, FString& OutError)
{
    OutError.Reset();
    if (!Slots.IsValidIndex(SlotIndex) || !ExpectedUnitId.IsValid() ||
        Slots[SlotIndex].Unit.UnitEntityId != ExpectedUnitId || NewHp.GetSign() < 0)
    {
        OutError = TEXT("Party HP update requires the expected slot entity and nonnegative resolved HP.");
        return false;
    }
    FOGWorldPartySlot& Slot = Slots[SlotIndex];
    if (Slot.bDefeated && NewHp.GetSign() > 0)
    {
        OutError = TEXT("Defeated party units require an authored revival operation.");
        return false;
    }
    // Do not clamp to MaxHp: explicit overflow-HP mechanics belong to effects.
    Slot.Unit.CurrentHp = NewHp;
    if (NewHp.IsZero() && !Slot.bDefeated)
    {
        // Existing runtime owns defeat flags, QTE removal and fallback choice.
        MarkDefeatedAndResolveFallback(SlotIndex);
    }
    return true;
}

// Add public native declarations to the existing party runtime:
// bool AssignResolvedCompanion(int32 SlotIndex, const FOGCombatUnitState& Unit, FString& Error);
// bool RestorePresentationParty(const TArray<FOGWorldPartySlot>& SavedSlots, int32 SavedControl, FString& Error);
// Content callers must prove ownership/physical anchoring before assignment.

bool UOGWorldPartyRuntimeComponent::AssignResolvedCompanion(int32 SlotIndex,
    const FOGCombatUnitState& Unit, FString& Error)
{
    Error.Reset();
    if (SlotIndex < 1 || SlotIndex > 2 || SlotIndex > Slots.Num() || !Unit.IsAlive())
    { Error = TEXT("Companion assignment requires a living resolved unit in companion slot 1 or 2."); return false; }
    FOGCombatUnitState Developed = Unit;
    if (!ProjectLiveDevelopment(GetWorld(), Developed, Error)) return false;
    if (auto* Core=GetWorld() && GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
        Core && Core->IsCoreReady() && Core->GetWorldStore())
        if (!FOGCombatVitalityService(*Core->GetWorldStore()).Project(Developed,Error)) return false;
    if (!Developed.IsAlive()) { Error=TEXT("The actual stored companion is defeated."); return false; }
    TArray<FOGCombatUnitState> Candidate;
    for (const FOGWorldPartySlot& Slot : Slots) Candidate.Add(Slot.Unit);
    if (SlotIndex == Candidate.Num()) Candidate.Add(Developed); else Candidate[SlotIndex] = Developed;
    if (!FOGActionCombatAdapter::ValidateSwitchParty(Candidate, Error)) return false;
    FOGWorldPartySlot Replacement;
    Replacement.Unit = Developed;
    Replacement.Unit.Presence = SlotIndex == ControlledSlot ? EOGCombatPresence::Active : EOGCombatPresence::Reserve;
    const bool bSameUnit = Slots.IsValidIndex(SlotIndex) &&
        Slots[SlotIndex].Unit.UnitEntityId == Unit.UnitEntityId;
    if (!bSameUnit)
    {
        if (SlotIndex == Slots.Num()) Slots.Add(Replacement); else Slots[SlotIndex] = Replacement;
    }
    // Selecting a different living owned unit can recover a defeated party;
    // assigning an existing unit never restores or replaces its persisted HP.
    if (ControlledSlot == INDEX_NONE && Slots[SlotIndex].Unit.IsAlive())
        return TrySwitchTo(SlotIndex, true);
    return true;
}

bool UOGWorldPartyRuntimeComponent::RestorePresentationParty(
    const TArray<FOGWorldPartySlot>& SavedSlots, int32 SavedControl, FString& Error)
{
    Error.Reset();
    if (SavedSlots.IsEmpty() || SavedSlots.Num() > 3 ||
        (SavedControl != INDEX_NONE && !SavedSlots.IsValidIndex(SavedControl)))
    { Error = TEXT("Saved presentation party/control is invalid."); return false; }
    TArray<FOGCombatUnitState> Units;
    for (const FOGWorldPartySlot& Slot : SavedSlots) Units.Add(Slot.Unit);
    if (!FOGActionCombatAdapter::ValidateSwitchParty(Units, Error)) return false;
    if (SavedControl != INDEX_NONE &&
        (!SavedSlots[SavedControl].bAvailable || SavedSlots[SavedControl].bDefeated || !SavedSlots[SavedControl].Unit.IsAlive()))
    { Error = TEXT("Saved controlled slot is unavailable/defeated."); return false; }
    for (int32 Index = 0; Index < SavedSlots.Num(); ++Index)
    {
        const FOGWorldPartySlot& Slot = SavedSlots[Index];
        if (Slot.bDefeated != !Slot.Unit.IsAlive() || (!Slot.Unit.IsAlive() && Slot.bAvailable))
        { Error = TEXT("Saved HP and defeat/availability flags disagree."); return false; }
    }
    TArray<FOGWorldPartySlot> Developed = SavedSlots;
    auto* Core=GetWorld() && GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    for (FOGWorldPartySlot& Slot : Developed)
    {
        if (!ProjectLiveDevelopment(GetWorld(), Slot.Unit, Error)) return false;
        if (Core && Core->IsCoreReady() && Core->GetWorldStore() &&
            !FOGCombatVitalityService(*Core->GetWorldStore()).Project(Slot.Unit,Error)) return false;
        const bool WasDefeated=Slot.bDefeated;
        Slot.bDefeated=!Slot.Unit.IsAlive();
        if (Slot.bDefeated) { Slot.bAvailable=false; Slot.bQteReady=false; }
        else if (WasDefeated) Slot.bAvailable=true; // Only the stored canonical health authorized this recovery.
    }
    Slots = MoveTemp(Developed);
    ControlledSlot = SavedControl;
    if (!Slots.IsValidIndex(ControlledSlot) || Slots[ControlledSlot].bDefeated || !Slots[ControlledSlot].bAvailable)
        ControlledSlot=Slots.IndexOfByPredicate([](const auto& Slot){return Slot.bAvailable && !Slot.bDefeated && Slot.Unit.IsAlive();});
    for (int32 Index = 0; Index < Slots.Num(); ++Index)
        Slots[Index].Unit.Presence = Slots[Index].bDefeated ? EOGCombatPresence::Defeated :
            Index == ControlledSlot ? EOGCombatPresence::Active : EOGCombatPresence::Reserve;
    return true;
}

bool UOGWorldPartyRuntimeComponent::ApplyResolvedCharacterProjection(int32 SlotIndex, const FOGCombatUnitState& Projection, FString& Error)
{
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].Unit.UnitEntityId != Projection.UnitEntityId ||
        Slots[SlotIndex].Unit.IdentityId != Projection.IdentityId || Projection.Stats.MaxHp.GetSign() <= 0)
    { Error = TEXT("Character projection does not match the current canonical party unit."); return false; }
    FOGCombatUnitState Developed = Projection;
    // This entry point receives fresh content/equipment stats. A resolver may
    // copy the existing live unit before replacing Stats, leaving cache flags
    // behind. The dedicated refresh path uses the retained base instead.
    Developed.bHasDevelopmentStatProjection = false;
    Developed.bHasEquipmentStatProjection = false;
    if (!ProjectLiveDevelopment(GetWorld(), Developed, Error)) return false;
    auto& Unit = Slots[SlotIndex].Unit;
    Unit.Stats = Developed.Stats;
    Unit.EquipmentBaseStats = Developed.EquipmentBaseStats;
    Unit.bHasEquipmentStatProjection = Developed.bHasEquipmentStatProjection;
    Unit.DevelopmentBaseStats = Developed.DevelopmentBaseStats;
    Unit.bHasDevelopmentStatProjection = Developed.bHasDevelopmentStatProjection;
    Unit.RankProjection = Developed.RankProjection;
    Unit.SkillSet = Developed.SkillSet;
    return true;
}

void UOGWorldPartyRuntimeComponent::BeginPlay()
{
    Super::BeginPlay();
    auto* Core=GetWorld() && GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    BindBattleProjection(Core);
}
void UOGWorldPartyRuntimeComponent::BindBattleProjection(UOGGameCoreSubsystem* Core)
{
    if (BoundCore.Get()!=Core)
    {
        ReleaseBattleProjection(); BoundCore=Core;
        if (Core)
        {
            Core->OnNativeBattleStateCommitted.AddUObject(this,&ThisClass::HandleBattleCommitted);
            Core->OnCanonicalRuntimeReleasing.AddUObject(this,&ThisClass::ReleaseBattleProjection);
        }
    }
    if (Core && Core->IsCoreReady() && Core->GetOwnedLuminousTurnBattle() && Core->GetLastNativeBattleReceipt().IsValid())
        HandleBattleCommitted(Core->GetOwnedLuminousTurnBattle()->GetState(),Core->GetLastNativeBattleReceipt());
}
void UOGWorldPartyRuntimeComponent::ReleaseBattleProjection()
{
    if (BoundCore.IsValid())
    {
        BoundCore->OnNativeBattleStateCommitted.RemoveAll(this);
        BoundCore->OnCanonicalRuntimeReleasing.RemoveAll(this);
    }
    BoundCore.Reset(); LastBattleReceipt=LastSyncedBattle=FOGEntityId();
}
void UOGWorldPartyRuntimeComponent::EndPlay(const EEndPlayReason::Type Reason)
{ ReleaseBattleProjection(); Super::EndPlay(Reason); }
void UOGWorldPartyRuntimeComponent::HandleBattleCommitted(const FOGTurnBattleState& State,const FOGEntityId& Receipt)
{
    FString Error;
    if (!ApplyCommittedBattleState(State,Receipt,Error))
        UE_LOG(LogTemp,Warning,TEXT("Committed battle party projection unavailable: %s"),*Error);
}
bool UOGWorldPartyRuntimeComponent::ApplyCommittedBattleState(
    const FOGTurnBattleState& State,const FOGEntityId& Receipt,FString& Error)
{
    Error.Reset();
    if (!State.BattleId.IsValid() || !Receipt.IsValid() || (State.Status!=EOGTurnBattleStatus::Running && State.Status!=EOGTurnBattleStatus::Completed))
    { Error=TEXT("Party projection requires an actual committed native encounter receipt."); return false; }
    if (LastBattleReceipt==Receipt) return true;
    auto Candidate=Slots; bool Changed=false;
    TSet<FOGEntityId> Seen;
    for (const auto& U:State.Units)
    {
        if (!U.UnitEntityId.IsValid() || Seen.Contains(U.UnitEntityId))
        { Error=TEXT("Committed battle has duplicate or invalid participant identity."); return false; }
        Seen.Add(U.UnitEntityId);
    }
    for (auto& Slot:Candidate)
    {
        const auto* Actual=State.Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Slot.Unit.UnitEntityId;});
        if (!Actual) continue;
        if (Actual->IdentityId!=Slot.Unit.IdentityId || Actual->SkillSet.CurrentForm!=Slot.Unit.SkillSet.CurrentForm ||
            Actual->CurrentHp.GetSign()<0 || Actual->Stats.MaxHp.GetSign()<=0 ||
            (Actual->Presence==EOGCombatPresence::Defeated && Actual->IsAlive()))
        { Error=TEXT("Battle/party Identity, Version or HP projection differs; no partial party update applied."); return false; }
        if (Slot.bDefeated && Actual->IsAlive() && LastSyncedBattle!=State.BattleId)
        { Error=TEXT("A different encounter cannot restore a previously defeated World party member."); return false; }
        const bool WasDefeated=Slot.bDefeated;
        Slot.Unit.CurrentHp=Actual->CurrentHp;
        Slot.Unit.BattleResources=Actual->BattleResources;
        Slot.bDefeated=!Actual->IsAlive() || Actual->Presence==EOGCombatPresence::Defeated;
        if (Slot.bDefeated) { Slot.bAvailable=false; Slot.bQteReady=false; Slot.Unit.Presence=EOGCombatPresence::Defeated; }
        else if (WasDefeated)
        {
            // Only an already accepted revival within this same battle can reach this branch.
            Slot.bAvailable=true; Slot.bQteReady=false; Slot.Unit.Presence=EOGCombatPresence::Reserve;
        }
        Changed=true;
    }
    if (!Changed) return true;
    const int32 Previous=ControlledSlot; int32 Next=Previous;
    if (!Candidate.IsValidIndex(Next) || Candidate[Next].bDefeated || !Candidate[Next].bAvailable)
    {
        Next=INDEX_NONE;
        for (int32 I=0;I<Candidate.Num();++I)
            if (Candidate[I].bAvailable && !Candidate[I].bDefeated && Candidate[I].Unit.IsAlive()) { Next=I; break; }
    }
    for (int32 I=0;I<Candidate.Num();++I)
        if (!Candidate[I].bDefeated) Candidate[I].Unit.Presence=I==Next?EOGCombatPresence::Active:EOGCombatPresence::Reserve;
    Slots=MoveTemp(Candidate); ControlledSlot=Next; LastBattleReceipt=Receipt; LastSyncedBattle=State.BattleId;
    if (Previous!=Next) OnControlTransferRequested(Previous,Next);
    return true;
}
