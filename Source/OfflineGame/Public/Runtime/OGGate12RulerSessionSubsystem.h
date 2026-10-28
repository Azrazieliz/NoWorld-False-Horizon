#pragma once

#include "CoreMinimal.h"
#include "Core/OGEntityId.h"
#include "Content/OGContentId.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/OGUiViewModels.h"
#include "Runtime/OGPackageReportManagementRecords.h"
#include "Objects/OGObjectActionDefinition.h"
#include "OGGate12RulerSessionSubsystem.generated.h"

class UOGGameCoreSubsystem;
class IOGWorldStore;
class UOGGate12CharacterVisualCatalog;
class AOGWorldPrototypeCharacter;
struct FOGCharacterDevelopmentPreview;
struct FOGCharacterManifestationRecord;
struct FOGOwnedInventorySnapshot;
struct FOGOwnedEquipmentPreview;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOGGate12CharacterDevelopmentApplied,
    const FOGEntityId&, const FOGContentId&);

/**
 * Post-Android P0: live Ruler navigation and authoritative service projections.
 *
 * This is a replaceable Gate10-facing runtime seam, NOT an alternative save
 * database or provisional Character Registry. It never creates a Ruler,
 * Manifestation, gacha banner, currency balance, or Luminous production ID.
 * Without a genuine owner record it remains unbound and shows empty/locked
 * states. Only verified owned Manifestations can be selected and persisted.
 */
UCLASS()
class OFFLINEGAME_API UOGGate12RulerSessionSubsystem final
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Read the world-embedded local owner binding; never creates a test owner. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool TryRestoreLocalRuler(FString& OutError);

    /** Explicit New Game action: creates only a real local Ruler, transactionally. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool CreateLocalRuler(FString& OutError);

    /** Bind an existing, store-backed player owner. No synthetic fallback ID. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool BindExistingRuler(const FOGEntityId& CandidateOwner, FString& OutError);

    /** Re-project all five destinations from the existing canonical world store. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool RefreshCanonicalProjections(FString& OutError);

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool Navigate(EOGRulerPrimaryDestination Destination, FString& OutError);

    /** Native Records detail is strictly read-only and never invents history. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Records")
    bool RefreshRecordsDetail(FString& OutError);

    /** Persist acknowledgement of an actual report belonging to this Ruler. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Records")
    bool AcknowledgeOwnedReport(const FOGEntityId& ReportId, FString& OutError);

    /** Historical results only; never synthesizes a banner or transaction. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Gacha")
    bool RefreshGachaHistory(FString& OutError);

    /** The Ruler is the original player and is NEVER a gacha Manifestation.
     * Selecting the Ruler must remain possible after selecting a summon. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Characters")
    bool SelectRulerForWorld(FString& OutError);

    /** Persist identity-specific "last used" only after validating actual ownership. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool SelectOwnedManifestation(
        const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId,
        FString& OutError);

    /** Read canonical state even when no authored development choices exist.
     * None means no reinforcement record, rather than an invented starting tier. */
    bool ReadOwnedDevelopmentState(const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId,
        FOGCharacterManifestationRecord& OutCurrent,
        FName& OutReinforcementState, bool& bOutMaxReinforced,
        FString& OutError);

    /** Only installed choices for this owned Manifestation's exact Identity/Version. */
    bool ReadOwnedDevelopmentOptions(const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId,
        TArray<FOGCharacterDevelopmentPreview>& OutOptions, FString& OutError);

    /** Re-read content and ownership, then let the canonical service transact.
     * A true result means Apply succeeded, even if OutError reports a subsequent
     * projection refresh warning. RequestId belongs to the reviewed command. */
    bool ConfirmOwnedDevelopment(const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId, const FOGContentId& DevelopmentId,
        const FString& ConfirmationToken, const FOGEntityId& RequestId,
        FString& OutError);

    // Presentation notification after a new committed transaction; no state authority.
    FOGGate12CharacterDevelopmentApplied OnCharacterDevelopmentApplied;

    /** Real item instances only; browsing also works before selecting a copy. */
    bool ReadOwnedInventory(FOGOwnedInventorySnapshot& Out, FString& OutError);
    bool ReviewOwnedEquipment(const FOGContentId& SlotId, const FOGEntityId& ItemId,
        bool bUnequip, FOGOwnedEquipmentPreview& Out, FString& OutError);
    /** A committed command returns true even when OutError contains a later
     * live-party/projection refresh warning. No UI quote is state authority. */
    bool ConfirmOwnedEquipment(const FOGOwnedEquipmentPreview& Reviewed,
        const FOGEntityId& RequestId, FString& OutError);

    /** Shared object operations use the selected real instances and canonical wallets/stock. */
    UFUNCTION(BlueprintCallable,Category="OfflineGame|Ruler|Objects")
    bool ReadOwnedObjectActions(const FOGEntityId& SelectedItem,const FOGEntityId& Counterparty,
        TArray<FOGObjectActionPreview>& Out,FString& Error);
    UFUNCTION(BlueprintCallable,Category="OfflineGame|Ruler|Objects")
    bool ReviewOwnedObjectAction(const FOGContentId& ActionId,const FOGEntityId& SelectedItem,
        const FOGEntityId& Counterparty,FOGObjectActionPreview& Out,FString& Error);
    UFUNCTION(BlueprintCallable,Category="OfflineGame|Ruler|Objects")
    bool ConfirmOwnedObjectAction(const FOGObjectActionPreview& Reviewed,const FOGEntityId& RequestId,FString& Error);

    /** Writes only an authored, version-exact real wardrobe value in canonical
     * presentation JSON, preserving other physical outfit/equipment/world data.
     * Privacy is NEVER changed by this command. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Characters")
    bool SelectOwnedWardrobeChoice(
        const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId,
        const UOGGate12CharacterVisualCatalog* VisualCatalog,
        FName WardrobeChoice,
        FString& OutError);

    /** Resolve a strictly approved playable actor for selected owned
     * Manifestation, or return null if none is selected. Never assigns IDs,
     * installs a package or grants ownership. Fail closed on missing assets. */
    bool ResolveSelectedOwnedWorldPawn(
        const UOGGate12CharacterVisualCatalog* VisualCatalog,
        TSubclassOf<AOGWorldPrototypeCharacter>& OutWorldClass,
        FOGContentId& OutVersionId,
        FString& OutError);

    /** Calls the existing presentation/orientation subsystem, no level reset. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool EnterWorld(FString& OutError);

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool ReturnToRuler(FString& OutError);

    UFUNCTION(BlueprintPure, Category="OfflineGame|Ruler|P0")
    bool IsBoundToStoredRuler() const { return bRulerValidated; }

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    EOGRulerPrimaryDestination CurrentDestination =
        EOGRulerPrimaryDestination::Home;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    FOGEntityId BoundRulerId;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    FOGContentId SelectedIdentityId;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    FOGEntityId SelectedManifestationId;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    FOGRulerShellViewModel HomeProjection;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    TArray<FOGRosterIdentityViewModel> OwnedRoster;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    FOGTerritoryViewModel TerritoryProjection;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    FOGRecordsHubViewModel RecordsProjection;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|Records")
    TArray<FOGChronicleEntryViewModel> ChronicleProjection;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|Records")
    TArray<FOGReportRecord> ReportProjection;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|Records")
    TArray<FOGIntelligenceEntryViewModel> IntelligenceProjection;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|Gacha")
    TArray<FOGGachaHistoryEntryViewModel> GachaHistoryProjection;

    /** Unlocked access does not imply any approved/authored banner exists. */
    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    bool bRealGachaAccessUnlocked = false;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    bool bApprovedBannerProvided = false;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Ruler|P0")
    FString UserFacingAvailability;

private:
    bool ResolveStore(
        UOGGameCoreSubsystem*& OutCore,
        IOGWorldStore*& OutStore,
        FString& OutError) const;

    bool ValidateOwnedDevelopmentTarget(IOGWorldStore& Store,
        const FOGContentId& IdentityId, const FOGEntityId& ManifestationId,
        FOGCharacterManifestationRecord& OutCurrent, FString& OutError) const;

    void ClearProjectionAndSelection();
    void HandleCanonicalRuntimeReleasing();
    bool bRulerValidated = false;
};
