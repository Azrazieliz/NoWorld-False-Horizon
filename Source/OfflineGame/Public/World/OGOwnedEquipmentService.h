#pragma once

#include "CoreMinimal.h"
#include "Combat/OGCombatTypes.h"
#include "Persistence/OGWorldStore.h"

/** Installed content declares exact anatomy/version slots. No default slots. */
struct OFFLINEGAME_API FOGAuthoredEquipmentSlot
{
    FOGContentId IdentityId;
    FOGContentId VersionId;
    FOGContentId SlotId;
    FString DisplayName;
};

struct OFFLINEGAME_API FOGAuthoredEquipmentItem
{
    FOGContentId DefinitionId;
    FString DisplayName;
};

/** Every match is exact, including empty rank/quality. No wildcard admission.
 * v1 supports explicit additive max_hp/attack/defense/speed contributions for
 * unmodified items; other mechanics require their own approved resolver. */
struct OFFLINEGAME_API FOGAuthoredEquipmentRule
{
    FOGContentId IdentityId;
    FOGContentId VersionId;
    FOGContentId SlotId;
    FOGContentId DefinitionId;
    FOGContentId RankId;
    FOGContentId QualityId;
    TMap<FName, FOGLargeNumber> StatBonuses;
};

struct OFFLINEGAME_API FOGEquipmentCatalog
{
    TArray<FOGAuthoredEquipmentSlot> Slots;
    TArray<FOGAuthoredEquipmentItem> Items;
    TArray<FOGAuthoredEquipmentRule> Rules;

    static bool LoadInstalled(FOGEquipmentCatalog& Out, FString& Error);
    static bool Parse(const FString& Json, FOGEquipmentCatalog& Out, FString& Error);
    bool Validate(FString& Error) const;
    FString Fingerprint() const;
    FString ItemName(const FOGContentId& DefinitionId) const;
    const FOGAuthoredEquipmentRule* FindRule(const FOGCharacterManifestationRecord&,
        const FOGContentId& SlotId, const FOGItemInstanceRecord&) const;
};

struct OFFLINEGAME_API FOGOwnedInventoryItem
{
    FOGItemInstanceRecord Item;
    FString DisplayName;
    TArray<FOGItemModifierRecord> Modifiers;
    TArray<FOGEquipmentBindingRecord> Bindings;
};

struct OFFLINEGAME_API FOGOwnedEquipmentSlot
{
    FOGContentId SlotId;
    FString DisplayName;
    bool bAuthored = false;
    bool bOccupied = false;
    FOGEquipmentBindingRecord Binding;
};

struct OFFLINEGAME_API FOGOwnedInventorySnapshot
{
    TArray<FOGOwnedInventoryItem> Items;
    TArray<FOGOwnedEquipmentSlot> Slots;
    bool bHasTarget = false;
    FOGCharacterManifestationRecord Target;
    FString Availability;
};

struct OFFLINEGAME_API FOGOwnedEquipmentPreview
{
    FOGEntityId RulerId;
    FOGCharacterManifestationRecord Target;
    FOGContentId SlotId;
    FString SlotName;
    FOGItemInstanceRecord Item;
    FString ItemName;
    bool bUnequip = false;
    bool bHasPreviousItem = false;
    FOGItemInstanceRecord PreviousItem;
    FString PreviousItemName;
    TMap<FName, FOGLargeNumber> StatBonuses;
    bool bEffectsKnown = false;
    bool bCanApply = false;
    TArray<FString> MissingRequirements;
    FString ConfirmationToken;
};

/** Stateless adapter over the existing inventory/equipment services and shared
 * transactions/events. Reviews do not reserve items or create a second wallet. */
class OFFLINEGAME_API FOGOwnedEquipmentService
{
public:
    explicit FOGOwnedEquipmentService(IOGWorldStore& InStore) : Store(InStore) {}
    bool ReadInventory(const FOGEntityId& RulerId, const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId, const FOGEquipmentCatalog& Catalog,
        FOGOwnedInventorySnapshot& Out, FString& Error) const;
    bool Preview(const FOGEntityId& RulerId, const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId, const FOGContentId& SlotId,
        const FOGEntityId& ItemId, bool bUnequip, const FOGEquipmentCatalog& Catalog,
        FOGOwnedEquipmentPreview& Out, FString& Error) const;
    bool Apply(const FOGOwnedEquipmentPreview& Reviewed,
        const FOGEquipmentCatalog& Catalog, const FOGEntityId& RequestId,
        int64 WorldTick, bool& bAlreadyApplied, FString& Error);

    /** Fresh content stats in, canonical equipment contributions out. The base
     * cache is projection-only; HP and skill truth remain with their executors. */
    bool ProjectCombatUnit(FOGCombatUnitState& Unit,
        const FOGEquipmentCatalog& Catalog, FString& Error) const;
    bool ProjectCombatUnit(FOGCombatUnitState& Unit, FString& Error) const;

private:
    bool ValidateRuler(const FOGEntityId&, FString&) const;
    bool ReadTarget(const FOGEntityId&, const FOGContentId&, const FOGEntityId&,
        FOGCharacterManifestationRecord&, FString&) const;
    IOGWorldStore& Store;
};
