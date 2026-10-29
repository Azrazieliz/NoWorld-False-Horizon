#include "Runtime/OGGate12RulerSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "Persistence/OGWorldStore.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12LocalRulerBootstrap.h"
#include "Runtime/OGReportService.h"
#include "Runtime/OGFoundationCharacterRuntime.h"
#include "UI/OGGate12CharacterVisualCatalog.h"
#include "UI/OGGate12OwnedWorldCharacterBinding.h"
#include "World/OGStartingRegionPresentation.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
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
    ReportProjection.Reset();
    GachaHistoryProjection.Reset();
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
        if (!RefreshCanonicalProjections(OutError)) return false;
        bool Restored=false; FString RecoveryError;
        if (!Core->RestoreOwnedNativeEncounter(BoundRulerId,Restored,RecoveryError))
            Core->NativeEncounterRecoveryStatus=RecoveryError;
        return true;
    }

    ClearProjectionAndSelection();
    BoundRulerId = CandidateOwner;
    bRulerValidated = true;
    if (!RefreshCanonicalProjections(OutError))
    {
        ClearProjectionAndSelection();
        return false;
    }
    bool Restored=false; FString RecoveryError;
    if (!Core->RestoreOwnedNativeEncounter(BoundRulerId,Restored,RecoveryError))
    {
        // Keep the valid Ruler/save available; retain the original recovery record for a later retry.
        Core->NativeEncounterRecoveryStatus=RecoveryError;
        UE_LOG(LogTemp,Warning,TEXT("Native encounter recovery retained for retry: %s"),*RecoveryError);
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

bool UOGGate12RulerSessionSubsystem::RefreshGachaHistory(FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated || !BoundRulerId.IsValid())
    {
        OutError = TEXT("Gacha history requires an existing local Ruler.");
        return false;
    }
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;

    // History is a genuine recorded event stream, not a claim of active
    // banner, currency, rate/pity context or new reward transaction.
    FOGUiViewModelService Ui(*Store);
    TArray<FOGGachaHistoryEntryViewModel> ActualHistory;
    if (!Ui.BuildGachaHistory(BoundRulerId, 50, ActualHistory, OutError))
        return false;

    GachaHistoryProjection = MoveTemp(ActualHistory);
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
    TArray<FOGReportRecord> ActualReports;
    TArray<FOGChronicleEntryViewModel> ActualChronicle;
    TArray<FOGIntelligenceEntryViewModel> ActualIntelligence;
    if (!Store->ListReportsByOwner(BoundRulerId, ActualReports, OutError) ||
        !Ui.BuildChronicle(BoundRulerId, 25, ActualChronicle, OutError) ||
        !Ui.BuildIntelligence(BoundRulerId, ActualIntelligence, OutError))
        return false;

    ActualReports.Sort([](const FOGReportRecord& A, const FOGReportRecord& B)
    {
        if (A.bAcknowledged != B.bAcknowledged) return !A.bAcknowledged;
        if (A.Priority != B.Priority) return A.Priority > B.Priority;
        if (A.CreatedWorldTick != B.CreatedWorldTick)
            return A.CreatedWorldTick > B.CreatedWorldTick;
        return A.ReportId.ToString().Compare(
            B.ReportId.ToString(), ESearchCase::CaseSensitive) < 0;
    });
    ReportProjection = MoveTemp(ActualReports);
    ChronicleProjection = MoveTemp(ActualChronicle);
    IntelligenceProjection = MoveTemp(ActualIntelligence);
    return true;
}

bool UOGGate12RulerSessionSubsystem::AcknowledgeOwnedReport(
    const FOGEntityId& ReportId, FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated || !BoundRulerId.IsValid() || !ReportId.IsValid())
    {
        OutError = TEXT("Acknowledging Reports requires a verified Ruler and Report.");
        return false;
    }

    // Revalidate the owner against the current world after a restore/import.
    if (!RefreshCanonicalProjections(OutError)) return false;

    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError)) return false;

    bool bFound = false;
    FOGReportRecord Report;
    if (!Store->TryReadReport(ReportId, bFound, Report, OutError))
        return false;
    if (!bFound || Report.OwnerEntityId != BoundRulerId)
    {
        OutError = TEXT("This Report does not belong to the selected Ruler.");
        return false;
    }
    if (Report.bAcknowledged)
        return RefreshRecordsDetail(OutError);

    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < Report.CreatedWorldTick)
    {
        OutError = TEXT("Canonical world time is unavailable for Report acknowledgement.");
        return false;
    }
    FOGReportService Reports(*Store);
    if (!Reports.AcknowledgeReport(ReportId, Tick, OutError))
        return false;
    if (!RefreshCanonicalProjections(OutError))
        return false;
    return RefreshRecordsDetail(OutError);
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

bool UOGGate12RulerSessionSubsystem::SelectRulerForWorld(FString& OutError)
{
    OutError.Reset();
    if (!RefreshCanonicalProjections(OutError))
        return false;

    // No manufactured owned Manifestation and no deletion of the previously
    // acquired roster. This selection changes only the World controller.
    SelectedIdentityId = FOGContentId();
    SelectedManifestationId = FOGEntityId();
    CurrentDestination = EOGRulerPrimaryDestination::Characters;
    UserFacingAvailability = TEXT("Controlling the original Ruler protagonist.");
    return true;
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

bool UOGGate12RulerSessionSubsystem::SelectOwnedWardrobeChoice(
    const FOGContentId& IdentityId,
    const FOGEntityId& ManifestationId,
    const UOGGate12CharacterVisualCatalog* VisualCatalog,
    FName WardrobeChoice,
    FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated || !BoundRulerId.IsValid() ||
        !IdentityId.IsValid() || !ManifestationId.IsValid() ||
        !VisualCatalog || WardrobeChoice.IsNone())
    {
        OutError = TEXT("A verified owned Manifestation and authored wardrobe are required.");
        return false;
    }
    // Recheck that this Ruler still exists in the current world after imports.
    if (!RefreshCanonicalProjections(OutError)) return false;

    const FOGRosterIdentityViewModel* Identity = OwnedRoster.FindByPredicate(
        [&IdentityId](const FOGRosterIdentityViewModel& Entry)
        {
            return Entry.IdentityId == IdentityId;
        });
    const FOGRosterManifestationViewModel* Owned = Identity
        ? Identity->Manifestations.FindByPredicate(
            [&ManifestationId](const FOGRosterManifestationViewModel& Entry)
            {
                return Entry.ManifestationId == ManifestationId;
            })
        : nullptr;
    if (!Owned || Owned->bLocked)
    {
        OutError = TEXT("Only unlocked owned Manifestations can change wardrobe.");
        return false;
    }

    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError)) return false;
    bool bFound = false;
    FOGCharacterManifestationRecord Record;
    if (!Store->TryReadCharacterManifestation(
            ManifestationId, bFound, Record, OutError))
        return false;
    if (!bFound || Record.OwningRulerId != BoundRulerId ||
        Record.IdentityId != IdentityId || !Record.ActiveVersionId.IsValid() ||
        Record.LifecycleState != FName(TEXT("active")))
    {
        OutError = TEXT("Canonical ownership/Version has changed; wardrobe not modified.");
        return false;
    }

    const FOGGate12CharacterVisualEntry* Entry =
        VisualCatalog->FindExact(IdentityId, Record.ActiveVersionId);
    if (!Entry || Entry->WardrobeStateJsonField.IsNone() ||
        !Entry->ApprovedWardrobeChoices.Contains(WardrobeChoice))
    {
        OutError = TEXT("Wardrobe value was not authored for this exact character Version.");
        return false;
    }

    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < 0)
    {
        OutError = TEXT("Canonical time unavailable for wardrobe change.");
        return false;
    }
    FOGFoundationCharacterContext Context;
    Context.EntityId = ManifestationId;
    Context.ManifestationId = ManifestationId;
    Context.OwnerEntityId = BoundRulerId;
    FOGFoundationCharacterRuntime Runtime(*Store);
    FOGFoundationCharacterProjection Projection;
    if (!Runtime.Project(Context, Projection, OutError))
        return false;
    if (!Projection.bHasManifestation ||
        Projection.Manifestation.IdentityId != IdentityId ||
        Projection.Manifestation.ActiveVersionId != Record.ActiveVersionId)
    {
        OutError = TEXT("Appearance projection no longer matches the owned Manifestation.");
        return false;
    }

    const FString Existing = Projection.bHasPresentation
        ? Projection.Presentation.OutfitStateJson : TEXT("{}");
    TSharedPtr<FJsonObject> Outfit;
    if (!FJsonSerializer::Deserialize(
            TJsonReaderFactory<>::Create(
                Existing.IsEmpty() ? TEXT("{}") : Existing), Outfit) ||
        !Outfit.IsValid())
    {
        OutError = TEXT("Existing canonical wardrobe JSON invalid; refusing overwrite.");
        return false;
    }
    // Preserve all other JSON fields. Neither Privacy nor clothing equipment is touched.
    Outfit->SetStringField(
        Entry->WardrobeStateJsonField.ToString(), WardrobeChoice.ToString());
    FString Updated;
    if (!FJsonSerializer::Serialize(
            Outfit.ToSharedRef(), TJsonWriterFactory<>::Create(&Updated)))
    {
        OutError = TEXT("Canonical outfit serialization failed.");
        return false;
    }
    FOGManifestationPresentationStateRecord Presentation =
        Projection.bHasPresentation
            ? Projection.Presentation : FOGManifestationPresentationStateRecord();
    Presentation.OutfitStateJson = Updated;
    if (Presentation.PresentationVariantStateJson.IsEmpty())
        Presentation.PresentationVariantStateJson = TEXT("{}");

    if (!Runtime.SetPresentation(Context, Presentation, Tick, OutError))
        return false;
    return RefreshCanonicalProjections(OutError);
}

bool UOGGate12RulerSessionSubsystem::ResolveSelectedOwnedWorldPawn(
    const UOGGate12CharacterVisualCatalog* VisualCatalog,
    TSubclassOf<AOGWorldPrototypeCharacter>& OutWorldClass,
    FOGContentId& OutVersionId,
    FString& OutError)
{
    OutWorldClass = nullptr;
    OutVersionId = FOGContentId();
    OutError.Reset();

    if (!bRulerValidated || !BoundRulerId.IsValid())
    {
        OutError = TEXT("A verified active Ruler is required for World selection.");
        return false;
    }
    if (!RefreshCanonicalProjections(OutError))
        return false;
    // A Ruler can enter World in their baseline form without acquiring
    // a playable character. This is NOT a substitute for an owned character.
    if (!SelectedManifestationId.IsValid() && SelectedIdentityId.IsEmpty())
        return true;
    if (!SelectedManifestationId.IsValid() || !SelectedIdentityId.IsValid())
    {
        OutError = TEXT("Incomplete Identity/owned Manifestation world selection.");
        return false;
    }

    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;

    FOGCharacterManifestationRecord Owned;
    bool bFound = false;
    if (!Store->TryReadCharacterManifestation(
            SelectedManifestationId, bFound, Owned, OutError))
        return false;
    if (!bFound || Owned.OwningRulerId != BoundRulerId ||
        Owned.IdentityId != SelectedIdentityId ||
        Owned.LifecycleState != FName(TEXT("active")) ||
        !Owned.ActiveVersionId.IsValid())
    {
        OutError = TEXT("Selected playable character is no longer an active owned Manifestation.");
        return false;
    }

    const FOGGate12CharacterVisualEntry* Assets = VisualCatalog
        ? VisualCatalog->FindExact(SelectedIdentityId, Owned.ActiveVersionId)
        : nullptr;
    if (!Assets || Assets->ApprovedWorldPlayableClass.IsNull())
    {
        OutError = TEXT("Installed character Version has no approved playable World pawn.");
        return false;
    }

    UClass* PawnClass = Assets->ApprovedWorldPlayableClass.LoadSynchronous();
    if (!PawnClass || !PawnClass->IsChildOf(AOGWorldPrototypeCharacter::StaticClass()) ||
        !PawnClass->ImplementsInterface(
            UOGGate12OwnedWorldCharacterBinding::StaticClass()))
    {
        OutError = TEXT("Approved World pawn does not support canonical owned-character binding.");
        return false;
    }

    OutWorldClass = PawnClass;
    OutVersionId = Owned.ActiveVersionId;
    return true;
}

bool UOGGate12RulerSessionSubsystem::EnterWorld(FString& OutError)
{
    OutError.Reset();
    if (!bRulerValidated || !BoundRulerId.IsValid())
    {
        OutError = TEXT("World transition requires an actual bound Ruler.");
        return false;
    }
    // A once-valid session cannot authorize World entry after import/restore
    // removed its owner. Recheck the canonical SQLite row and projections
    // immediately before flipping presentation/orientation.
    if (!RefreshCanonicalProjections(OutError))
    {
        if (OutError.IsEmpty())
            OutError = TEXT("Canonical Ruler validation failed before World entry.");
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
