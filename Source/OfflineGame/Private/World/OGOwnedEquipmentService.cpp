#include "World/OGOwnedEquipmentService.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Runtime/OGFoundationCharacterRuntime.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "World/OGInventoryEquipmentService.h"

namespace
{
const FName OwnedEquipmentEvent(TEXT("equipment_changed"));
void EquipmentQuotePart(FString& Text, const FString& Part)
{
    Text += LexToString(Part.Len()) + TEXT(":") + Part;
}
void EquipmentItemQuote(FString& Text, const FOGItemInstanceRecord& I)
{
    for (const auto& Part : {I.ItemId.ToString(), I.OwnerEntityId.ToString(), I.DefinitionId.Value,
        I.CurrentRankId.Value, I.QualityId.Value, I.DurabilityStateJson, I.EvolutionStateJson, I.HistoryStateJson})
        EquipmentQuotePart(Text, Part);
}
bool EquipmentObject(const FString& Json, TSharedPtr<FJsonObject>& Out)
{
    return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out) && Out.IsValid();
}
bool EquipmentUsable(const FOGItemInstanceRecord& Item)
{
    // Same stored condition semantics as FoundationCharacterRuntime::Equip.
    TSharedPtr<FJsonObject> State;
    if (!EquipmentObject(Item.DurabilityStateJson, State)) return false;
    bool bBroken = false;
    double Condition = 10000;
    if ((State->HasField(TEXT("broken")) && !State->TryGetBoolField(TEXT("broken"), bBroken)) ||
        (State->HasField(TEXT("condition_bps")) && !State->TryGetNumberField(TEXT("condition_bps"), Condition)))
        return false;
    return !bBroken && FMath::IsFinite(Condition) && Condition > 0 && Condition <= 10000;
}
FOGFoundationCharacterContext EquipmentContext(const FOGCharacterManifestationRecord& M)
{
    FOGFoundationCharacterContext C;
    C.EntityId = M.ManifestationId;
    C.ManifestationId = M.ManifestationId;
    C.OwnerEntityId = M.OwningRulerId;
    return C;
}
bool AddEquipmentBonuses(FOGCombatStats& Stats, const TMap<FName, FOGLargeNumber>& Bonuses,
    FString& Error)
{
    FOGCombatStats Result = Stats;
    for (const auto& Bonus : Bonuses)
    {
        if (Bonus.Key == TEXT("max_hp")) Result.MaxHp = FOGLargeNumber::Add(Result.MaxHp, Bonus.Value);
        else if (Bonus.Key == TEXT("attack")) Result.Attack = FOGLargeNumber::Add(Result.Attack, Bonus.Value);
        else if (Bonus.Key == TEXT("defense")) Result.Defense = FOGLargeNumber::Add(Result.Defense, Bonus.Value);
        else if (Bonus.Key == TEXT("speed"))
        {
            int64 Amount = Bonus.Value.Significand;
            if (Amount != 0)
            {
                if (Bonus.Value.Exponent10 < -9 || Bonus.Value.Exponent10 > 9)
                { Error = TEXT("Equipment speed contribution is out of range."); return false; }
                for (int32 I = 0; I < Bonus.Value.Exponent10; ++I)
                {
                    if (Amount > MAX_int32 / 10)
                    { Error = TEXT("Equipment speed contribution overflows."); return false; }
                    Amount *= 10;
                }
                for (int32 I = 0; I > Bonus.Value.Exponent10; --I)
                {
                    if (Amount % 10) { Error = TEXT("Equipment speed must be integral."); return false; }
                    Amount /= 10;
                }
            }
            if (Amount < 0 || Amount > MAX_int32 ||
                static_cast<int64>(Result.Speed) + Amount > MAX_int32)
            { Error = TEXT("Equipment speed contribution overflows resolved stats."); return false; }
            Result.Speed += static_cast<int32>(Amount);
        }
        else { Error = TEXT("Equipment stat needs an approved native resolver."); return false; }
    }
    Stats = Result;
    return true;
}
}

bool FOGOwnedEquipmentService::ValidateRuler(const FOGEntityId& Ruler, FString& Error) const
{
    bool bFound = false;
    FName Kind;
    FString State;
    int64 Revision = 0;
    if (!Store.IsOpen() || !Ruler.IsValid())
    { Error = TEXT("A stored Ruler is required for inventory."); return false; }
    if (!Store.TryReadEntity(Ruler, bFound, Kind, State, Revision, Error)) return false;
    if (!bFound || Kind != TEXT("ruler") || State.Contains(TEXT("\"diagnosticFixture\"")))
    { Error = TEXT("Inventory owner is not a canonical Ruler."); return false; }
    return true;
}

bool FOGOwnedEquipmentService::ReadTarget(const FOGEntityId& Ruler,
    const FOGContentId& Identity, const FOGEntityId& Manifestation,
    FOGCharacterManifestationRecord& Out, FString& Error) const
{
    bool bFound = false;
    if (!Identity.IsValid() || !Manifestation.IsValid() || Identity.Value.StartsWith(TEXT("diagnostic:")))
    { Error = TEXT("Select an actually owned character before changing equipment."); return false; }
    if (!Store.TryReadCharacterManifestation(Manifestation, bFound, Out, Error)) return false;
    if (!bFound || Out.ManifestationId != Manifestation || Out.IdentityId != Identity ||
        Out.OwningRulerId != Ruler || !Out.ActiveVersionId.IsValid() ||
        Out.ActiveVersionId.Value.StartsWith(TEXT("diagnostic:")) || Out.LifecycleState != TEXT("active"))
    { Error = TEXT("Equipment target is not this Ruler's active exact Identity/Version."); return false; }
    return true;
}

bool FOGOwnedEquipmentService::ReadInventory(const FOGEntityId& Ruler,
    const FOGContentId& Identity, const FOGEntityId& Manifestation,
    const FOGEquipmentCatalog& Catalog, FOGOwnedInventorySnapshot& Out, FString& Error) const
{
    Out = FOGOwnedInventorySnapshot();
    Error.Reset();
    if (!ValidateRuler(Ruler, Error)) return false;
    FOGOwnedInventorySnapshot Snapshot;
    if (Identity.IsValid() || Manifestation.IsValid())
    {
        if (!ReadTarget(Ruler, Identity, Manifestation, Snapshot.Target, Error)) return false;
        Snapshot.bHasTarget = true;
    }
    TArray<FOGCharacterManifestationRecord> Copies;
    if (!Store.ListCharacterManifestationsByOwner(Ruler, Copies, Error)) return false;
    TArray<FOGEntityId> Owners = {Ruler};
    for (const auto& Copy : Copies)
        if (Copy.OwningRulerId == Ruler) Owners.AddUnique(Copy.ManifestationId);
    TSet<FOGEntityId> Seen;
    TArray<FOGEquipmentBindingRecord> AllBindings;
    for (const auto& Owner : Owners)
    {
        TArray<FOGItemInstanceRecord> Items;
        TArray<FOGEquipmentBindingRecord> Bindings;
        if (!Store.ListItemInstancesByOwner(Owner, Items, Error) ||
            !Store.ListEquipmentBindings(Owner, Bindings, Error)) return false;
        AllBindings.Append(Bindings);
        for (const auto& Item : Items)
        {
            if (Item.OwnerEntityId != Owner || !Item.ItemId.IsValid() || Seen.Contains(Item.ItemId))
            { Error = TEXT("Stored inventory ownership is inconsistent."); return false; }
            Seen.Add(Item.ItemId);
            FOGOwnedInventoryItem Row;
            Row.Item = Item;
            Row.DisplayName = Catalog.ItemName(Item.DefinitionId);
            if (!Store.ListItemModifiers(Item.ItemId, Row.Modifiers, Error)) return false;
            Snapshot.Items.Add(MoveTemp(Row));
        }
    }
    for (auto& Item : Snapshot.Items)
        for (const auto& Binding : AllBindings)
            if (Binding.ItemId == Item.Item.ItemId) Item.Bindings.Add(Binding);
    if (Snapshot.bHasTarget)
    {
        for (const auto& Definition : Catalog.Slots)
            if (Definition.IdentityId == Identity && Definition.VersionId == Snapshot.Target.ActiveVersionId)
            {
                FOGOwnedEquipmentSlot Slot;
                Slot.SlotId = Definition.SlotId;
                Slot.DisplayName = Definition.DisplayName;
                Slot.bAuthored = true;
                Snapshot.Slots.Add(MoveTemp(Slot));
            }
        for (const auto& Binding : AllBindings)
            if (Binding.WearerEntityId == Manifestation)
            {
                auto* Slot = Snapshot.Slots.FindByPredicate([&](const auto& S) { return S.SlotId == Binding.SlotId; });
                if (!Slot)
                {
                    FOGOwnedEquipmentSlot Stored;
                    Stored.SlotId = Binding.SlotId;
                    Stored.DisplayName = Binding.SlotId.ToString();
                    Snapshot.Slots.Add(MoveTemp(Stored));
                    Slot = &Snapshot.Slots.Last();
                }
                Slot->bOccupied = true;
                Slot->Binding = Binding;
            }
        if (Snapshot.Slots.IsEmpty())
            Snapshot.Availability = TEXT("No equipment slots are authored for this character's exact version.");
    }
    else Snapshot.Availability = TEXT("Choose an owned Manifestation to inspect equipment slots.");
    Snapshot.Items.Sort([](const auto& A, const auto& B)
    { return A.DisplayName == B.DisplayName ? A.Item.ItemId.ToString() < B.Item.ItemId.ToString() : A.DisplayName < B.DisplayName; });
    Out = MoveTemp(Snapshot);
    return true;
}

bool FOGOwnedEquipmentService::Preview(const FOGEntityId& Ruler,
    const FOGContentId& Identity, const FOGEntityId& Manifestation,
    const FOGContentId& SlotId, const FOGEntityId& ItemId, bool bUnequip,
    const FOGEquipmentCatalog& Catalog, FOGOwnedEquipmentPreview& Out, FString& Error) const
{
    Out = FOGOwnedEquipmentPreview();
    Error.Reset();
    if (!Catalog.Validate(Error) || !ValidateRuler(Ruler, Error) ||
        !ReadTarget(Ruler, Identity, Manifestation, Out.Target, Error)) return false;
    if (!SlotId.IsValid()) { Error = TEXT("A real equipment slot is required."); return false; }
    Out.RulerId = Ruler;
    Out.SlotId = SlotId;
    Out.bUnequip = bUnequip;
    Out.SlotName = SlotId.ToString();
    const auto* Slot = Catalog.Slots.FindByPredicate([&](const auto& S)
    { return S.IdentityId == Identity && S.VersionId == Out.Target.ActiveVersionId && S.SlotId == SlotId; });
    if (Slot) Out.SlotName = Slot->DisplayName;
    bool bFound = false;
    FOGEquipmentBindingRecord Previous;
    if (!Store.TryReadEquipmentBinding(Manifestation, SlotId, bFound, Previous, Error)) return false;
    if (bFound)
    {
        if (!Store.TryReadItemInstance(Previous.ItemId, Out.bHasPreviousItem, Out.PreviousItem, Error)) return false;
        if (!Out.bHasPreviousItem) { Error = TEXT("Equipment references a missing stored item."); return false; }
        if (Out.PreviousItem.OwnerEntityId != Ruler && Out.PreviousItem.OwnerEntityId != Manifestation)
        { Error = TEXT("Stored equipment refers to an item owned by another character."); return false; }
        Out.PreviousItemName = Catalog.ItemName(Out.PreviousItem.DefinitionId);
    }
    if (bUnequip)
    {
        if (!Out.bHasPreviousItem)
        { Error = TEXT("This equipment slot is already empty."); return false; }
        if (ItemId.IsValid() && ItemId != Previous.ItemId)
        { Error = TEXT("The equipped item changed. Review the slot again."); return false; }
        Out.Item = Out.PreviousItem;
    }
    else
    {
        if (!Store.TryReadItemInstance(ItemId, bFound, Out.Item, Error)) return false;
        if (!bFound || Out.Item.ItemId != ItemId)
        { Error = TEXT("This item instance is no longer stored."); return false; }
    }
    if (Out.Item.OwnerEntityId != Ruler && Out.Item.OwnerEntityId != Manifestation)
    { Error = TEXT("This item belongs to another owner or Manifestation."); return false; }
    Out.ItemName = Catalog.ItemName(Out.Item.DefinitionId);
    TArray<FOGItemModifierRecord> Modifiers;
    if (!Store.ListItemModifiers(Out.Item.ItemId, Modifiers, Error)) return false;
    if (bUnequip && Modifiers.IsEmpty())
        if (const auto* Rule = Catalog.FindRule(Out.Target, SlotId, Out.Item))
        {
            Out.bEffectsKnown = true;
            if (EquipmentUsable(Out.Item)) Out.StatBonuses = Rule->StatBonuses;
        }
    if (!bUnequip)
    {
        const auto* Rule = Catalog.FindRule(Out.Target, SlotId, Out.Item);
        if (!Slot || !Rule)
            Out.MissingRequirements.Add(TEXT("No approved exact version/slot/item/rank/quality rule is installed."));
        else
        {
            Out.StatBonuses = Rule->StatBonuses;
            Out.bEffectsKnown = true;
        }
        if (!EquipmentUsable(Out.Item))
            Out.MissingRequirements.Add(TEXT("Restore this item's stored condition before equipping."));
        if (!Modifiers.IsEmpty())
            Out.MissingRequirements.Add(TEXT("This item's modifiers require an approved equipment resolver."));
        FOGOwnedInventorySnapshot Inventory;
        if (!ReadInventory(Ruler, Identity, Manifestation, Catalog, Inventory, Error)) return false;
        const auto* Candidate = Inventory.Items.FindByPredicate([&](const auto& I) { return I.Item.ItemId == ItemId; });
        if (Candidate && !Candidate->Bindings.IsEmpty())
            Out.MissingRequirements.Add(TEXT("Unequip this physical item from its current slot before equipping it here."));
    }
    FString Quote = Catalog.Fingerprint();
    for (const auto& Part : {Ruler.ToString(), Manifestation.ToString(), Identity.Value,
        Out.Target.ActiveVersionId.Value, Out.Target.LifecycleState.ToString(),
        LexToString(Out.Target.Level), Out.Target.ProgressionStateJson, SlotId.Value,
        bUnequip ? FString(TEXT("unequip")) : FString(TEXT("equip")),
        Out.bHasPreviousItem ? FString(TEXT("occupied")) : FString(TEXT("empty")), Previous.StateJson})
        EquipmentQuotePart(Quote, Part);
    EquipmentItemQuote(Quote, Out.Item);
    if (Out.bHasPreviousItem) EquipmentItemQuote(Quote, Out.PreviousItem);
    for (const auto& Modifier : Modifiers)
        for (const auto& Part : {Modifier.ModifierId.Value, LexToString(Modifier.Ordinal), Modifier.StateJson})
            EquipmentQuotePart(Quote, Part);
    Out.ConfirmationToken = FMD5::HashAnsiString(*Quote);
    Out.bCanApply = Out.MissingRequirements.IsEmpty();
    return true;
}

bool FOGOwnedEquipmentService::Apply(const FOGOwnedEquipmentPreview& Reviewed,
    const FOGEquipmentCatalog& Catalog, const FOGEntityId& RequestId,
    int64 Tick, bool& bAlreadyApplied, FString& Error)
{
    Error.Reset();
    bAlreadyApplied = false;
    if (!RequestId.IsValid() || Reviewed.ConfirmationToken.IsEmpty() || Tick < 0 ||
        !Catalog.Validate(Error) || !ValidateRuler(Reviewed.RulerId, Error))
    { if (Error.IsEmpty()) Error = TEXT("Review a valid equipment request before confirming."); return false; }
    if (!Store.BeginTransaction(Error)) return false;
    auto Fail = [&]() { FString Rollback; Store.RollbackTransaction(Rollback); return false; };
    bool bFound = false;
    FOGWorldEvent Existing;
    if (!Store.TryReadWorldEvent(RequestId, bFound, Existing, Error)) return Fail();
    if (bFound)
    {
        TSharedPtr<FJsonObject> Receipt;
        FString Owner, Slot, Item, Token, Action;
        if (Existing.EventType != OwnedEquipmentEvent || Existing.PrimaryEntity != Reviewed.Target.ManifestationId ||
            !EquipmentObject(Existing.PayloadJson, Receipt) ||
            !Receipt->TryGetStringField(TEXT("ruler_id"), Owner) || Owner != Reviewed.RulerId.ToString() ||
            !Receipt->TryGetStringField(TEXT("slot_id"), Slot) || Slot != Reviewed.SlotId.Value ||
            !Receipt->TryGetStringField(TEXT("item_id"), Item) || Item != Reviewed.Item.ItemId.ToString() ||
            !Receipt->TryGetStringField(TEXT("confirmation_token"), Token) || Token != Reviewed.ConfirmationToken ||
            !Receipt->TryGetStringField(TEXT("action"), Action) || Action != (Reviewed.bUnequip ? TEXT("unequip") : TEXT("equip")))
        { Error = TEXT("This request ID belongs to a different canonical action."); return Fail(); }
        if (!Store.CommitTransaction(Error)) return Fail();
        bAlreadyApplied = true;
        return true;
    }
    FOGOwnedEquipmentPreview Fresh;
    if (!Preview(Reviewed.RulerId, Reviewed.Target.IdentityId, Reviewed.Target.ManifestationId,
            Reviewed.SlotId, Reviewed.Item.ItemId, Reviewed.bUnequip, Catalog, Fresh, Error)) return Fail();
    if (!Fresh.bCanApply || Fresh.ConfirmationToken != Reviewed.ConfirmationToken)
    {
        Error = Fresh.bCanApply ? TEXT("Item, equipment or installed rules changed. Review again.") :
            FString::Join(Fresh.MissingRequirements, TEXT("\n"));
        return Fail();
    }
    if (Fresh.bUnequip)
    {
        if (!FOGFoundationCharacterRuntime(Store).Unequip(
                EquipmentContext(Fresh.Target), Fresh.SlotId, Error)) return Fail();
    }
    else
    {
        FOGEquipmentSlotValidator Strict = [&](const FOGEntityId& Wearer,
            const FOGContentId& Slot, const FOGItemInstanceRecord& Item, FString& E)
        {
            if (Wearer != Fresh.Target.ManifestationId || Slot != Fresh.SlotId ||
                !Catalog.FindRule(Fresh.Target, Slot, Item) || !EquipmentUsable(Item))
            { E = TEXT("Installed exact equipment rule no longer admits this item."); return false; }
            return true;
        };
        if (!FOGInventoryEquipmentService(Store).EquipItem(Fresh.Target.ManifestationId,
                Fresh.SlotId, Fresh.Item.ItemId, Strict, Error)) return Fail();
    }
    auto Receipt = MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("ruler_id"), Fresh.RulerId.ToString());
    Receipt->SetStringField(TEXT("slot_id"), Fresh.SlotId.Value);
    Receipt->SetStringField(TEXT("item_id"), Fresh.Item.ItemId.ToString());
    Receipt->SetStringField(TEXT("confirmation_token"), Reviewed.ConfirmationToken);
    Receipt->SetStringField(TEXT("action"), Fresh.bUnequip ? TEXT("unequip") : TEXT("equip"));
    Receipt->SetStringField(TEXT("previous_item_id"), Fresh.bHasPreviousItem ? Fresh.PreviousItem.ItemId.ToString() : TEXT(""));
    FOGWorldEvent Event;
    Event.EventId = RequestId;
    Event.EventType = OwnedEquipmentEvent;
    Event.WorldTick = Tick;
    Event.PrimaryEntity = Fresh.Target.ManifestationId;
    Event.RelatedEntities = {Fresh.RulerId, Fresh.Item.ItemId};
    Event.bChronicleEligible = true;
    FJsonSerializer::Serialize(Receipt.ToSharedRef(), TJsonWriterFactory<>::Create(&Event.PayloadJson));
    if (!Store.AppendWorldEvent(Event, Error) || !Store.CommitTransaction(Error)) return Fail();
    return true;
}

bool FOGOwnedEquipmentService::ProjectCombatUnit(FOGCombatUnitState& Unit,
    const FOGEquipmentCatalog& Catalog, FString& Error) const
{
    Error.Reset();
    bool bFound = false;
    FOGCharacterManifestationRecord M;
    if (!Store.TryReadCharacterManifestation(Unit.UnitEntityId, bFound, M, Error)) return false;
    if (!bFound) return true; // Ruler/NPC mechanics have their own content resolvers.
    if (!Catalog.Validate(Error)) return false;
    if (M.IdentityId != Unit.IdentityId || M.ActiveVersionId != Unit.SkillSet.CurrentForm || M.LifecycleState != TEXT("active"))
    { Error = TEXT("Equipment projection does not match this active character/version."); return false; }
    FOGFoundationEquipmentFunctions Contributions;
    FOGFoundationEquipmentResolver Resolver = [&](const FOGFoundationCharacterProjection&,
        const FOGFoundationEquippedItemProjection& Item, FOGFoundationEquipmentFunctions& Out, FString& E)
    {
        const auto* Rule = Catalog.FindRule(M, Item.Binding.SlotId, Item.Item);
        if (Item.Item.OwnerEntityId != M.OwningRulerId && Item.Item.OwnerEntityId != M.ManifestationId)
        { E = TEXT("Stored equipment is not owned by this character or Ruler."); return false; }
        const bool bManagedDefinition = Catalog.Items.ContainsByPredicate([&](const auto& I)
        { return I.DefinitionId == Item.Item.DefinitionId; });
        // Existing equipment resolvers retain authority over definitions absent
        // from this catalog. Absence never permits a new native equip command.
        if (!bManagedDefinition) return true;
        if (!Rule || !Item.Modifiers.IsEmpty())
        { E = TEXT("Stored equipment mechanics need an approved exact native rule; preserving the live snapshot."); return false; }
        Out.StatContributions = Rule->StatBonuses;
        return true;
    };
    if (!FOGFoundationCharacterRuntime(Store).ProjectEquipmentFunctions(
            EquipmentContext(M), Resolver, Contributions, Error)) return false;
    const FOGCombatStats Base = Unit.bHasEquipmentStatProjection ? Unit.EquipmentBaseStats :
        (Unit.bHasDevelopmentStatProjection ? Unit.DevelopmentBaseStats : Unit.Stats);
    FOGCombatStats Equipped = Base;
    if (!AddEquipmentBonuses(Equipped, Contributions.StatContributions, Error)) return false;
    Unit.EquipmentBaseStats = Base;
    Unit.bHasEquipmentStatProjection = true;
    Unit.Stats = Equipped;
    Unit.bHasDevelopmentStatProjection = false; // reapply earned grants after equipment, exactly once.
    return true;
}

bool FOGOwnedEquipmentService::ProjectCombatUnit(FOGCombatUnitState& Unit, FString& Error) const
{
    bool bFound = false;
    FOGCharacterManifestationRecord M;
    if (!Store.TryReadCharacterManifestation(Unit.UnitEntityId, bFound, M, Error)) return false;
    if (!bFound) return true;
    FOGEquipmentCatalog Catalog;
    return FOGEquipmentCatalog::LoadInstalled(Catalog, Error) && ProjectCombatUnit(Unit, Catalog, Error);
}
