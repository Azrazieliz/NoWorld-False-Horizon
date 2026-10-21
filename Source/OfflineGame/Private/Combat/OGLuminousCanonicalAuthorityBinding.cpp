#include "Combat/OGLuminousCanonicalAuthorityBinding.h"
#include "Persistence/OGWorldStore.h"

namespace
{
const FOGContentId LuminousIdentity(TEXT("character:luminous_valentine.identity"));
const FName ActiveLifecycle(TEXT("active"));
const FName RequiredMR(TEXT("MR"));

bool ReadOwnedActiveManifestation(
    IOGWorldStore& Store, const FOGEntityId& Ruler,
    const FOGCombatUnitState& Unit, FOGCharacterManifestationRecord& Out)
{
    bool bFound=false;
    FString Error;
    if(!Unit.UnitEntityId.IsValid() ||
       !Store.TryReadCharacterManifestation(Unit.UnitEntityId,bFound,Out,Error) ||
       !bFound)
        return false;
    return Out.ManifestationId==Unit.UnitEntityId &&
           Out.OwningRulerId==Ruler &&
           Out.IdentityId==LuminousIdentity &&
           Out.IdentityId==Unit.IdentityId &&
           Out.ActiveVersionId.IsValid() &&
           Out.LifecycleState==ActiveLifecycle;
}
}

bool FOGLuminousCanonicalAuthorityBinding::Bind(
    FOGTurnBattle& Battle,
    IOGWorldStore& Store,
    const FOGEntityId& OwningRulerId,
    FOGLuminousCanonicalSources Sources,
    FString& OutError)
{
    OutError.Reset();
    if(!Store.IsOpen() || !OwningRulerId.IsValid() ||
       Battle.GetState().Status!=EOGTurnBattleStatus::Running ||
       !Sources.ReadInstalledIdentity ||
       !Sources.ReadNativeBloodEligibility ||
       !Sources.ReadNativeSoulContinuity ||
       !Sources.ValidateRegisteredEffectContest ||
       !Sources.CheckRegisteredWorldFieldOwnership ||
       !Sources.RegisteredBleedFamilyId.IsValid() ||
       !Sources.WorldNegationGradeId.IsValid() ||
       Sources.FantasmByRegisteredVersion.IsEmpty())
    {
        OutError=TEXT("Cannot bind combat authority without a live canonical store and every registered blood, soul, Registry and World provider.");
        return false;
    }

    // Refuse fabricated Manifestation IDs before installing any battle gates.
    bool bFoundLuminous=false;
    for(const FOGCombatUnitState& Unit: Battle.GetState().Units)
    {
        if(Unit.IdentityId!=LuminousIdentity)continue;
        bFoundLuminous=true;
        FOGCharacterManifestationRecord Record;
        if(!ReadOwnedActiveManifestation(Store,OwningRulerId,Unit,Record))
        {
            OutError=TEXT("Luminous battle unit has no live, owned Registry Manifestation.");
            return false;
        }
        if(!Sources.FantasmByRegisteredVersion.Contains(Record.ActiveVersionId))
        {
            OutError=TEXT("Luminous Manifestation version has no installed approved Fantasm registration.");
            return false;
        }
    }
    if(!bFoundLuminous)
    {
        OutError=TEXT("No owned Luminous Manifestation exists in this battle.");
        return false;
    }

    TSharedRef<FOGLuminousCanonicalSources> Canonical =
        MakeShared<FOGLuminousCanonicalSources>(MoveTemp(Sources));

    Battle.BindLuminousAuthoritativeStatusGates(
        [&Store,OwningRulerId,Canonical](
            const FOGCombatUnitState& Attacker,
            const FOGCombatUnitState& Target,
            const FOGContentId& Family)
        {
            FOGCharacterManifestationRecord Copy;
            return Family==Canonical->RegisteredBleedFamilyId &&
                ReadOwnedActiveManifestation(Store,OwningRulerId,Attacker,Copy) &&
                Canonical->ReadNativeBloodEligibility(Attacker,Target,Family);
        },
        [&Store,OwningRulerId,Canonical](
            const FOGCombatUnitState& Restoring,
            const FOGCombatUnitState& Target,
            bool bAllyCovenant)
        {
            FOGCharacterManifestationRecord Copy;
            return ReadOwnedActiveManifestation(Store,OwningRulerId,Restoring,Copy) &&
                Canonical->ReadNativeSoulContinuity(Restoring,Target,bAllyCovenant);
        },
        Canonical->RegisteredBleedFamilyId);

    Battle.BindLuminousRegisteredContestEvidence(
        [Canonical](const FOGLuminousRegisteredEffectContest& Contest,
                    FString& Error)
        {
            // Re-evaluate source/instance/footprint and effective priorities
            // through the actual registered mechanic each event.
            return Canonical->ValidateRegisteredEffectContest(Contest,Error);
        });

    Battle.BindApprovedLuminousFieldEligibility(
        [&Store,OwningRulerId,Canonical](
            const FOGCombatUnitState& Unit,
            const FOGContentId& FantasmId,
            FString& Error)
        {
            Error.Reset();
            FOGCharacterManifestationRecord Copy;
            if(!ReadOwnedActiveManifestation(Store,OwningRulerId,Unit,Copy))
            {
                Error=TEXT("Fantasm rejected: live owner/identity/version not registered.");
                return false;
            }
            const FOGContentId* RegisteredFantasm=
                Canonical->FantasmByRegisteredVersion.Find(Copy.ActiveVersionId);
            if(!RegisteredFantasm || *RegisteredFantasm!=FantasmId)
            {
                Error=TEXT("Fantasm rejected: selected ability does not belong to the installed version.");
                return false;
            }
            if(Copy.CurrentRarity!=RequiredMR)
            {
                Error=TEXT("Fantasm rejected: mutable Current Rarity is not MR.");
                return false;
            }
            FOGCharacterIdentityDefinition Identity;
            if(!Canonical->ReadInstalledIdentity(Copy.IdentityId,Identity,Error) ||
               Identity.IdentityId!=Copy.IdentityId ||
               Identity.OriginRarity!=RequiredMR)
            {
                if(Error.IsEmpty())
                    Error=TEXT("Fantasm rejected: installed immutable Origin Rarity is not MR.");
                return false;
            }
            bool bFoundGrade=false;
            FOGManifestationWorldFantasmStateRecord Grade;
            if(!Store.TryReadManifestationWorldFantasmState(
                   Copy.ManifestationId,bFoundGrade,Grade,Error) ||
               !bFoundGrade || Grade.ManifestationId!=Copy.ManifestationId ||
               Grade.GradeId!=Canonical->WorldNegationGradeId)
            {
                if(Error.IsEmpty())
                    Error=TEXT("Fantasm rejected: no matching canonical World Negation grade entitlement.");
                return false;
            }
            if(!Canonical->CheckRegisteredWorldFieldOwnership(
                   Copy.ManifestationId,FantasmId,Error))
            {
                if(Error.IsEmpty())
                    Error=TEXT("Fantasm rejected by live World authority/field ownership.");
                return false;
            }
            return true;
        });
    return true;
}
