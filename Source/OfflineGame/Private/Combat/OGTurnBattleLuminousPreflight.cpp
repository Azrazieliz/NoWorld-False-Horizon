#include "Combat/OGTurnBattle.h"
#include "Combat/OGApprovedLuminousSourceKit.h"
#include "Combat/OGLuminousAlterV1Projection.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const FOGContentId BaseIdentity(TEXT("character:luminous_valentine.identity"));
const FOGContentId BaseCoreSkill(TEXT("combat:luminous_valentine.base.skill.scarlet_cadence"));
const FOGContentId AlterCoreSkill(TEXT("draft:luminous_valentine.alter.active_1"));
bool FailPreflight(FString& Error,const TCHAR* Reason)
{
    Error=Reason;
    return false;
}
}

bool FOGTurnBattle::PrevalidateLuminousActionWithoutSideEffects(
    const FOGResolvedCombatAction& Action,
    const FOGCombatUnitState& Source,
    FString& OutError) const
{
    OutError.Reset();
    if(Source.IdentityId!=BaseIdentity)
        return true;

    const bool bBase=Source.SkillSet.ActiveSkills.Contains(BaseCoreSkill);
    const bool bAlter=Source.SkillSet.ActiveSkills.Contains(AlterCoreSkill);
    if(!bBase && !bAlter)
        return true; // Learned skills and future approved versions use own executors.
    if(bBase && bAlter)
        return FailPreflight(OutError,TEXT("Base and Alter cannot both be the same resolved version."));

    FOGResolvedSkillSet ApprovedSkills;
    FOGContentId WorldFantasm;
    int32 BaseCurrentReserve=0,AlterCurrentTithe=0,CurrentCharge=0;
    int32 OwnerAction=0;
    int32 BaseA2Cooldown=0,BaseA3Cooldown=0,AlterA2Cooldown=0,AlterA3Cooldown=0;
    int32 DiscountA2=0;
    bool bAllyRevivalSpent=false;
    if(bBase)
    {
        FOGApprovedLuminousSourceKitView Kit;
        if(!FOGApprovedLuminousSourceKit::LoadBaseV6(Kit,OutError))
            return false;
        ApprovedSkills=Kit.CoreSkills;
        WorldFantasm=Kit.WorldFantasmId;
        const FLuminousBaseBattleRuntime Own=LuminousBaseStates.FindRef(Source.UnitEntityId);
        BaseCurrentReserve=Own.ScarletReserve;
        CurrentCharge=Own.UltimateChargePercent;
        OwnerAction=Own.OwnerActions;
        BaseA2Cooldown=Own.NextA2ReadyOwnerAction;
        BaseA3Cooldown=Own.NextA3ReadyOwnerAction;
        DiscountA2=Own.DiscountNextA2Reserve;
        bAllyRevivalSpent=Own.bAllyRevivalSpent;
    }
    else
    {
        FOGLuminousAlterV1Projection Kit;
        if(!FOGLuminousAlterV1Projection::LoadApprovedV1(Kit,OutError))
            return false;
        ApprovedSkills=Kit.Skills;
        WorldFantasm=Kit.WorldFantasmId;
        const FLuminousAlterBattleRuntime Own=LuminousAlterStates.FindRef(Source.UnitEntityId);
        AlterCurrentTithe=Own.RedTithe;
        CurrentCharge=Own.UltimateChargePercent;
        OwnerAction=Own.OwnerActions;
        AlterA2Cooldown=Own.EucharistNextAvailableAction;
        AlterA3Cooldown=Own.CommunionNextAvailableAction;
    }
    const bool bA1=Action.SkillId==ApprovedSkills.ActiveSkills[0];
    const bool bA2=Action.SkillId==ApprovedSkills.ActiveSkills[1];
    const bool bA3=Action.SkillId==ApprovedSkills.ActiveSkills[2];
    const bool bUlt=Action.SkillId==ApprovedSkills.UltimateSkill;
    const bool bField=Action.SkillId==WorldFantasm;
    if(!bA1 && !bA2 && !bA3 && !bUlt && !bField)
        return true;
    if(Source.SkillSet.ActiveSkills.Num()!=3 ||
       Source.SkillSet.PassiveSkills.Num()!=6 ||
       Source.SkillSet.UltimateSkill!=ApprovedSkills.UltimateSkill)
        return FailPreflight(OutError,TEXT("Resolved Luminous character does not own the complete approved 3A+6P kit."));
    for(const FOGContentId& Id:ApprovedSkills.ActiveSkills)
        if(!Source.SkillSet.ActiveSkills.Contains(Id))
            return FailPreflight(OutError,TEXT("Resolved Luminous actor missing approved active."));
    for(const FOGContentId& Id:ApprovedSkills.PassiveSkills)
        if(!Source.SkillSet.PassiveSkills.Contains(Id))
            return FailPreflight(OutError,TEXT("Resolved Luminous actor missing approved passive."));

    if(bField)
    {
        if(!Source.CanAct() || !LuminousFieldEligibility)
            return FailPreflight(OutError,TEXT("No authoritative World Fantasm eligibility gate attached."));
        if(!LuminousFieldEligibility(Source,WorldFantasm,OutError))
        {
            if(OutError.IsEmpty())
                OutError=TEXT("Canonical Rank, Authority or field ownership rejected Fantasm.");
            return false;
        }
        if(const FLuminousFieldState* Old=LuminousFields.Find(Source.UnitEntityId))
            if(Old->ExpiresBeforeOwnerAction>=OwnerAction+1)
                return FailPreflight(OutError,TEXT("Manifestation's field already active."));
        if(bAlter)
        {
            if(AlterCurrentTithe<60)
                return FailPreflight(OutError,TEXT("Alter Cathedral requires 60 Red Tithe."));
        }
        else
        {
            const FLuminousBaseBattleRuntime Own=LuminousBaseStates.FindRef(Source.UnitEntityId);
            if(BaseCurrentReserve<60 || OwnerAction+1<Own.NextFantasmReadyOwnerAction)
                return FailPreflight(OutError,TEXT("Base Eclipse requires 60 Scarlet Reserve and legal field cooldown."));
            bool bPrepared=false;
            for(const auto& Entry:Own.HemalMarks)
                if(Entry.Value>0){bPrepared=true;break;}
            if(!bPrepared)
                for(const auto& Entry:Own.DawnOathByBeneficiary)
                    if(Entry.Value>=OwnerAction+1){bPrepared=true;break;}
            if(!bPrepared)
                return FailPreflight(OutError,TEXT("Eclipse requires an owned mark or active Dawn Oath."));
        }
        return true;
    }

    TSharedPtr<FJsonObject> Context;
    if(!FJsonSerializer::Deserialize(
          TJsonReaderFactory<>::Create(Action.ResolutionJson),Context) ||
       !Context.IsValid())
        return FailPreflight(OutError,TEXT("Luminous action requires valid branch/target JSON."));
    FString Branch;
    if(bUlt)
    {
        if(!Context->TryGetStringField(TEXT("branch"),Branch) ||
           (bBase && Branch!=TEXT("day") && Branch!=TEXT("night")) ||
           (bAlter && Branch!=TEXT("ruin") && Branch!=TEXT("harvest")))
            return FailPreflight(OutError,TEXT("Invalid explicit Luminous ultimate branch."));
    }
    FString Mode;
    if(bBase && (bA3 || bUlt))
        Context->TryGetStringField(TEXT("mode"),Mode);
    const bool bCovenant=bBase && bA3 && Mode==TEXT("covenant");
    const bool bDayRevivalPrepare=bBase && bUlt &&
        Branch==TEXT("day") && Mode==TEXT("revival_prepare");
    const int32 MaxTargets=bBase
        ? (bA1?1:bA2?3:bCovenant?1:3)
        : (bA1?1:bA2?4:bA3?1:
           Branch==TEXT("ruin")?4:1);
    if(Action.TargetUnitIds.IsEmpty() || Action.TargetUnitIds.Num()>MaxTargets)
        return FailPreflight(OutError,TEXT("Luminous action target count invalid."));
    TSet<FOGEntityId> Unique;
    for(const FOGEntityId& TargetId:Action.TargetUnitIds)
    {
        const FOGCombatUnitState* Target=FindUnit(TargetId);
        if(!Target || !Target->IsAlive() ||
           Target->Presence==EOGCombatPresence::Removed ||
           Unique.Contains(TargetId))
            return FailPreflight(OutError,TEXT("Luminous target absent, defeated or duplicated."));
        Unique.Add(TargetId);
        const bool bAllied=Target->TeamIndex==Source.TeamIndex;
        const bool bNeedsAlly=bBase &&
           (bA3 || bUlt && Branch==TEXT("day"));
        const bool bSelf=bAlter && bA3;
        if((bSelf && TargetId!=Source.UnitEntityId) ||
           (!bSelf && (bNeedsAlly!=bAllied)))
            return FailPreflight(OutError,TEXT("Luminous target allegiance invalid."));
    }
    if(bBase)
    {
        if(bA2 && OwnerAction+1<BaseA2Cooldown ||
           bA3 && OwnerAction+1<BaseA3Cooldown)
            return FailPreflight(OutError,TEXT("Base owner-action cooldown is not ready."));
        const int32 ReserveCost=bA2?FMath::Max(0,30-DiscountA2):
             bCovenant?75:bA3?30:0;
        if(BaseCurrentReserve<ReserveCost)
            return FailPreflight(OutError,TEXT("Insufficient Scarlet Reserve."));
        if((bCovenant || bDayRevivalPrepare) &&
           (bAllyRevivalSpent ||
            (bDayRevivalPrepare && Action.TargetUnitIds.Num()!=1) ||
            !IsLuminousRevivalEligible(Source.UnitEntityId,
                                     Action.TargetUnitIds[0],true)))
            return FailPreflight(OutError,TEXT("Prepared ally revival is used or denied by registered Soul/Authority gate."));
        if(bDayRevivalPrepare && BaseCurrentReserve<35)
            return FailPreflight(OutError,TEXT("Day Ultimate revival preparation costs 35 Reserve in addition to 100% charge."));
        const int32 ChargeCost=bUlt?(Branch==TEXT("day")?100:150):0;
        if(CurrentCharge<ChargeCost)
            return FailPreflight(OutError,TEXT("Insufficient Base Ultimate charge."));
    }
    else
    {
        if(bA2 && OwnerAction+1<AlterA2Cooldown ||
           bA3 && OwnerAction+1<AlterA3Cooldown)
            return FailPreflight(OutError,TEXT("Alter owner-action cooldown is not ready."));
        const int32 TitheCost=bA2?35:bA3?25:bUlt?
            (Branch==TEXT("ruin")?25:40):0;
        const int32 ChargeCost=bUlt?
            (Branch==TEXT("ruin")?100:150):0;
        const int32 SelfCostBps=bA1?200:bA2?400:bA3?800:bUlt?
            (Branch==TEXT("ruin")?500:800):0;
        const int32 Threshold=bA1||bA2?800:0;
        const bool bA3SourceHpEligible=!bA3 ||
            FOGLargeNumber::Compare(Source.CurrentHp,
                FOGLargeNumber::ScaleByBasisPoints(
                    Source.Stats.MaxHp,1500))>0;
        const FOGLargeNumber Remaining=FOGLargeNumber::Add(
            Source.CurrentHp,FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.MaxHp,-SelfCostBps));
        if(AlterCurrentTithe<TitheCost || CurrentCharge<ChargeCost ||
           !bA3SourceHpEligible ||
           FOGLargeNumber::Compare(Remaining,
               FOGLargeNumber::ScaleByBasisPoints(
                   Source.Stats.MaxHp,Threshold))<=0)
            return FailPreflight(OutError,
                TEXT("Alter lacks Tithe, charge, or safe actual HP for lawful cast."));
    }
    return true;
}
