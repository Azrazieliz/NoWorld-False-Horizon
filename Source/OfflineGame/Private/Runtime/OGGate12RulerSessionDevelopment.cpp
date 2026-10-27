#include "Runtime/OGGate12RulerSessionSubsystem.h"

#include "Combat/OGTurnBattle.h"
#include "Combat/OGWorldActionRuntime.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Progression/OGCharacterDevelopmentService.h"
#include "Runtime/OGGameCoreSubsystem.h"

namespace
{
bool IsDevelopmentBattleRunning(const UOGGameCoreSubsystem& Core)
{
    const FOGTurnBattle* Battle = Core.GetOwnedLuminousTurnBattle();
    return Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running;
}
}

bool UOGGate12RulerSessionSubsystem::ValidateOwnedDevelopmentTarget(
    IOGWorldStore& Store, const FOGContentId& IdentityId,
    const FOGEntityId& ManifestationId,
    FOGCharacterManifestationRecord& OutCurrent, FString& OutError) const
{
    OutCurrent = FOGCharacterManifestationRecord();
    if (!bRulerValidated || !BoundRulerId.IsValid() || !IdentityId.IsValid() ||
        !ManifestationId.IsValid() ||
        IdentityId.ToString().StartsWith(TEXT("diagnostic:")))
    {
        OutError = TEXT("Character development requires a real owned character and Ruler.");
        return false;
    }

    // A cached binding is insufficient after recovery/import or owner removal.
    bool bOwnerFound = false;
    FName OwnerKind;
    FString OwnerState;
    int64 OwnerRevision = 0;
    if (!Store.TryReadEntity(BoundRulerId, bOwnerFound, OwnerKind,
            OwnerState, OwnerRevision, OutError))
        return false;
    if (!bOwnerFound || OwnerKind != FName(TEXT("ruler")) ||
        OwnerState.Contains(TEXT("\"diagnosticFixture\"")))
    {
        OutError = TEXT("The bound Ruler is no longer valid in the canonical world.");
        return false;
    }

    bool bFound = false;
    FOGCharacterManifestationRecord Current;
    if (!Store.TryReadCharacterManifestation(
            ManifestationId, bFound, Current, OutError))
        return false;
    if (!bFound || Current.ManifestationId != ManifestationId ||
        Current.OwningRulerId != BoundRulerId || Current.IdentityId != IdentityId ||
        !Current.ActiveVersionId.IsValid() ||
        Current.ActiveVersionId.ToString().StartsWith(TEXT("diagnostic:")) ||
        Current.LifecycleState != FName(TEXT("active")))
    {
        OutError = TEXT("Development requires an active owned Manifestation with an exact Identity/Version mapping.");
        return false;
    }
    OutCurrent = MoveTemp(Current);
    return true;
}

bool UOGGate12RulerSessionSubsystem::ReadOwnedDevelopmentState(
    const FOGContentId& IdentityId, const FOGEntityId& ManifestationId,
    FOGCharacterManifestationRecord& OutCurrent, FName& OutReinforcementState,
    bool& bOutMaxReinforced, FString& OutError)
{
    OutError.Reset();
    OutCurrent = FOGCharacterManifestationRecord();
    OutReinforcementState = NAME_None;
    bOutMaxReinforced = false;
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;
    FOGCharacterManifestationRecord Current;
    if (!ValidateOwnedDevelopmentTarget(
            *Store, IdentityId, ManifestationId, Current, OutError))
        return false;

    bool bFound = false;
    FOGManifestationReinforcementRecord Reinforcement;
    if (!Store->TryReadManifestationReinforcement(
            ManifestationId, bFound, Reinforcement, OutError))
        return false;
    if (bFound)
    {
        if (Reinforcement.ManifestationId != ManifestationId)
        {
            OutError = TEXT("Reinforcement state does not match this Manifestation.");
            return false;
        }
        OutReinforcementState = Reinforcement.ReinforcementState;
        bOutMaxReinforced = Reinforcement.bMaxReinforced;
    }
    OutCurrent = MoveTemp(Current);
    return true;
}

bool UOGGate12RulerSessionSubsystem::ReadOwnedDevelopmentOptions(
    const FOGContentId& IdentityId, const FOGEntityId& ManifestationId,
    TArray<FOGCharacterDevelopmentPreview>& OutOptions, FString& OutError)
{
    OutOptions.Reset();
    OutError.Reset();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;
    FOGCharacterManifestationRecord Current;
    if (!ValidateOwnedDevelopmentTarget(
            *Store, IdentityId, ManifestationId, Current, OutError))
        return false;

    TArray<FOGCharacterDevelopmentDefinition> Definitions;
    if (!FOGCharacterDevelopmentCatalog::LoadInstalled(Definitions, OutError))
        return false;

    FOGCharacterDevelopmentService Development(*Store);
    TArray<FOGCharacterDevelopmentPreview> Options;
    const bool bBattleRunning = IsDevelopmentBattleRunning(*Core);
    for (const FOGCharacterDevelopmentDefinition& Definition : Definitions)
    {
        if (Definition.IdentityId != IdentityId ||
            Definition.VersionId != Current.ActiveVersionId)
            continue;

        FOGCharacterDevelopmentPreview Preview;
        if (!Development.Preview(BoundRulerId, ManifestationId,
                Definition, Preview, OutError))
            return false;
        if (bBattleRunning)
        {
            Preview.bCanApply = false;
            Preview.MissingRequirements.Add(
                TEXT("Finish the running battle before developing this character."));
        }
        Options.Add(MoveTemp(Preview));
    }
    OutOptions = MoveTemp(Options);
    return true;
}

bool UOGGate12RulerSessionSubsystem::ConfirmOwnedDevelopment(
    const FOGContentId& IdentityId, const FOGEntityId& ManifestationId,
    const FOGContentId& DevelopmentId, const FString& ConfirmationToken,
    const FOGEntityId& RequestId, FString& OutError)
{
    OutError.Reset();
    UOGGameCoreSubsystem* Core = nullptr;
    IOGWorldStore* Store = nullptr;
    if (!ResolveStore(Core, Store, OutError))
        return false;
    FOGCharacterManifestationRecord Current;
    if (!ValidateOwnedDevelopmentTarget(
            *Store, IdentityId, ManifestationId, Current, OutError))
        return false;
    if (SelectedIdentityId != IdentityId ||
        SelectedManifestationId != ManifestationId)
    {
        OutError = TEXT("Character selection changed. Select and review this Manifestation again.");
        return false;
    }
    if (!DevelopmentId.IsValid() || ConfirmationToken.IsEmpty() || !RequestId.IsValid())
    {
        OutError = TEXT("Review a valid authored development choice before confirming it.");
        return false;
    }
    if (IsDevelopmentBattleRunning(*Core))
    {
        OutError = TEXT("Finish the running battle before developing a character; its combat snapshot is already active.");
        return false;
    }

    // Never execute a UI-cached definition or accept a different Version's choice.
    TArray<FOGCharacterDevelopmentDefinition> Definitions;
    if (!FOGCharacterDevelopmentCatalog::LoadInstalled(Definitions, OutError))
        return false;
    const FOGCharacterDevelopmentDefinition* Definition = Definitions.FindByPredicate(
        [&](const FOGCharacterDevelopmentDefinition& Candidate)
        {
            return Candidate.DevelopmentId == DevelopmentId &&
                Candidate.IdentityId == IdentityId &&
                Candidate.VersionId == Current.ActiveVersionId;
        });
    if (!Definition)
    {
        OutError = TEXT("The reviewed development is no longer installed for this exact Identity/Version.");
        return false;
    }
    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < 0)
    {
        OutError = TEXT("Development requires canonical world time.");
        return false;
    }

    bool bAlreadyApplied = false;
    FOGCharacterDevelopmentService Development(*Store);
    if (!Development.Apply(BoundRulerId, ManifestationId, *Definition,
            ConfirmationToken, RequestId, Tick, bAlreadyApplied, OutError))
        return false;
    const FOGEntityId AppliedManifestationId = ManifestationId;
    const FOGContentId AppliedDevelopmentId = DevelopmentId;

    // The transaction has committed. A presentation refresh failure must not
    // masquerade as a failed purchase and encourage a second command.
    OutError.Reset();
    TArray<FString> RefreshWarnings;
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            TArray<UOGWorldPartyRuntimeComponent*> Parties;
            It->GetComponents<UOGWorldPartyRuntimeComponent>(Parties);
            for (UOGWorldPartyRuntimeComponent* Party : Parties)
            {
                if (!IsValid(Party) || Party->GetWorld() != World)
                    continue;
                FString PartyError;
                if (!Party->RefreshDevelopmentStats(PartyError))
                    RefreshWarnings.Add(TEXT("World party stats: ") +
                        (PartyError.IsEmpty() ? TEXT("refresh unavailable.") : PartyError));
            }
        }
    }
    FString RefreshError;
    if (!RefreshCanonicalProjections(RefreshError))
        RefreshWarnings.Add(TEXT("Ruler display: ") +
            (RefreshError.IsEmpty() ? TEXT("refresh unavailable.") : RefreshError));
    if (!RefreshWarnings.IsEmpty())
    {
        OutError = TEXT("Development is saved. Display update: ") +
            FString::Join(RefreshWarnings, TEXT(" "));
        UE_LOG(LogTemp, Warning, TEXT("%s"), *OutError);
    }
    if (!bAlreadyApplied)
        OnCharacterDevelopmentApplied.Broadcast(AppliedManifestationId, AppliedDevelopmentId);
    return true;
}
