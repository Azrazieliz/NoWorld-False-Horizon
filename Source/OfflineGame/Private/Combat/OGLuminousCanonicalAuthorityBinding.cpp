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
       Battle.GetState().Status!=EOGTurnBattleStatus::Running)
    {
        OutError=TEXT("Cannot bind combat authority without a live owned WorldStore battle.");
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

    // Gate 11 was dissolved. The user-approved locked Base/Alter designs
    // supply their own real identity, Version, skillset and Fantasm mapping;
    // no missing package install or synthesized Registry record is required.
    const TMap<FOGContentId,FOGContentId> AuthoredFantasmByVersion={
        {BaseKit.VersionId,BaseKit.WorldFantasmId},
        {AlterKit.DraftVersionId,AlterKit.WorldFantasmId}
    };
    if(Sources.FantasmByRegisteredVersion.IsEmpty())
        Sources.FantasmByRegisteredVersion=AuthoredFantasmByVersion;
    else
    {
        for(const auto& Pair:AuthoredFantasmByVersion)
            if(const FOGContentId* Found=Sources.FantasmByRegisteredVersion.Find(Pair.Key))
                if(*Found!=Pair.Value)
                {
                    OutError=TEXT("External Fantasm mapping conflicts with approved Base/Alter sources.");
                    return false;
                }
    }
    if(!Sources.ReadInstalledIdentity)
    {
        Sources.ReadInstalledIdentity=[IdentityId=BaseKit.IdentityId](
            const FOGContentId& Query,FOGCharacterIdentityDefinition& Identity,
            FString& Error)
        {
            if(Query!=IdentityId)
            {
                Error=TEXT("No approved Luminous identity for this source ID.");
                return false;
            }
            Identity=FOGCharacterIdentityDefinition();
            Identity.IdentityId=IdentityId;
            // MR Origin is explicitly approved in the locked Base v6 and
            // inherited by Alter. Current MR is checked independently in store.
            Identity.OriginRarity=FName(TEXT("MR"));
            return true;
        };
    }
    // Native battle state is authoritative for ownership/teams/defeat. A
    // missing species-specific blood contract must NEVER fabricate blood.
    if(!Sources.RegisteredBleedFamilyId.IsValid())
        Sources.RegisteredBleedFamilyId=FOGContentId(
            TEXT("combat:luminous_valentine.base.status.hemorrhage"));
    if(!Sources.ReadNativeBloodEligibility)
        Sources.ReadNativeBloodEligibility=[](
            const FOGCombatUnitState&,const FOGCombatUnitState&,
            const FOGContentId&){return false;};
    if(!Sources.ReadNativeSoulContinuity)
        Sources.ReadNativeSoulContinuity=[](
            const FOGCombatUnitState& Restoring,const FOGCombatUnitState& Target,
            bool bAlly)
        {
            // Defeated is a battlefield status, not canonical death. Removed
            // entities have no continuous World presence and cannot revive.
            return Restoring.UnitEntityId.IsValid() && Target.UnitEntityId.IsValid() &&
                   (bAlly ? Restoring.TeamIndex==Target.TeamIndex :
                    Restoring.UnitEntityId==Target.UnitEntityId) &&
                   Target.Presence!=EOGCombatPresence::Removed;
        };
    if(!Sources.ValidateRegisteredEffectContest)
        Sources.ValidateRegisteredEffectContest=[](
            const FOGLuminousRegisteredEffectContest&,FString& Error)
        {
            Error=TEXT("No independently registered live effect instance to contest.");
            return false;
        };
    if(!Sources.CheckRegisteredWorldFieldOwnership)
        Sources.CheckRegisteredWorldFieldOwnership=[
            &Store,OwningRulerId,AuthoredFantasmByVersion](
            const FOGEntityId& Manifestation,const FOGContentId& Fantasm,
            FString& Error)
        {
            bool bFound=false;
            FOGCharacterManifestationRecord Owned;
            if(!Store.TryReadCharacterManifestation(
                   Manifestation,bFound,Owned,Error) || !bFound ||
               Owned.ManifestationId!=Manifestation ||
               Owned.OwningRulerId!=OwningRulerId ||
               Owned.LifecycleState!=FName(TEXT("active")) ||
               AuthoredFantasmByVersion.FindRef(Owned.ActiveVersionId)!=Fantasm)
            {
                if(Error.IsEmpty())
                    Error=TEXT("Manifestation is not the active native owner of this Fantasm.");
                return false;
            }
            bool bGrade=false;
            FOGManifestationWorldFantasmStateRecord State;
            if(!Store.TryReadManifestationWorldFantasmState(
                   Manifestation,bGrade,State,Error) || !bGrade ||
               State.ManifestationId!=Manifestation ||
               !State.GradeId.IsValid() ||
               State.ExpressionProfileId!=Fantasm)
            {
                if(Error.IsEmpty())
                    Error=TEXT("No real WorldStore Fantasm expression for this Manifestation.");
                return false;
            }
            return true;
        };

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
            OutError=TEXT("Luminous battle unit has no active, owned WorldStore Manifestation.");
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
            OutError=TEXT("Luminous active version has no exact approved Base/Alter World Fantasm mapping.");
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
                Error=TEXT("Fantasm rejected: live WorldStore owner/identity/version differs.");
                return false;
            }
            const FOGContentId* RegisteredFantasm=
                Canonical->FantasmByRegisteredVersion.Find(Copy.ActiveVersionId);
            if(!RegisteredFantasm || *RegisteredFantasm!=FantasmId)
            {
                Error=TEXT("Fantasm rejected: selected ability does not belong to the approved active version.");
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
                    Error=TEXT("Fantasm rejected: approved immutable Origin Rarity is not MR.");
                return false;
            }
            bool bFoundGrade=false;
            FOGManifestationWorldFantasmStateRecord Grade;
            if(!Store.TryReadManifestationWorldFantasmState(
                   Copy.ManifestationId,bFoundGrade,Grade,Error) ||
               !bFoundGrade || Grade.ManifestationId!=Copy.ManifestationId ||
               !Grade.GradeId.IsValid() ||
               Grade.ExpressionProfileId!=FantasmId ||
               (Canonical->WorldNegationGradeId.IsValid() &&
                Grade.GradeId!=Canonical->WorldNegationGradeId))
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
