#include "Combat/OGTurnBattle.h"
#include "Combat/OGLuminousAlterV1Projection.h"
#include "Combat/OGCombatMath.h"
#include "Random/OGDeterministicRng.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FOGLargeNumber AlterMinus(const FOGLargeNumber& N)
{ return FOGLargeNumber(-N.Significand,N.Exponent10); }
FOGLargeNumber AlterMin(const FOGLargeNumber& A,const FOGLargeNumber& B)
{ return FOGLargeNumber::Compare(A,B)<0?A:B; }
FOGLargeNumber AlterMissing(const FOGCombatUnitState& C)
{
    const FOGLargeNumber Missing=FOGLargeNumber::Add(C.Stats.MaxHp,AlterMinus(C.CurrentHp));
    return FOGLargeNumber::Compare(Missing,FOGLargeNumber())>0?Missing:FOGLargeNumber();
}
bool IsAlter(const FOGCombatUnitState& C)
{
    return C.IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"))
        && C.SkillSet.ActiveSkills.Contains(
            FOGContentId(TEXT("draft:luminous_valentine.alter.active_1")));
}
const FOGContentId A1(TEXT("draft:luminous_valentine.alter.active_1"));
const FOGContentId A2(TEXT("draft:luminous_valentine.alter.active_2"));
const FOGContentId A3(TEXT("draft:luminous_valentine.alter.active_3"));
const FOGContentId Ultimate(TEXT("draft:luminous_valentine.alter.ultimate.black_mass"));
}

bool FOGTurnBattle::ExecuteApprovedLuminousAlterAction(
    const FOGResolvedCombatAction& Action,FOGCombatUnitState& Source,
    FString& OutError)
{
    OutError.Reset();
    if(!IsAlter(Source))return true;
    // Execute only against the exact 35,551-byte user-approved Alter design
    // source. A draft content identifier is not installed Registry authority.
    FOGLuminousAlterV1Projection Approved;
    if(!FOGLuminousAlterV1Projection::LoadApprovedV1(Approved,OutError))
        return false;
    if(Approved.Skills.ActiveSkills[0]!=A1 ||
       Approved.Skills.ActiveSkills[1]!=A2 ||
       Approved.Skills.ActiveSkills[2]!=A3 ||
       Approved.Skills.UltimateSkill!=Ultimate)
    {
        OutError=TEXT("Alter v1 source IDs do not match the native executor binding.");
        return false;
    }
    const bool bA1=Action.SkillId==A1;
    const bool bA2=Action.SkillId==A2;
    const bool bA3=Action.SkillId==A3;
    const bool bUlt=Action.SkillId==Ultimate;
    const bool bField=Action.SkillId==Approved.WorldFantasmId;
    if(!bA1&&!bA2&&!bA3&&!bUlt&&!bField)return true;
    if(Source.SkillSet.ActiveSkills.Num()!=3 ||
       Source.SkillSet.PassiveSkills.Num()!=6 ||
       Source.SkillSet.UltimateSkill!=Ultimate ||
       !Source.SkillSet.ActiveSkills.Contains(A2) ||
       !Source.SkillSet.ActiveSkills.Contains(A3))
    {
        OutError=TEXT("Exact approved Alter v1 combat kit not present in source unit.");
        return false;
    }
    static const TCHAR* Passives[]={
        TEXT("draft:luminous_valentine.alter.passive_1"),
        TEXT("draft:luminous_valentine.alter.passive_2"),
        TEXT("draft:luminous_valentine.alter.passive_3"),
        TEXT("draft:luminous_valentine.alter.passive_4"),
        TEXT("draft:luminous_valentine.alter.passive_5"),
        TEXT("draft:luminous_valentine.alter.passive_6")
    };
    for(const TCHAR* Id:Passives)
        if(!Source.SkillSet.PassiveSkills.Contains(FOGContentId(Id)))
        {
            OutError=TEXT("Alter is missing a source-approved passive.");
            return false;
        }
    if(bField)
        return TryActivateLuminousAuthoredField(
            Source,Approved.WorldFantasmId,OutError);

    TSharedPtr<FJsonObject> Context;
    if(!FJsonSerializer::Deserialize(
        TJsonReaderFactory<>::Create(Action.ResolutionJson),Context) ||
       !Context.IsValid())
    {
        OutError=TEXT("Alter action requires a typed native resolution context.");
        return false;
    }
    FString Branch;
    if(bUlt && (!Context->TryGetStringField(TEXT("branch"),Branch) ||
       (Branch!=TEXT("ruin")&&Branch!=TEXT("harvest"))))
    {
        OutError=TEXT("Black Mass requires explicit Ruin or Hollow Crown selection.");
        return false;
    }
    const int32 MaxTargets=bA1?1:bA2?4:bA3?1:
        (Branch==TEXT("ruin")?4:1);
    if(Action.TargetUnitIds.Num()<1 ||
       Action.TargetUnitIds.Num()>MaxTargets)
    {
        OutError=TEXT("Alter authored target count violation.");
        return false;
    }
    if(bA3 && (Action.TargetUnitIds.Num()!=1 ||
                 Action.TargetUnitIds[0]!=Source.UnitEntityId))
    {
        OutError=TEXT("Martyr Communion is self-targeted, not a team heal.");
        return false;
    }
    TSet<FOGEntityId> Seen;
    for(const FOGEntityId& Id:Action.TargetUnitIds)
    {
        const FOGCombatUnitState* T=FindUnit(Id);
        if(!T || !T->IsAlive() || T->Presence==EOGCombatPresence::Removed ||
           Seen.Contains(Id) ||
           (bA3?(Id!=Source.UnitEntityId):(T->TeamIndex==Source.TeamIndex)))
        {
            OutError=TEXT("Alter target allegiance, life or uniqueness invalid.");
            return false;
        }
        Seen.Add(Id);
    }
    FLuminousAlterBattleRuntime& Own=LuminousAlterStates.FindOrAdd(
        Source.UnitEntityId);
    const int32 Current=Own.OwnerActions+1;
    if(bA2 && Current<Own.EucharistNextAvailableAction)
    {
        OutError=TEXT("Black Eucharist is on an authored 2-owner-action cooldown.");
        return false;
    }
    if(bA3 && Current<Own.CommunionNextAvailableAction)
    {
        OutError=TEXT("Martyr Communion is on an authored 3-owner-action cooldown.");
        return false;
    }
    const int32 CostRed=bA2?35:bA3?25:bUlt?(Branch==TEXT("ruin")?25:40):0;
    const int32 CostCharge=bUlt?(Branch==TEXT("ruin")?100:150):0;
    const int32 SelfCostBps=bA1?200:bA2?400:bA3?800:
        bUlt?(Branch==TEXT("ruin")?500:800):0;
    const int32 MinRemainingBps=bA1||bA2?800:0;
    const FOGLargeNumber SelfCost=FOGLargeNumber::ScaleByBasisPoints(
        Source.Stats.MaxHp,SelfCostBps);
    const FOGLargeNumber PostPayment=FOGLargeNumber::Add(
        Source.CurrentHp,AlterMinus(SelfCost));
    const bool bA3SourceHpEligible=!bA3 ||
        FOGLargeNumber::Compare(Source.CurrentHp,
            FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.MaxHp,1500))>0;
    if(Own.RedTithe<CostRed || Own.UltimateChargePercent<CostCharge ||
       !bA3SourceHpEligible ||
       FOGLargeNumber::Compare(PostPayment,
           FOGLargeNumber::ScaleByBasisPoints(Source.Stats.MaxHp,
                                              MinRemainingBps))<=0)
    {
        OutError=TEXT("Alter lacks Red Tithe, ultimate charge or safe HP for the lawful self-cost.");
        return false;
    }
    Own.RedTithe-=CostRed;
    Own.UltimateChargePercent-=CostCharge;
    Own.OwnerActions=Current;
    if(Current>Own.MomentumExpiresOwnerAction)
        Own.MartyrMomentum=0;
    if(!bA1 && !SelfCost.IsZero())
    {
        // A cost, not enemy damage. Never triggers reflected attacks,
        // enemy HP-loss resource loops, or free defeated-caster actions.
        Source.CurrentHp=PostPayment;
        Own.RedTithe=FMath::Min(100,Own.RedTithe+(bA1?8:bA2?15:bA3?20:0));
        Own.MartyrMomentum=FMath::Min(3,Own.MartyrMomentum+1);
        Own.MomentumExpiresOwnerAction=Current+2;
    }
    if(bA2)Own.EucharistNextAvailableAction=Current+3;
    if(bA3)Own.CommunionNextAvailableAction=Current+4;
    // The source's exact self-sacrificial standing buff never makes Alter
    // a substitute for Base's healer or player Ruler.
    if(bA3)
    {
        Own.MartyrStanceExpires=Current+2;
        Own.bMartyrNextFinisherPending=true;
        return true;
    }

    TSet<FOGEntityId> Eligible;
    for(const FOGEntityId& TargetId:Action.TargetUnitIds)
        if(IsLuminousBloodStatusEligible(Source.UnitEntityId,TargetId,
            AcceptedBleedFamilyId))
            Eligible.Add(TargetId);

    // Alter-owned Rupture Seals expire after three owner turns; an
    // expired seal cannot fuel a delayed bonus or World Fantasm contest.
    for(auto It=Own.RuptureSeals.CreateIterator();It;++It)
    {
        if(Own.SealExpiryOwnerAction.FindRef(It.Key())<Current)
        {
            Own.SealExpiryOwnerAction.Remove(It.Key());
            It.RemoveCurrent();
        }
    }
    // P1 Blood's Accounting: DIRECT SWORD AND REGISTERED WOUND TICKS share
    // one 16% MaxHP actual-restoration allowance per owner action. Previously
    // each wound tick healed outside the cap and could bypass DeathPrice.
    const FOGLargeNumber SiphonCap=FOGLargeNumber::ScaleByBasisPoints(
        Source.Stats.MaxHp,1600);
    FOGLargeNumber ActualSiphonRestored;
    auto SiphonWithinAllowance=[&](const FOGLargeNumber& Candidate)->bool
    {
        const FOGLargeNumber Remaining=FOGLargeNumber::Add(
            SiphonCap,AlterMinus(ActualSiphonRestored));
        if(FOGLargeNumber::Compare(Remaining,FOGLargeNumber())<=0 ||
           Candidate.IsZero())return true;
        const FOGLargeNumber Missing=AlterMissing(Source);
        if(Missing.IsZero())return true;
        // ApplyHealing owns the real target's authored healing-received
        // modifiers. Resolve their inverse BEFORE spending this allowance.
        int32 BuffAtk=0,BuffSpeed=0,BuffHealing=0,BuffReduction=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,
            BuffAtk,BuffSpeed,BuffHealing,BuffReduction);
        const FOGLargeNumber MaxNominal=FOGLargeNumber::ScaleByBasisPoints(
            Remaining,100000000/(10000+BuffHealing));
        FOGLargeNumber Heal=AlterMin(AlterMin(Candidate,Missing),MaxNominal);
        if(Current<=Own.DeathPriceExpiresOwnerAction)
            Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,7000);
        if(Heal.IsZero())return true;
        const FOGLargeNumber BeforeHeal=Source.CurrentHp;
        if(!ApplyHealing(Source.UnitEntityId,Source.UnitEntityId,
                        Heal,Action.SkillId,OutError))return false;
        const FOGLargeNumber Actual=FOGLargeNumber::Add(
            Source.CurrentHp,AlterMinus(BeforeHeal));
        if(FOGLargeNumber::Compare(Actual,FOGLargeNumber())>0)
            ActualSiphonRestored=FOGLargeNumber::Add(
                ActualSiphonRestored,Actual);
        return true;
    };

    // P4 Pursuit must be based on an actual final HP hit, not projected
    // damage, a shield interaction or another Manifestation's kill.
    auto RegisterPursuitOnDefeat=[&](const FOGEntityId& Victim)->void
    {
        const FOGCombatUnitState* Defeated=FindUnit(Victim);
        if(!Defeated || Defeated->IsAlive() ||
           Own.LastPursuitOwnerAction==Current ||
           Own.SeveringWounds.FindRef(Victim)<=0 ||
           Own.WoundExpiryOwnerAction.FindRef(Victim)<Current ||
           FOGLargeNumber::Compare(
               GetLastEffectiveHpDamage(Victim),FOGLargeNumber())<=0)
            return;
        Own.LastPursuitOwnerAction=Current;
        Own.bPursuitDelayPending=true;
    };

    // Registered Severing Wound ticks. End at source-approved 2 owner turns.
    for(auto It=Own.SeveringWounds.CreateIterator();It;++It)
    {
        if(Own.WoundExpiryOwnerAction.FindRef(It.Key())<Current)
        {
            Own.WoundExpiryOwnerAction.Remove(It.Key());
            It.RemoveCurrent();
            continue;
        }
        FOGCombatUnitState* T=FindMutableUnit(It.Key());
        if(!T || !T->IsAlive())continue;
        FOGLargeNumber Tick=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.Attack,1600*It.Value());
        // Martyr Crown P2: stance raises only the proven registered
        // Bleed-derived Severing Wound potency, never generic DoT.
        if(Current<=Own.MartyrStanceExpires)
            Tick=FOGLargeNumber::ScaleByBasisPoints(Tick,12200);
        if(!Tick.IsZero())
        {
            const FOGLargeNumber Before=T->CurrentHp;
            if(!ApplyDamage(Source.UnitEntityId,T->UnitEntityId,
                Tick,Action.SkillId,OutError))return false;
            const FOGLargeNumber Actual=GetLastEffectiveHpDamage(T->UnitEntityId);
            if(Actual.IsZero())continue;
            RegisterPursuitOnDefeat(T->UnitEntityId);
            Own.RedTithe=FMath::Min(100,Own.RedTithe+5);
            // 9% Wound Siphon uses the SAME allowance as direct damage.
            const FOGLargeNumber Heal=FOGLargeNumber::ScaleByBasisPoints(
                Actual,900);
            if(!SiphonWithinAllowance(Heal))return false;
        }
    }
    FOGDeterministicRng Rng(static_cast<uint64>(
        HashCombine(GetTypeHash(State.BattleId.ToString()),
                    GetTypeHash(Action.ActionId.ToString()))));
    FOGLargeNumber SiphonSum;
    auto Strike=[&](const FOGEntityId& Id,int32 AtkBps,
                    int32 SiphonRate,bool bSeal,bool bWound)->bool
    {
        // Source-owned hit provenance must not inherit a previous strike.
        LastEffectiveHpDamage.Remove(Id);
        FOGCombatUnitState* T=FindMutableUnit(Id);
        if(!T || !T->IsAlive())return true;
        FOGCombatStats Attacker=Source.Stats;
        int32 DawnAtk=0,DawnSpd=0,DawnHeal=0,DawnReduction=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,
            DawnAtk,DawnSpd,DawnHeal,DawnReduction);
        if(DawnAtk>0)
            Attacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                Attacker.Attack,10000+DawnAtk);
        if(DawnSpd>0)
            Attacker.Speed=Attacker.Speed*(10000+DawnSpd)/10000;
        Attacker.CritDamageBonusBps+=Own.MartyrMomentum*800;
        if(Current<=Own.MartyrStanceExpires)
        {
            // Source A3 Martyr Crown: +26% ATK is a stat effect, not an
            // unrelated +26% generic damage multiplier. Crit +18%, SPD
            // +12%, and Bleed-specific penetration remain independent.
            Attacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                Attacker.Attack,12600);
            Attacker.CritRateBps+=1800;
            Attacker.Speed=Attacker.Speed*11200/10000;
            if(Own.SeveringWounds.FindRef(Id)>0 &&
               Own.WoundExpiryOwnerAction.FindRef(Id)>=Current)
                Attacker.DefensePenetrationBps+=1200;
        }
        if(Current<=Own.DeathPriceExpiresOwnerAction)
            Attacker.CritDamageBonusBps+=2000;
        FOGDamageRequest Req;
        Req.BaseDamage=FOGLargeNumber::ScaleByBasisPoints(
            Attacker.Attack,AtkBps);
        Req.DefenseReference=Source.Stats.Attack;
        Req.bAllowHitOverflowReplication=false;
        // Martyr Crown is already included in Attacker's actual ATK,
        // CritRate, Speed and conditional Bleed penetration; don't apply
        // a second duplicate +26% damage multiplier.
        if(Own.SeveringWounds.FindRef(Id)>=2)
            Req.AdditionalMultiplierBps.Add(11400);
        if(FOGLargeNumber::Compare(Source.CurrentHp,
            FOGLargeNumber::ScaleByBasisPoints(Source.Stats.MaxHp,4000))<=0 &&
           Own.SeveringWounds.FindRef(Id)>0)
            Req.AdditionalMultiplierBps.Add(11200);
        const FOGDamageResolution D=FOGCombatMath::ResolveDamage(
            Req,Attacker,T->Stats,Rng);
        if(D.TotalDamage.IsZero())return true;
        const FOGLargeNumber Before=T->CurrentHp;
        if(!ApplyDamage(Source.UnitEntityId,Id,D.TotalDamage,
                        Action.SkillId,OutError))return false;
        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(Id);
        if(!Actual.IsZero())
            RegisterPursuitOnDefeat(Id);
        if(!Actual.IsZero()&&Eligible.Contains(Id))
        {
            if(bSeal)
            {
                Own.RuptureSeals.FindOrAdd(Id)=
                    FMath::Min(3,Own.RuptureSeals.FindRef(Id)+1);
                Own.SealExpiryOwnerAction.Add(Id,Current+3);
            }
            if(bWound)
            {
                Own.SeveringWounds.FindOrAdd(Id)=
                    FMath::Min(2,Own.SeveringWounds.FindRef(Id)+1);
                Own.WoundExpiryOwnerAction.Add(Id,Current+2);
            }
            SiphonSum=FOGLargeNumber::Add(SiphonSum,
                FOGLargeNumber::ScaleByBasisPoints(Actual,SiphonRate));
        }
        return true;
    };
    if(bA1)
    {
        const FOGEntityId Id=Action.TargetUnitIds[0];
        constexpr int32 Steps[]={4800,6400,10500};
        bool bActualThirdHit=false;
        for(int32 I=0;I<3;++I)
        {
            const int32 Before=Own.RuptureSeals.FindRef(Id);
            if(!Strike(Id,Steps[I],1800,I==0||I==2,I==2 && Before>=2))
                return false;
            if(I==2)
                bActualThirdHit=FOGLargeNumber::Compare(
                    GetLastEffectiveHpDamage(Id),FOGLargeNumber())>0;
            if(I==2 && Before>=3 && bActualThirdHit)
            {
                Own.RuptureSeals.Remove(Id);
                Own.SealExpiryOwnerAction.Remove(Id);
                if(!Strike(Id,7500,1800,false,false))return false;
            }
        }
        if(Own.bMartyrNextFinisherPending &&
           Current<=Own.MartyrStanceExpires && bActualThirdHit)
        {
            Own.bMartyrNextFinisherPending=false;
            if(!Strike(Id,5000,1800,false,false))return false;
        }
        // The v1 vitality offering is committed AFTER the third blade
        // swing, once even if that swing missed. It is not free opening
        // Momentum on the earlier two strikes.
        const FOGLargeNumber FinalPost=FOGLargeNumber::Add(
            Source.CurrentHp,AlterMinus(SelfCost));
        if(FOGLargeNumber::Compare(FinalPost,FOGLargeNumber())>0)
        {
            Source.CurrentHp=FinalPost;
            Own.RedTithe=FMath::Min(100,Own.RedTithe+8);
            Own.MartyrMomentum=FMath::Min(3,Own.MartyrMomentum+1);
            Own.MomentumExpiresOwnerAction=Current+2;
        }
    }
    else if(bA2)
    {
        const bool bFinisher=Own.RuptureSeals.FindRef(Action.TargetUnitIds[0])>=3;
        if(bFinisher)
        {
            Own.RuptureSeals.Remove(Action.TargetUnitIds[0]);
            Own.SealExpiryOwnerAction.Remove(Action.TargetUnitIds[0]);
        }
        for(const FOGEntityId& Id:Action.TargetUnitIds)
        {
            if(!Strike(Id,9800,1800,false,true) ||
               !Strike(Id,11200,1800,false,false))return false;
        }
        if(bFinisher && !Strike(Action.TargetUnitIds[0],
                               10000,1800,false,false))return false;
    }
    else if(bUlt && Branch==TEXT("ruin"))
    {
        // Approved Ruin branch grants +10 Tithe once ONLY if its primary
        // genuinely suffered at least one Ruin hit and died from the cast.
        const FOGEntityId Primary=Action.TargetUnitIds[0];
        const FOGCombatUnitState* PrimaryBefore=FindUnit(Primary);
        const bool bPrimaryAliveBeforeRuin=PrimaryBefore && PrimaryBefore->IsAlive();
        bool bPrimaryRuinHpHit=false;
        constexpr int32 Steps[]={14500,14500,8000};
        for(const FOGEntityId& Id:Action.TargetUnitIds)
            for(const int32 Step:Steps)
            {
                if(!Strike(Id,Step,1800,false,Step==8000))return false;
                if(Id==Primary &&
                   FOGLargeNumber::Compare(GetLastEffectiveHpDamage(Id),
                                             FOGLargeNumber())>0)
                    bPrimaryRuinHpHit=true;
            }
        const FOGCombatUnitState* PrimaryAfter=FindUnit(Primary);
        if(bPrimaryAliveBeforeRuin && bPrimaryRuinHpHit && PrimaryAfter &&
           !PrimaryAfter->IsAlive())
            Own.RedTithe=FMath::Min(100,Own.RedTithe+10);
    }
    else if(bUlt)
    {
        const FOGEntityId Id=Action.TargetUnitIds[0];
        const bool bEligibleFinisher=Own.RuptureSeals.FindRef(Id)>=3;
        bool bFirstHitConfirmed=false;
        constexpr int32 Steps[]={5500,6500,7500,9000,10500,13500};
        for(int32 I=0;I<6;++I)
        {
            int32 Scale=Steps[I];
            if(I>=4 && FOGLargeNumber::Compare(Source.CurrentHp,
                FOGLargeNumber::ScaleByBasisPoints(Source.Stats.MaxHp,4000))<=0)
                Scale=Scale*12000/10000;
            if(!Strike(Id,Scale,2200,false,false))return false;
            if(I==0 && bEligibleFinisher &&
               FOGLargeNumber::Compare(GetLastEffectiveHpDamage(Id),
                                         FOGLargeNumber())>0)
            {
                // Hollow Crown v1 consumes three seals on a successful
                // first hit ONLY; no mark loss on blocked/missed opener.
                bFirstHitConfirmed=true;
                Own.RuptureSeals.Remove(Id);
                Own.SealExpiryOwnerAction.Remove(Id);
            }
        }
        if(bFirstHitConfirmed &&
           !Strike(Id,13000,2200,false,false))return false;
    }
    // Remaining direct sword siphon (18%, Harvest 22%) is applied once
    // using the unused portion of the exact same owner-action HP cap.
    if(!SiphonWithinAllowance(SiphonSum))return false;
    return true;
}

void FOGTurnBattle::TryResolveApprovedLuminousAlterRevival(
    const FOGEntityId& UnitId)
{
    FLuminousAlterBattleRuntime* R=LuminousAlterStates.Find(UnitId);
    FOGCombatUnitState* U=FindMutableUnit(UnitId);
    if(!R || !U || !IsAlter(*U) || R->bSelfRevived || R->RedTithe<75 ||
       !IsLuminousRevivalEligible(UnitId,UnitId,false) ||
       U->Presence!=EOGCombatPresence::Defeated ||
       U->OccupiedLaneIndex==INDEX_NONE)return;
    const FOGLargeNumber Restore=FOGLargeNumber::ScaleByBasisPoints(
        U->Stats.MaxHp,1800);
    FString Error;
    if(Restore.IsZero() ||
       !ApplyApprovedLuminousRevivalHealing(UnitId,UnitId,Restore,
            FOGContentId(TEXT("draft:luminous_valentine.alter.passive_5")),Error))
        return;
    if(!ChangePresence(UnitId,EOGCombatPresence::Active,Error))return;
    R->RedTithe-=75;
    R->bSelfRevived=true;
    R->DeathPriceExpiresOwnerAction=R->OwnerActions+1;
}
