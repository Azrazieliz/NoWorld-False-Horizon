#include "Runtime/OGGate12RulerSessionSubsystem.h"

#include "Combat/OGDiagnosticCombatComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Combat/OGWorldActionRuntime.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "World/OGOwnedEquipmentService.h"
#include "Objects/OGObjectActionService.h"
#include "Content/OGDefinitionLibrary.h"

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
    if (auto* Library=UOGDefinitionLibrary::Get())
        for (auto& Row:Out.Items)
            if (const auto* Definition=Cast<UOGObjectDefinition>(Library->Resolve(TEXT("object"),Row.Item.DefinitionId)))
                if (!Definition->DisplayName.IsEmpty()) Row.DisplayName=Definition->DisplayName.ToString();
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

bool UOGGate12RulerSessionSubsystem::ReadOwnedObjectActions(const FOGEntityId& SelectedItem,
    const FOGEntityId& Counterparty,TArray<FOGObjectActionPreview>& Out,FString& Error)
{
    Out.Reset(); Error.Reset(); UOGGameCoreSubsystem* Core=nullptr; IOGWorldStore* Store=nullptr;
    if (!ResolveStore(Core,Store,Error) || !bRulerValidated) return false;
    auto* Library=UOGDefinitionLibrary::Get(); if (!Library) { Error=TEXT("Object actions are unavailable."); return false; }
    FName CounterpartyKind;
    if (Counterparty.IsValid())
    {
        bool Found=false; FString State; int64 Revision=0;
        if (!Store->TryReadEntity(Counterparty,Found,CounterpartyKind,State,Revision,Error)) return false;
        if (!Found) { Error=TEXT("This counterparty is no longer available."); return false; }
    }
    FOGItemInstanceRecord Selected; bool HasSelected=false;
    if (SelectedItem.IsValid() && !Store->TryReadItemInstance(SelectedItem,HasSelected,Selected,Error)) return false;
    TArray<UDataAsset*> Actions; Library->List(TEXT("object_action"),UOGObjectActionDefinition::StaticClass(),Actions);
    for (auto* Asset:Actions)
    {
        const auto* Definition=CastChecked<UOGObjectActionDefinition>(Asset);
        if (Definition->UsesCounterparty()!=Counterparty.IsValid() ||
            (Counterparty.IsValid() && Definition->CounterpartyEntityKind!=CounterpartyKind)) continue;
        if (!Counterparty.IsValid() && Definition->Kind==EOGObjectActionKind::Use && (!HasSelected ||
            !Definition->Inputs.ContainsByPredicate([&](const auto& Input)
            { return Input.From==EOGObjectActionParty::Ruler && Input.Disposition==EOGObjectInputDisposition::Consume && Input.ObjectId==Selected.DefinitionId; }))) continue;
        if (!Counterparty.IsValid() && Definition->Kind==EOGObjectActionKind::Repair && (!HasSelected || Selected.DefinitionId!=Definition->SubjectObjectId)) continue;
        FOGObjectActionPreview Review;
        if (!ReviewOwnedObjectAction(Definition->DefinitionId,SelectedItem,Counterparty,Review,Error)) return false;
        if (Definition->UsesCounterparty() && Review.OfferStateJson.IsEmpty()) continue;
        Out.Add(MoveTemp(Review));
    }
    Out.Sort([](const auto& A,const auto& B)
    { return A.DisplayName.ToString()==B.DisplayName.ToString()?A.ActionId.ToString()<B.ActionId.ToString():A.DisplayName.ToString()<B.DisplayName.ToString(); });
    return true;
}
bool UOGGate12RulerSessionSubsystem::ReviewOwnedObjectAction(const FOGContentId& ActionId,
    const FOGEntityId& SelectedItem,const FOGEntityId& Counterparty,FOGObjectActionPreview& Out,FString& Error)
{
    Out=FOGObjectActionPreview(); Error.Reset(); UOGGameCoreSubsystem* Core=nullptr; IOGWorldStore* Store=nullptr;
    if (!ResolveStore(Core,Store,Error) || !bRulerValidated) return false;
    auto* Library=UOGDefinitionLibrary::Get();
    const auto* Definition=Library?Cast<UOGObjectActionDefinition>(Library->Resolve(TEXT("object_action"),ActionId)):nullptr;
    if (!Definition) { Error=TEXT("This object action is unavailable."); return false; }
    FOGObjectActionContext Context; Context.RulerId=BoundRulerId; Context.CounterpartyId=Counterparty; Context.SelectedItemId=SelectedItem;
    if (!FOGObjectActionService(*Store).Preview(*Definition,Context,Core->GetCanonicalWorldTick(),Out,Error)) return false;
    if (OwnedInventoryBattleLocked(*Core,GetWorld()))
    { Out.bCanApply=false; Out.MissingRequirements.Add(TEXT("Finish the active battle or action first.")); }
    return true;
}
bool UOGGate12RulerSessionSubsystem::ConfirmOwnedObjectAction(const FOGObjectActionPreview& Reviewed,
    const FOGEntityId& RequestId,FString& Error)
{
    Error.Reset(); UOGGameCoreSubsystem* Core=nullptr; IOGWorldStore* Store=nullptr;
    if (!ResolveStore(Core,Store,Error) || !bRulerValidated || Reviewed.Context.RulerId!=BoundRulerId)
    { if (Error.IsEmpty()) Error=TEXT("The active Ruler changed. Review this action again."); return false; }
    if (OwnedInventoryBattleLocked(*Core,GetWorld()))
    { Error=TEXT("Finish the active battle or action first."); return false; }
    auto* Library=UOGDefinitionLibrary::Get();
    const auto* Definition=Library?Cast<UOGObjectActionDefinition>(Library->Resolve(TEXT("object_action"),Reviewed.ActionId)):nullptr;
    if (!Definition) { Error=TEXT("This object action is unavailable."); return false; }
    bool Already=false; TArray<FOGEntityId> Created;
    if (!FOGObjectActionService(*Store).Apply(*Definition,Reviewed,RequestId,Core->GetCanonicalWorldTick(),Already,Created,Error)) return false;
    TArray<FString> Warnings;
    if (auto* World=GetWorld())
        for (TActorIterator<AActor> It(World);It;++It)
        {
            TArray<UOGWorldPartyRuntimeComponent*> Parties; It->GetComponents<UOGWorldPartyRuntimeComponent>(Parties);
            for (auto* Party:Parties)
            { FString Warning; if (IsValid(Party) && !Party->RefreshEquipmentStats(Warning)) Warnings.Add(Warning); }
        }
    FString Warning; if (!RefreshCanonicalProjections(Warning)) Warnings.Add(Warning);
    Error=Warnings.IsEmpty()?FString():TEXT("Action saved. Some views could not refresh: ")+FString::Join(Warnings,TEXT(" "));
    return true;
}
