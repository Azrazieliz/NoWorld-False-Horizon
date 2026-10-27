#include "Runtime/OGGate12RulerSessionSubsystem.h"

#include "Combat/OGDiagnosticCombatComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGWorldActionRuntime.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "World/OGOwnedEquipmentService.h"

namespace
{
bool OwnedInventoryBattleLocked(const UOGGameCoreSubsystem& Core, UWorld* World)
{
    const FOGTurnBattle* Battle = Core.GetOwnedLuminousTurnBattle();
    if (Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running) return true;
    if (!World) return false;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        TArray<UOGDiagnosticCombatComponent*> Combats;
        It->GetComponents<UOGDiagnosticCombatComponent>(Combats);
        for (const auto* Combat : Combats)
            if (IsValid(Combat) && Combat->GetWorld() == World &&
                (Combat->IsInTurn() || Combat->GetTurnState().Status == EOGTurnBattleStatus::Running))
                return true;
        TArray<UOGWorldActionRuntimeComponent*> Actions;
        It->GetComponents<UOGWorldActionRuntimeComponent>(Actions);
        for (const auto* Action : Actions)
            if (IsValid(Action) && Action->GetWorld() == World && Action->HasActiveAction()) return true;
    }
    return false;
}
}

bool UOGGate12RulerSessionSubsystem::ReadOwnedInventory(
    FOGOwnedInventorySnapshot& Out, FString& OutError)
{
    Out = FOGOwnedInventorySnapshot();
    OutError.Reset();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError)) return false;
    if (!bRulerValidated || !BoundRulerId.IsValid())
    { OutError = TEXT("Select a stored Ruler to browse owned inventory."); return false; }
    FOGEquipmentCatalog Catalog;
    FString CatalogError;
    const bool bRulesAvailable = FOGEquipmentCatalog::LoadInstalled(Catalog, CatalogError);
    if (!FOGOwnedEquipmentService(*Store).ReadInventory(BoundRulerId,
            SelectedIdentityId, SelectedManifestationId, Catalog, Out, OutError)) return false;
    if (!bRulesAvailable)
        Out.Availability = TEXT("Equipment changes unavailable: ") + CatalogError;
    else if (OwnedInventoryBattleLocked(*Core, GetWorld()))
        Out.Availability = TEXT("Finish the active battle or action before changing equipment.");
    return true;
}

bool UOGGate12RulerSessionSubsystem::ReviewOwnedEquipment(
    const FOGContentId& SlotId, const FOGEntityId& ItemId, bool bUnequip,
    FOGOwnedEquipmentPreview& Out, FString& OutError)
{
    Out = FOGOwnedEquipmentPreview();
    OutError.Reset();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError)) return false;
    if (!bRulerValidated || !BoundRulerId.IsValid())
    { OutError = TEXT("Select a stored Ruler before reviewing equipment."); return false; }
    FOGEquipmentCatalog Catalog;
    if (!FOGEquipmentCatalog::LoadInstalled(Catalog, OutError) ||
        !FOGOwnedEquipmentService(*Store).Preview(BoundRulerId, SelectedIdentityId,
            SelectedManifestationId, SlotId, ItemId, bUnequip, Catalog, Out, OutError)) return false;
    if (OwnedInventoryBattleLocked(*Core, GetWorld()))
    {
        Out.bCanApply = false;
        Out.MissingRequirements.Add(TEXT("Finish the active battle or action before changing equipment."));
    }
    return true;
}

bool UOGGate12RulerSessionSubsystem::ConfirmOwnedEquipment(
    const FOGOwnedEquipmentPreview& Reviewed, const FOGEntityId& RequestId, FString& OutError)
{
    OutError.Reset();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError)) return false;
    if (!bRulerValidated || Reviewed.RulerId != BoundRulerId ||
        Reviewed.Target.OwningRulerId != BoundRulerId ||
        Reviewed.Target.IdentityId != SelectedIdentityId ||
        Reviewed.Target.ManifestationId != SelectedManifestationId)
    { OutError = TEXT("Character selection changed. Review equipment for the selected owned copy again."); return false; }
    if (OwnedInventoryBattleLocked(*Core, GetWorld()))
    { OutError = TEXT("Equipment cannot change during an active battle or action."); return false; }
    FOGEquipmentCatalog Catalog;
    if (!FOGEquipmentCatalog::LoadInstalled(Catalog, OutError)) return false;
    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < 0) { OutError = TEXT("Equipment changes require canonical world time."); return false; }
    bool bAlreadyApplied = false;
    if (!FOGOwnedEquipmentService(*Store).Apply(Reviewed, Catalog, RequestId,
            Tick, bAlreadyApplied, OutError)) return false;

    // Canonical commit is the success boundary. Refresh failures are warnings,
    // with no repeated inventory command, resource deduction or item recreation.
    TArray<FString> Warnings;
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            TArray<UOGDiagnosticCombatComponent*> Combats;
            It->GetComponents<UOGDiagnosticCombatComponent>(Combats);
            for (auto* Combat : Combats)
            {
                FString Error;
                if (IsValid(Combat) && Combat->GetWorld() == World &&
                    !Combat->RefreshCanonicalPartyProjection(Error)) Warnings.Add(Error);
            }
        }
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            TArray<UOGWorldPartyRuntimeComponent*> Parties;
            It->GetComponents<UOGWorldPartyRuntimeComponent>(Parties);
            for (auto* Party : Parties)
            {
                FString Error;
                if (IsValid(Party) && Party->GetWorld() == World &&
                    !Party->RefreshEquipmentStats(Error)) Warnings.Add(Error);
            }
        }
    }
    FString RefreshError;
    if (!RefreshCanonicalProjections(RefreshError)) Warnings.Add(RefreshError);
    OutError.Reset();
    if (!Warnings.IsEmpty())
    {
        OutError = TEXT("Equipment change saved. Some live views could not refresh: ") +
            FString::Join(Warnings, TEXT(" "));
        UE_LOG(LogTemp, Warning, TEXT("%s"), *OutError);
    }
    return true;
}
