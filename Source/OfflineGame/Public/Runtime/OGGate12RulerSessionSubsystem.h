#pragma once

#include "CoreMinimal.h"
#include "Core/OGEntityId.h"
#include "Content/OGContentId.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/OGUiViewModels.h"
#include "OGGate12RulerSessionSubsystem.generated.h"

class UOGGameCoreSubsystem;
class IOGWorldStore;

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
    virtual void Deinitialize() override;

    /** Bind an existing, store-backed player owner. No synthetic fallback ID. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool BindExistingRuler(const FOGEntityId& CandidateOwner, FString& OutError);

    /** Re-project all five destinations from the existing canonical world store. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool RefreshCanonicalProjections(FString& OutError);

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool Navigate(EOGRulerPrimaryDestination Destination, FString& OutError);

    /** Persist identity-specific "last used" only after validating actual ownership. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|P0")
    bool SelectOwnedManifestation(
        const FOGContentId& IdentityId,
        const FOGEntityId& ManifestationId,
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

    void ClearProjectionAndSelection();
    bool bRulerValidated = false;
};
