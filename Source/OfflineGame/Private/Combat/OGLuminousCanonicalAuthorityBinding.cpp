#include "Combat/OGLuminousCanonicalAuthorityBinding.h"
#include "Persistence/OGWorldStore.h"
#include "Combat/OGApprovedLuminousSourceKit.h"
#include "Combat/OGLuminousAlterV1Projection.h"

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
           Out.ActiveVersionId==Unit.SkillSet.CurrentForm &&
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

    // A unit may never borrow the other approved version's actions, resource
    // account or World Fantasm. The immutable design is a validation source,
    // not an unregistered package grant.
    FOGApprovedLuminousSourceKitView BaseKit;
    FOGLuminousAlterV1Projection AlterKit;
    if(!FOGApprovedLuminousSourceKit::LoadBaseV6(BaseKit,OutError) ||
       !FOGLuminousAlterV1Projection::LoadApprovedV1(AlterKit,OutError))
        return false;
    auto MatchesKit=[](const FOGResolvedSkillSet& Owned,
                       const FOGResolvedSkillSet& Approved)
    {
        if(Owned.ActiveSkills.Num()!=3 || Owned.PassiveSkills.Num()!=6 ||
           Owned.UltimateSkill!=Approved.UltimateSkill ||
           Owned.HasDuplicateSkills())return false;
        for(const FOGContentId& Skill:Approved.ActiveSkills)
            if(!Owned.ActiveSkills.Contains(Skill))return false;
        for(const FOGContentId& Skill:Approved.PassiveSkills)
            if(!Owned.PassiveSkills.Contains(Skill))return false;
        return true;
    };
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
        const bool bBase=Record.ActiveVersionId==BaseKit.VersionId &&
            MatchesKit(Unit.SkillSet,BaseKit.CoreSkills);
        const bool bAlter=Record.ActiveVersionId==AlterKit.DraftVersionId &&
            MatchesKit(Unit.SkillSet,AlterKit.Skills);
        if(bBase==bAlter)
        {
            OutError=TEXT("Luminous live version, skillset and exactly one approved 3A/6P kit disagree; no cross-version abilities.");
            return false;
        }
        const FOGContentId ExpectedFantasm=bBase
            ? BaseKit.WorldFantasmId:AlterKit.WorldFantasmId;
        const FOGContentId* InstalledFantasm=
            Sources.FantasmByRegisteredVersion.Find(Record.ActiveVersionId);
        if(!InstalledFantasm || *InstalledFantasm!=ExpectedFantasm)
        {
            OutError=TEXT("Luminous active version has no exact installed Base/Alter World Fantasm registration.");
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
