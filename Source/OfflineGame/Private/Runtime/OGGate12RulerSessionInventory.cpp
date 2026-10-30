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
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Runtime/OGCanonicalCharacterPresentation.h"
#include "Templates/UnrealTemplate.h"

namespace
{
bool InventoryUnit(const FOGCombatUnitState& Unit, const FOGEntityId& Ruler,
    const TArray<FOGCharacterManifestationRecord>& Targets)
{
    return Unit.UnitEntityId == Ruler || Targets.ContainsByPredicate([&](const auto& M)
    { return Unit.UnitEntityId == M.ManifestationId && Unit.IdentityId == M.IdentityId &&
        Unit.SkillSet.CurrentForm == M.ActiveVersionId; });
}
bool InventoryProjectionBody(const FOGFoundationCharacterProjection& P, const FOGEntityId& Ruler,
    const TArray<FOGCharacterManifestationRecord>& Targets)
{
    if (!P.bHasManifestation) return P.EntityKind == FName(TEXT("ruler")) &&
        P.Context.EntityId == Ruler && P.Context.OwnerEntityId == Ruler;
    return P.Context.EntityId == P.Context.ManifestationId &&
        P.Manifestation.ManifestationId == P.Context.ManifestationId &&
        P.Context.OwnerEntityId == Ruler && P.Manifestation.OwningRulerId == Ruler &&
        P.Manifestation.LifecycleState == TEXT("active") &&
        Targets.ContainsByPredicate([&](const auto& M)
        { return M.ManifestationId == P.Context.ManifestationId && M.IdentityId == P.Manifestation.IdentityId &&
            M.ActiveVersionId == P.Manifestation.ActiveVersionId; });
}
bool InventoryBody(const AActor& Actor, const FOGEntityId& Ruler,
    const TArray<FOGCharacterManifestationRecord>& Targets)
{
    TArray<UOGCanonicalCharacterPresentation*> Presentations;
    Actor.GetComponents<UOGCanonicalCharacterPresentation>(Presentations);
    for (const auto* Presentation : Presentations)
    {
        if (!IsValid(Presentation) || !Presentation->HasProjection()) continue;
        const auto& P = Presentation->GetProjection();
        if (InventoryProjectionBody(P, Ruler, Targets)) return true;
    }
    TArray<UOGCharacterAppearanceComponent*> Appearances;
    Actor.GetComponents<UOGCharacterAppearanceComponent>(Appearances);
    for (const auto* Appearance : Appearances)
        if (IsValid(Appearance))
            for (const auto& M : Targets)
                if (Appearance->MatchesOwnedManifestation(Ruler, M.ManifestationId)) return true;
    return false;
}
bool OwnedInventoryBattleLocked(const UOGGameCoreSubsystem& Core, UWorld* World,
    const FOGEntityId& Ruler, const TArray<FOGCharacterManifestationRecord>& Targets)
{
    const auto* Battle = Core.GetOwnedLuminousTurnBattle();
    if (Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running &&
        (Core.GetOwnedBattleRulerId() == Ruler ||
            Battle->GetState().Units.ContainsByPredicate([&](const auto& U) { return InventoryUnit(U, Ruler, Targets); }))) return true;
    if (!World) return false;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        TArray<UOGDiagnosticCombatComponent*> Combats;
        It->GetComponents<UOGDiagnosticCombatComponent>(Combats);
        for (const auto* Combat : Combats)
            if (IsValid(Combat) && Combat->GetWorld() == World &&
                (Combat->IsInTurn() || Combat->GetTurnState().Status == EOGTurnBattleStatus::Running) &&
                (Combat->GetRulerId() == Ruler || Combat->GetTurnState().Units.ContainsByPredicate(
                    [&](const auto& U) { return InventoryUnit(U, Ruler, Targets); }))) return true;
        if (!InventoryBody(**It, Ruler, Targets)) continue;
        TArray<UOGWorldActionRuntimeComponent*> Actions;
        It->GetComponents<UOGWorldActionRuntimeComponent>(Actions);
        for (const auto* Action : Actions)
            if (IsValid(Action) && Action->GetWorld() == World && Action->HasActiveAction()) return true;
    }
    return false;
}
bool ObjectActionTargets(IOGWorldStore& Store, const FOGObjectActionPreview& Review,
    const FOGEntityId& Selected, TArray<FOGCharacterManifestationRecord>& Out, FString& Error)
{
    TArray<FOGCharacterManifestationRecord> Copies;
    if (!Store.ListCharacterManifestationsByOwner(Review.Context.RulerId, Copies, Error)) return false;
    for (const auto& M : Copies)
    {
        if (M.OwningRulerId != Review.Context.RulerId || M.LifecycleState != TEXT("active")) continue;
        bool Affected = M.ManifestationId == Selected || M.ManifestationId == Review.SubjectBefore.OwnerEntityId ||
            Review.Inputs.ContainsByPredicate([&](const auto& I) { return I.Item.OwnerEntityId == M.ManifestationId; });
        TArray<FOGEquipmentBindingRecord> Bindings;
        if (!Store.ListEquipmentBindings(M.ManifestationId, Bindings, Error)) return false;
        Affected |= Bindings.ContainsByPredicate([&](const auto& B)
        { return B.ItemId == Review.Context.SelectedItemId || Review.Inputs.ContainsByPredicate(
            [&](const auto& I) { return I.Item.ItemId == B.ItemId; }); });
        if (Affected) Out.Add(M);
    }
    return true;
}
void RefreshInventoryBodies(UWorld* World, const FOGEntityId& Ruler,
    const TArray<FOGCharacterManifestationRecord>& Targets, TArray<FString>& Warnings)
{
    if (!World) return;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        TArray<UOGDiagnosticCombatComponent*> Combats;
        It->GetComponents<UOGDiagnosticCombatComponent>(Combats);
        for (auto* Combat : Combats)
        {
            if (!IsValid(Combat) || Combat->GetWorld() != World ||
                (Combat->GetRulerId() != Ruler && !Combat->GetTurnState().Units.ContainsByPredicate(
                    [&](const auto& U) { return InventoryUnit(U, Ruler, Targets); }))) continue;
            FString Warning;
            if (!Combat->RefreshCanonicalPartyProjection(Warning)) Warnings.Add(Warning);
        }
        TArray<UOGWorldPartyRuntimeComponent*> Parties;
        It->GetComponents<UOGWorldPartyRuntimeComponent>(Parties);
        for (auto* Party : Parties)
        {
            if (!IsValid(Party) || Party->GetWorld() != World ||
                !Party->GetSlots().ContainsByPredicate([&](const auto& S) { return InventoryUnit(S.Unit, Ruler, Targets); })) continue;
            FString Warning;
            if (!Party->RefreshEquipmentStats(Warning)) Warnings.Add(Warning);
        }
        if (!InventoryBody(**It, Ruler, Targets)) continue;
        TArray<UOGCanonicalCharacterPresentation*> Presentations;
        It->GetComponents<UOGCanonicalCharacterPresentation>(Presentations);
        for (auto* Presentation : Presentations)
        {
            FString Warning;
            if (IsValid(Presentation) && Presentation->HasProjection() &&
                InventoryProjectionBody(Presentation->GetProjection(), Ruler, Targets) && !Presentation->Refresh(Warning)) Warnings.Add(Warning);
        }
    }
}
bool ResolveInventoryStats(UWorld* World, const FOGOwnedEquipmentPreview& Review,
    FOGCombatUnitState& Out)
{
    if (!World) return false;
    // Only an existing resolved target supplies a base. No default kit/stat curve.
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        TArray<UOGWorldPartyRuntimeComponent*> Parties;
        It->GetComponents<UOGWorldPartyRuntimeComponent>(Parties);
        for (const auto* Party : Parties)
        {
            if (!IsValid(Party) || Party->GetWorld() != World) continue;
            for (const auto& Slot : Party->GetSlots())
                if (Slot.Unit.UnitEntityId == Review.Target.ManifestationId &&
                    Slot.Unit.IdentityId == Review.Target.IdentityId &&
                    Slot.Unit.SkillSet.CurrentForm == Review.Target.ActiveVersionId)
                { Out = Slot.Unit; return true; }
        }
    }
    return false;
}
FString InventoryObjectName(const FOGContentId& Id, const FString& CatalogName)
{
    if (auto* Library = UOGDefinitionLibrary::Get())
        if (const auto* Definition = Cast<UOGObjectDefinition>(Library->Resolve(TEXT("object"), Id)))
            if (!Definition->DisplayName.IsEmpty()) return Definition->DisplayName.ToString();
    return CatalogName.IsEmpty() || CatalogName == Id.ToString() ? TEXT("Unnamed stored item") : CatalogName;
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
    for (auto& Row : Out.Items)
        Row.DisplayName = InventoryObjectName(Row.Item.DefinitionId, Row.DisplayName);
    if (!bRulesAvailable)
        Out.Availability = TEXT("Equipment changes unavailable: ") + CatalogError;
    else if (OwnedInventoryBattleLocked(*Core, GetWorld(), BoundRulerId,
        Out.bHasTarget ? TArray<FOGCharacterManifestationRecord>{Out.Target} : TArray<FOGCharacterManifestationRecord>{}))
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
    Out.ItemName = InventoryObjectName(Out.Item.DefinitionId, Out.ItemName);
    if (Out.bHasPreviousItem) Out.PreviousItemName = InventoryObjectName(Out.PreviousItem.DefinitionId, Out.PreviousItemName);
    FOGCombatUnitState Resolved;
    FString StatsError;
    if (!ResolveInventoryStats(GetWorld(), Out, Resolved))
        Out.StatsAvailability = TEXT("Total stats are unavailable until this owned copy has a resolved world projection.");
    else if (!FOGOwnedEquipmentService(*Store).PreviewStats(Out, Resolved, Catalog, Out, StatsError))
        Out.StatsAvailability = StatsError;
    if (OwnedInventoryBattleLocked(*Core, GetWorld(), BoundRulerId, {Out.Target}))
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
    if (bInventoryCommandInFlight) { OutError = TEXT("An inventory confirmation is already in progress."); return false; }
    TGuardValue<bool> InFlight(bInventoryCommandInFlight, true);
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError)) return false;
    if (!bRulerValidated || Reviewed.RulerId != BoundRulerId ||
        Reviewed.Target.OwningRulerId != BoundRulerId ||
        Reviewed.Target.IdentityId != SelectedIdentityId ||
        Reviewed.Target.ManifestationId != SelectedManifestationId)
    { OutError = TEXT("Character selection changed. Review equipment for the selected owned copy again."); return false; }
    if (OwnedInventoryBattleLocked(*Core, GetWorld(), BoundRulerId, {Reviewed.Target}))
    { OutError = TEXT("Equipment cannot change during an active battle or action."); return false; }
    FOGEquipmentCatalog Catalog;
    if (!FOGEquipmentCatalog::LoadInstalled(Catalog, OutError)) return false;
    if (Reviewed.bStatsKnown)
    {
        // Receipt retries remain the service's authority; only a new command
        // needs the same totals that the player actually reviewed.
        bool HasReceipt = false; FOGWorldEvent Receipt;
        if (!Store->TryReadWorldEvent(RequestId, HasReceipt, Receipt, OutError)) return false;
        if (!HasReceipt)
        {
            FOGOwnedEquipmentPreview Fresh;
            if (!ReviewOwnedEquipment(Reviewed.SlotId, Reviewed.Item.ItemId, Reviewed.bUnequip, Fresh, OutError)) return false;
            if (!Fresh.bStatsKnown || Fresh.StatPreviewToken != Reviewed.StatPreviewToken)
            { OutError = TEXT("Resolved equipment totals changed. Review the current stats again."); return false; }
        }
    }
    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < 0) { OutError = TEXT("Equipment changes require canonical world time."); return false; }
    bool bAlreadyApplied = false;
    if (!FOGOwnedEquipmentService(*Store).Apply(Reviewed, Catalog, RequestId,
            Tick, bAlreadyApplied, OutError)) return false;

    // Canonical commit is the success boundary. Refresh failures are warnings,
    // with no repeated inventory command, resource deduction or item recreation.
    TArray<FString> Warnings;
    RefreshInventoryBodies(GetWorld(), BoundRulerId, {Reviewed.Target}, Warnings);
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
    TArray<FOGCharacterManifestationRecord> Targets;
    if (!ObjectActionTargets(*Store, Out, SelectedManifestationId, Targets, Error)) return false;
    if (OwnedInventoryBattleLocked(*Core,GetWorld(),BoundRulerId,Targets))
    { Out.bCanApply=false; Out.MissingRequirements.Add(TEXT("Finish the active battle or action first.")); }
    return true;
}
bool UOGGate12RulerSessionSubsystem::ConfirmOwnedObjectAction(const FOGObjectActionPreview& Reviewed,
    const FOGEntityId& RequestId,FString& Error)
{
    Error.Reset();
    if (bInventoryCommandInFlight) { Error=TEXT("An inventory confirmation is already in progress."); return false; }
    TGuardValue<bool> InFlight(bInventoryCommandInFlight,true);
    UOGGameCoreSubsystem* Core=nullptr; IOGWorldStore* Store=nullptr;
    if (!ResolveStore(Core,Store,Error) || !bRulerValidated || Reviewed.Context.RulerId!=BoundRulerId)
    { if (Error.IsEmpty()) Error=TEXT("The active Ruler changed. Review this action again."); return false; }
    auto* Library=UOGDefinitionLibrary::Get();
    const auto* Definition=Library?Cast<UOGObjectActionDefinition>(Library->Resolve(TEXT("object_action"),Reviewed.ActionId)):nullptr;
    if (!Definition) { Error=TEXT("This object action is unavailable."); return false; }
    // Derive affected bodies from canonical inputs, not caller-supplied preview rows.
    // A receipt retry still goes through Apply's exact receipt validation.
    bool HasReceipt=false; FOGWorldEvent Receipt;
    if (!Store->TryReadWorldEvent(RequestId,HasReceipt,Receipt,Error)) return false;
    FOGObjectActionPreview Actual=Reviewed;
    if (!HasReceipt)
    {
        if (!FOGObjectActionService(*Store).Preview(*Definition,Reviewed.Context,Core->GetCanonicalWorldTick(),Actual,Error)) return false;
        if (!Actual.bCanApply || Actual.ConfirmationToken!=Reviewed.ConfirmationToken)
        { Error=TEXT("The action changed. Review the current objects and balances again."); return false; }
    }
    TArray<FOGCharacterManifestationRecord> Targets;
    if (!ObjectActionTargets(*Store,Actual,SelectedManifestationId,Targets,Error)) return false;
    if (OwnedInventoryBattleLocked(*Core,GetWorld(),BoundRulerId,Targets))
    { Error=TEXT("Finish the active battle or action first."); return false; }
    bool Already=false; TArray<FOGEntityId> Created;
    if (!FOGObjectActionService(*Store).Apply(*Definition,Reviewed,RequestId,Core->GetCanonicalWorldTick(),Already,Created,Error)) return false;
    TArray<FString> Warnings;
    RefreshInventoryBodies(GetWorld(),BoundRulerId,Targets,Warnings);
    FString Warning; if (!RefreshCanonicalProjections(Warning)) Warnings.Add(Warning);
    Error=Warnings.IsEmpty()?FString():TEXT("Action saved. Some views could not refresh: ")+FString::Join(Warnings,TEXT(" "));
    return true;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGOwnedInventoryBodyGuardTest,
    "OfflineGame.Inventory.ActionGuardsRequireExactRulerOrTargetProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGOwnedInventoryBodyGuardTest::RunTest(const FString&)
{
    const auto Ruler = FOGEntityId::NewId();
    FOGCharacterManifestationRecord Target;
    Target.ManifestationId = FOGEntityId::NewId();
    Target.OwningRulerId = Ruler;
    Target.IdentityId = FOGContentId(TEXT("test:inventory.identity"));
    Target.ActiveVersionId = FOGContentId(TEXT("test:inventory.version"));
    Target.LifecycleState = TEXT("active");
    const TArray<FOGCharacterManifestationRecord> Targets = {Target};
    FOGFoundationCharacterProjection P;
    P.EntityKind = TEXT("npc");
    P.Context.EntityId = FOGEntityId::NewId();
    P.Context.OwnerEntityId = Ruler;
    TestFalse(TEXT("An unrelated NPC does not become the Ruler body through ownership"), InventoryProjectionBody(P, Ruler, Targets));
    P.EntityKind = TEXT("ruler");
    P.Context.EntityId = Ruler;
    TestTrue(TEXT("Exact canonical Ruler body is guarded"), InventoryProjectionBody(P, Ruler, Targets));
    P.bHasManifestation = true;
    P.Manifestation = Target;
    P.Context.EntityId = Target.ManifestationId;
    P.Context.ManifestationId = Target.ManifestationId;
    TestTrue(TEXT("Exact owned target body is guarded"), InventoryProjectionBody(P, Ruler, Targets));
    P.Manifestation.ActiveVersionId = FOGContentId(TEXT("test:inventory.other_version"));
    TestFalse(TEXT("A stale version is not the selected actual body"), InventoryProjectionBody(P, Ruler, Targets));
    P.Manifestation = Target;
    P.Context.OwnerEntityId = FOGEntityId::NewId();
    TestFalse(TEXT("A foreign body is not guarded as this Ruler"), InventoryProjectionBody(P, Ruler, Targets));
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = FOGEntityId::NewId();
    Unit.IdentityId = Target.IdentityId;
    Unit.SkillSet.CurrentForm = Target.ActiveVersionId;
    TestFalse(TEXT("Another copy of the same identity cannot lock this target"), InventoryUnit(Unit, Ruler, Targets));
    Unit.UnitEntityId = Target.ManifestationId;
    TestTrue(TEXT("Actual target battle participant is guarded"), InventoryUnit(Unit, Ruler, Targets));
    Unit.UnitEntityId = Ruler;
    TestTrue(TEXT("Actual Ruler battle participant is guarded"), InventoryUnit(Unit, Ruler, Targets));
    return true;
}
#endif
