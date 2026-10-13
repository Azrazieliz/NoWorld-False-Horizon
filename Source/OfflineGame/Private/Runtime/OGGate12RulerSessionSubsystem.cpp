#include "Runtime/OGGate12RulerSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Persistence/OGWorldStore.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12LocalRulerBootstrap.h"
#include "Runtime/OGPackageReportManagementRecords.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "UI/OGUiViewModelService.h"

void UOGGate12RulerSessionSubsystem::Initialize(
    FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UOGGameCoreSubsystem>();
    if (UOGGameCoreSubsystem* Core = GetGameInstance()
            ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>()
            : nullptr)
    {
        Core->OnCanonicalRuntimeReleasing.AddUObject(
            this, &UOGGate12RulerSessionSubsystem::HandleCanonicalRuntimeReleasing);
    }
    FString Error;
    if (!TryRestoreLocalRuler(Error) && !Error.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Gate12 local Ruler restore is fail-closed: %s"), *Error);
        UserFacingAvailability = Error;
    }
}

bool UOGGate12RulerSessionSubsystem::TryRestoreLocalRuler(FString& OutError)
{
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;

    bool bFound = false;
    FOGEntityId ExistingRuler;
    if (!FOGGate12LocalRulerBootstrap::TryLoad(
            *Store, bFound, ExistingRuler, OutError))
    {
        UserFacingAvailability = OutError;
        return false;
    }
    if (!bFound)
    {
        UserFacingAvailability =
            TEXT("No local Ruler exists. Use Create Local Ruler to begin.");
        OutError.Reset();
        return false;
    }
    return BindExistingRuler(ExistingRuler, OutError);
}

bool UOGGate12RulerSessionSubsystem::CreateLocalRuler(FString& OutError)
{
    OutError.Reset();
    if (bRulerValidated)
    {
        OutError = TEXT("Local Ruler is already active. No duplicate created.");
        return false;
    }
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;

    FOGEntityId Created;
    if (!FOGGate12LocalRulerBootstrap::Create(
            *Store, Core->GetCanonicalWorldTick(), Created, OutError))
    {
        UserFacingAvailability = OutError;
        return false;
    }
    return BindExistingRuler(Created, OutError);
}

void UOGGate12RulerSessionSubsystem::HandleCanonicalRuntimeReleasing()
{
    // Never show an owner/roster from the previous canonical database after
    // Clear, Recovery Import, world rollback, or runtime replacement.
    ClearProjectionAndSelection();
}

void UOGGate12RulerSessionSubsystem::ClearProjectionAndSelection()
{
    bRulerValidated = false;
    BoundRulerId = FOGEntityId();
    SelectedIdentityId = FOGContentId();
    SelectedManifestationId = FOGEntityId();
    CurrentDestination = EOGRulerPrimaryDestination::Home;
    HomeProjection = FOGRulerShellViewModel();
    OwnedRoster.Reset();
    TerritoryProjection = FOGTerritoryViewModel();
    RecordsProjection = FOGRecordsHubViewModel();
    ChronicleProjection.Reset();
    IntelligenceProjection.Reset();
    bRealGachaAccessUnlocked = false;
    bApprovedBannerProvided = false;
    UserFacingAvailability = TEXT("No existing Ruler is selected.");
}

void UOGGate12RulerSessionSubsystem::Deinitialize()
{
    if (UOGGameCoreSubsystem* Core = GetGameInstance()
            ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>()
            : nullptr)
    {
        Core->OnCanonicalRuntimeReleasing.RemoveAll(this);
    }
    ClearProjectionAndSelection();
    Super::Deinitialize();
}

bool UOGGate12RulerSessionSubsystem::ResolveStore(
    UOGGameCoreSubsystem*& OutCore,
    IOGWorldStore*& OutStore,
    FString& OutError) const
{
    OutError.Reset();
    OutStore = nullptr;
    OutCore = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>()
        : nullptr;

    if (!OutCore || !OutCore->IsCoreReady())
    {
        OutError = TEXT("The canonical world is not ready.");
        return false;
    }

    OutStore = OutCore->GetWorldStore();
    if (!OutStore || !OutStore->IsOpen())
    {
        OutError = TEXT("The canonical world store is unavailable.");
        return false;
    }
    return true;
}

bool UOGGate12RulerSessionSubsystem::BindExistingRuler(
    const FOGEntityId& CandidateOwner,
    FString& OutError)
{
    if (!CandidateOwner.IsValid())
    {
        OutError = TEXT("No authoritative Ruler identity supplied.");
        return false;
    }

    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
    {
        return false;
    }

    bool bFound = false;
    FName Kind;
    FString StateJson;
    int64 Revision = 0;
    if (!Store->TryReadEntity(
            CandidateOwner, bFound, Kind, StateJson, Revision, OutError))
    {
        return false;
    }

    // Refuse to turn an arbitrary World entity or presentation fixture into
    // a player owner. FName is case-insensitive in Unreal.
    if (!bFound || Kind != FName(TEXT("ruler")) ||
        StateJson.Contains(TEXT("\"diagnosticFixture\"")))
    {
        OutError =
            TEXT("Candidate is not a stored canonical Ruler entity.");
        return false;
    }

    if (bRulerValidated && CandidateOwner == BoundRulerId)
    {
        return RefreshCanonicalProjections(OutError);
    }

    ClearProjectionAndSelection();
    BoundRulerId = CandidateOwner;
    bRulerValidated = true;
    if (!RefreshCanonicalProjections(OutError))
    {
        ClearProjectionAndSelection();
        return false;
    }
    return true;
}

bool UOGGate12RulerSessionSubsystem::RefreshCanonicalProjections(
    FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated || !BoundRulerId.IsValid())
    {
        OutError = TEXT("No stored Ruler selected. Cannot project a fictitious roster.");
        UserFacingAvailability = OutError;
        return false;
    }

    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
    {
        UserFacingAvailability = OutError;
        return false;
    }

    // Recheck the bound owner on every refresh. A world import/recovery may
    // have swapped databases since selection was established.
    bool bOwnerStillFound = false;
    FName OwnerKind;
    FString OwnerState;
    int64 OwnerCreated = 0;
    if (!Store->TryReadEntity(BoundRulerId, bOwnerStillFound,
            OwnerKind, OwnerState, OwnerCreated, OutError))
        return false;
    if (!bOwnerStillFound || OwnerKind != FName(TEXT("ruler")) ||
        OwnerState.Contains(TEXT("\"diagnosticFixture\"")))
    {
        ClearProjectionAndSelection();
        OutError = TEXT("Bound player is no longer a valid Ruler in this world.");
        UserFacingAvailability = OutError;
        return false;
    }

    // Projections are assembled off-state and published only if all succeed.
    FOGUiViewModelService Ui(*Store);
    FOGRulerShellViewModel NewShell;
    TArray<FOGRosterIdentityViewModel> NewRoster;
    FOGTerritoryViewModel NewTerritory;
    FOGRecordsHubViewModel NewRecords;

    if (!Ui.BuildRulerShell(BoundRulerId, NewShell, OutError) ||
        !Ui.BuildRoster(BoundRulerId, NewRoster, OutError) ||
        !Ui.BuildTerritory(BoundRulerId, FOGEntityId(), NewTerritory, OutError) ||
        !Ui.BuildRecordsHub(BoundRulerId, NewRecords, OutError))
    {
        UserFacingAvailability = OutError.IsEmpty()
            ? TEXT("Canonical Ruler projections unavailable.")
            : OutError;
        return false;
    }

    HomeProjection = MoveTemp(NewShell);
    OwnedRoster = MoveTemp(NewRoster);
    TerritoryProjection = MoveTemp(NewTerritory);
    RecordsProjection = MoveTemp(NewRecords);
    bRealGachaAccessUnlocked = HomeProjection.bGachaUnlocked;

    // There is no accepted live banner contract yet. Access is not a pool.
    bApprovedBannerProvided = false;

    bool bSelectionStillOwned = false;
    for (const FOGRosterIdentityViewModel& Identity : OwnedRoster)
    {
        if (Identity.IdentityId != SelectedIdentityId)
        {
            continue;
        }
        for (const FOGRosterManifestationViewModel& Instance :
             Identity.Manifestations)
        {
            if (Instance.ManifestationId == SelectedManifestationId &&
                !Instance.bLocked)
            {
                bSelectionStillOwned = true;
                break;
            }
        }
    }
    if (!bSelectionStillOwned)
    {
        SelectedIdentityId = FOGContentId();
        SelectedManifestationId = FOGEntityId();
    }

    switch (CurrentDestination)
    {
        case EOGRulerPrimaryDestination::Characters:
            UserFacingAvailability = OwnedRoster.IsEmpty()
                ? TEXT("No owned character Manifestations in this world.")
                : TEXT("Showing actual owned Manifestations.");
            break;
        case EOGRulerPrimaryDestination::Gacha:
            UserFacingAvailability = bRealGachaAccessUnlocked
                ? TEXT("No approved live banner supplied. Summoning unavailable.")
                : TEXT("Summoning is locked in canonical world state.");
            break;
        case EOGRulerPrimaryDestination::Territory:
            UserFacingAvailability = TerritoryProjection.Territories.IsEmpty()
                ? TEXT("No controlled territories in this world.")
                : TEXT("Showing persisted territory claims.");
            break;
        case EOGRulerPrimaryDestination::Records:
            UserFacingAvailability = RecordsProjection.ReportCount == 0
                ? TEXT("No reports in this world.")
                : TEXT("Showing canonical report records.");
            break;
        default:
            UserFacingAvailability = TEXT("Ruler Home is connected to canonical world state.");
            break;
    }
    return true;
}

bool UOGGate12RulerSessionSubsystem::RefreshRecordsDetail(FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated || !BoundRulerId.IsValid())
    {
        OutError = TEXT("No real Ruler selected for Records.");
        return false;
    }
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;

    FOGUiViewModelService Ui(*Store);
    TArray<FOGChronicleEntryViewModel> ActualChronicle;
    TArray<FOGIntelligenceEntryViewModel> ActualIntelligence;
    if (!Ui.BuildChronicle(BoundRulerId, 25, ActualChronicle, OutError) ||
        !Ui.BuildIntelligence(BoundRulerId, ActualIntelligence, OutError))
        return false;

    ChronicleProjection = MoveTemp(ActualChronicle);
    IntelligenceProjection = MoveTemp(ActualIntelligence);
    return true;
}

bool UOGGate12RulerSessionSubsystem::Navigate(
    EOGRulerPrimaryDestination Destination,
    FString& OutError)
{
    if (!bRulerValidated)
    {
        OutError = TEXT("Select a real Ruler owner before navigating.");
        UserFacingAvailability = OutError;
        return false;
    }

    CurrentDestination = Destination;
    return RefreshCanonicalProjections(OutError);
}

bool UOGGate12RulerSessionSubsystem::SelectOwnedManifestation(
    const FOGContentId& IdentityId,
    const FOGEntityId& ManifestationId,
    FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated || !IdentityId.IsValid() ||
        !ManifestationId.IsValid() ||
        IdentityId.ToString().StartsWith(TEXT("diagnostic:")))
    {
        OutError =
            TEXT("Selection requires real owned, non-diagnostic canonical character IDs.");
        return false;
    }

    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
    {
        return false;
    }

    TArray<FOGRosterIdentityViewModel> Roster;
    FOGUiViewModelService Ui(*Store);
    if (!Ui.BuildRoster(BoundRulerId, Roster, OutError))
    {
        return false;
    }

    bool bOwnedAndSelectable = false;
    for (const FOGRosterIdentityViewModel& Identity : Roster)
    {
        if (Identity.IdentityId != IdentityId)
        {
            continue;
        }
        for (const FOGRosterManifestationViewModel& Entry :
             Identity.Manifestations)
        {
            if (Entry.ManifestationId == ManifestationId && !Entry.bLocked)
            {
                bOwnedAndSelectable = true;
            }
        }
    }

    if (!bOwnedAndSelectable)
    {
        OutError = TEXT("Manifestation is not selectable in this Ruler's owned roster.");
        return false;
    }

    bool bFound = false;
    FOGCharacterManifestationRecord Record;
    if (!Store->TryReadCharacterManifestation(
            ManifestationId, bFound, Record, OutError))
    {
        return false;
    }

    if (!bFound || Record.OwningRulerId != BoundRulerId ||
        Record.IdentityId != IdentityId ||
        !Record.ActiveVersionId.IsValid() ||
        Record.LifecycleState != FName(TEXT("active")))
    {
        OutError =
            TEXT("Selected Manifestation has no valid active owned Identity/Version mapping.");
        return false;
    }

    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < 0)
    {
        OutError = TEXT("Cannot persist selection without canonical world time.");
        return false;
    }

    FOGManifestationContextSelectionRecord Selection;
    Selection.OwnerEntityId = BoundRulerId;
    Selection.ContextId =
        FOGUiViewModelService::CharacterLastUsedContextId(IdentityId);
    Selection.ManifestationId = ManifestationId;
    Selection.UpdatedWorldTick = Tick;

    if (!Store->UpsertManifestationContextSelection(Selection, OutError))
    {
        return false;
    }

    SelectedIdentityId = IdentityId;
    SelectedManifestationId = ManifestationId;
    CurrentDestination = EOGRulerPrimaryDestination::Characters;
    return RefreshCanonicalProjections(OutError);
}

bool UOGGate12RulerSessionSubsystem::EnterWorld(FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated)
    {
        OutError = TEXT("World transition requires an actual bound Ruler.");
        return false;
    }
    UOGPresentationModeSubsystem* Presentation = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGPresentationModeSubsystem>()
        : nullptr;
    if (!Presentation)
    {
        OutError = TEXT("Presentation subsystem is unavailable.");
        return false;
    }
    Presentation->SetMode(EOGPresentationMode::World);
    return Presentation->GetMode() == EOGPresentationMode::World;
}

bool UOGGate12RulerSessionSubsystem::ReturnToRuler(FString& OutError)
{
    OutError.Reset();
    UOGPresentationModeSubsystem* Presentation = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGPresentationModeSubsystem>()
        : nullptr;
    if (!Presentation)
    {
        OutError = TEXT("Presentation subsystem is unavailable.");
        return false;
    }
    Presentation->SetMode(EOGPresentationMode::Ruler);
    return Presentation->GetMode() == EOGPresentationMode::Ruler;
}
