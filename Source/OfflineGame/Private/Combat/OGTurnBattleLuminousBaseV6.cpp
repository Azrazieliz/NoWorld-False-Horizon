#include "Combat/OGTurnBattle.h"
#include "Combat/OGApprovedLuminousSourceKit.h"
#include "Combat/OGCombatMath.h"
#include "Random/OGDeterministicRng.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FOGLargeNumber Minus(const FOGLargeNumber& Value)
{
    return FOGLargeNumber(-Value.Significand, Value.Exponent10);
}
FOGLargeNumber MinNumber(const FOGLargeNumber& A, const FOGLargeNumber& B)
{
    return FOGLargeNumber::Compare(A,B)<0?A:B;
}
FOGLargeNumber PositiveMissing(const FOGCombatUnitState& U)
{
    const FOGLargeNumber V=FOGLargeNumber::Add(
        U.Stats.MaxHp,Minus(U.CurrentHp));
    return FOGLargeNumber::Compare(V,FOGLargeNumber())>0
        ? V:FOGLargeNumber();
}
bool IsBaseLuminous(const FOGCombatUnitState& Unit)
{
    // Base and Alter are distinct versions of one Identity. Never infer
    // Base just because a runtime actor lacks an optional CurrentForm field.
    return Unit.IdentityId ==
        FOGContentId(TEXT("character:luminous_valentine.identity")) &&
        Unit.SkillSet.ActiveSkills.Contains(
            FOGContentId(TEXT("combat:luminous_valentine.base.skill.scarlet_cadence")));
}
}

bool FOGTurnBattle::ExecuteApprovedLuminousBaseAction(
    const FOGResolvedCombatAction& Action,
    FOGCombatUnitState& Source,
    FString& OutError)
{
    OutError.Reset();
    if (!IsBaseLuminous(Source))
        return true;

    FOGApprovedLuminousSourceKitView Kit;
    if (!FOGApprovedLuminousSourceKit::LoadBaseV6(Kit,OutError))
        return false;
    const bool bA1=Action.SkillId==Kit.CoreSkills.ActiveSkills[0];
    const bool bA2=Action.SkillId==Kit.CoreSkills.ActiveSkills[1];
    const bool bA3=Action.SkillId==Kit.CoreSkills.ActiveSkills[2];
    const bool bUlt=Action.SkillId==Kit.CoreSkills.UltimateSkill;
    const bool bField=Action.SkillId==Kit.WorldFantasmId;
    if (!bA1 && !bA2 && !bA3 && !bUlt && !bField)
        return true; // Other skill definitions use the existing content executor.

    if (Source.SkillSet.ActiveSkills.Num()!=3 ||
        Source.SkillSet.PassiveSkills.Num()!=6 ||
        Source.SkillSet.UltimateSkill!=Kit.CoreSkills.UltimateSkill)
    {
        OutError=TEXT("Owned Luminous combat snapshot has no exact approved Base v6 kit.");
        return false;
    }
    for(int32 I=0;I<3;++I)
        if (!Source.SkillSet.ActiveSkills.Contains(Kit.CoreSkills.ActiveSkills[I]))
        {
            OutError=TEXT("Base v6 active skill absent from combat snapshot.");
            return false;
        }
    for(const FOGContentId& P:Kit.CoreSkills.PassiveSkills)
        if(!Source.SkillSet.PassiveSkills.Contains(P))
        {
            OutError=TEXT("Base v6 passive missing; refusing partial skill effects.");
            return false;
        }

    if(bField)
        return TryActivateLuminousAuthoredField(Source,Kit.WorldFantasmId,OutError);

    TSharedPtr<FJsonObject> Context;
    if (!FJsonSerializer::Deserialize(
            TJsonReaderFactory<>::Create(Action.ResolutionJson),Context) ||
        !Context.IsValid())
    {
        OutError=TEXT("A Luminous action requires a structured resolution context.");
        return false;
    }

    FString A3Mode;
    if(bA3)Context->TryGetStringField(TEXT("mode"),A3Mode);
    const bool bA3Covenant=bA3 && A3Mode==TEXT("covenant");
    FString UltimateMode;
    if(bUlt)Context->TryGetStringField(TEXT("mode"),UltimateMode);
    const int32 MaxTargets=bA1?1:bA2?3:bA3Covenant?1:3;
    if(Action.TargetUnitIds.IsEmpty() || Action.TargetUnitIds.Num()>MaxTargets)
    {
        OutError=TEXT("Luminous action target count violates its approved targeting rule.");
        return false;
    }
    TSet<FOGEntityId> Unique;
    for(const FOGEntityId& Id:Action.TargetUnitIds)
    {
        const FOGCombatUnitState* Target=FindUnit(Id);
        if (!Id.IsValid() || !Target || Unique.Contains(Id) ||
            Target->Presence==EOGCombatPresence::Removed ||
            ((bA1||bA2) && (Target->TeamIndex==Source.TeamIndex || !Target->IsAlive())) ||
            (bA3 && (Target->TeamIndex!=Source.TeamIndex || !Target->IsAlive())))
        {
            OutError=TEXT("Invalid allied/enemy target, duplicate or defeated actor.");
            return false;
        }
        Unique.Add(Id);
    }

    FString UltimateBranch;
    if(bA3Covenant &&
       !IsLuminousRevivalEligible(Source.UnitEntityId,
                                 Action.TargetUnitIds[0],true))
    {
        OutError=TEXT("A3 Covenant preparation denied by authoritative native soul and superior Authority rules.");
        return false;
    }
    if(bUlt)
    {
        if (!Context->TryGetStringField(TEXT("branch"),UltimateBranch) ||
            (UltimateBranch!=TEXT("day") && UltimateBranch!=TEXT("night")))
        {
            OutError=TEXT("Day/Night ultimate requires explicit player branch selection.");
            return false;
        }
        for(const FOGEntityId& Id:Action.TargetUnitIds)
        {
            const FOGCombatUnitState* T=FindUnit(Id);
            if(!T || !T->IsAlive() ||
               (UltimateBranch==TEXT("day"))!=(T->TeamIndex==Source.TeamIndex))
            {
                OutError=TEXT("Selected ultimate branch has illegal target allegiance.");
                return false;
            }
        }
    }

    const bool bDayRevivalPrepare=bUlt &&
        UltimateBranch==TEXT("day") &&
        UltimateMode==TEXT("revival_prepare");
    if(bDayRevivalPrepare)
    {
        if(Action.TargetUnitIds.Num()!=1 ||
           !IsLuminousRevivalEligible(Source.UnitEntityId,
                Action.TargetUnitIds[0],true))
        {
            OutError=TEXT("Day Ultimate revival must select one living soul-continuous authorized ally BEFORE defeat.");
            return false;
        }
    }
    FLuminousBaseBattleRuntime& StateOfOwner=
        LuminousBaseStates.FindOrAdd(Source.UnitEntityId);
    const int32 ChargeCost=bUlt
        ? (UltimateBranch==TEXT("day")?Kit.DayUltimateCostPercent:Kit.NightUltimateCostPercent)
        : 0;
    if(ChargeCost>StateOfOwner.UltimateChargePercent)
    {
        OutError=TEXT("Manifestation lacks its own required ultimate charge.");
        return false;
    }
    const int32 NextOwnerAction=StateOfOwner.OwnerActions+1;
    if((bA2 && NextOwnerAction<StateOfOwner.NextA2ReadyOwnerAction) ||
       (bA3 && NextOwnerAction<StateOfOwner.NextA3ReadyOwnerAction))
    {
        OutError=TEXT("Approved Base v6 two/three-owner-action cooldown not ready.");
        return false;
    }
    const int32 BaseReserveCost=bA2?
        FMath::Max(0,30-StateOfOwner.DiscountNextA2Reserve):
        bA3Covenant?75:bA3?30:0;
    if(BaseReserveCost>StateOfOwner.ScarletReserve)
    {
        OutError=TEXT("Base v6 insufficient Scarlet Reserve for actual skill cost.");
        return false;
    }
    if((bA3Covenant || bDayRevivalPrepare) &&
       StateOfOwner.bAllyRevivalSpent)
    {
        OutError=TEXT("The shared ally revival ledger was already consumed in this encounter.");
        return false;
    }
    if(bDayRevivalPrepare && StateOfOwner.ScarletReserve<35)
    {
        OutError=TEXT("Day Ultimate prepared revival needs 35 Scarlet Reserve in addition to its Ultimate charge.");
        return false;
    }
    if(bA3Covenant && StateOfOwner.ScarletReserve<75)
    {
        OutError=TEXT("A3 Covenant requires 75 actual Scarlet Reserve at preparation.");
        return false;
    }
    // A2 Bloodless Cost is lawful only when a selected enemy lacks the
    // canonical blood family and Luminous has no already stored valid blood.
    // The 2% current MaxHP sacrificial cost cannot itself defeat the caster.
    bool bA2NeedsBloodlessCost=false;
    if(bA2)
    {
        bool bStoredBlood=false;
        for(const auto& Row:StateOfOwner.HemalMarks)
            if(Row.Value>0 &&
               StateOfOwner.HemalMarkExpiryOwnerAction.FindRef(Row.Key)>=
                   NextOwnerAction){bStoredBlood=true;break;}
        if(!bStoredBlood)
            for(const auto& Row:StateOfOwner.HemorrhageStacks)
                if(Row.Value>0 &&
                   StateOfOwner.HemorrhageEndOwnerAction.FindRef(Row.Key)>=
                       NextOwnerAction){bStoredBlood=true;break;}
        if(!bStoredBlood)
            for(const FOGEntityId& Id:Action.TargetUnitIds)
                if(!IsLuminousBloodStatusEligible(
                    Source.UnitEntityId,Id,AcceptedBleedFamilyId))
                {
                    bA2NeedsBloodlessCost=true;
                    break;
                }
        if(bA2NeedsBloodlessCost)
        {
            const FOGLargeNumber SelfCost=FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.MaxHp,200);
            const FOGLargeNumber Remaining=FOGLargeNumber::Add(
                Source.CurrentHp,Minus(SelfCost));
            if(FOGLargeNumber::Compare(Remaining,FOGLargeNumber())<=0)
            {
                OutError=TEXT("Bloodless Hematic Crucible cannot sacrifice lethal HP.");
                return false;
            }
            Source.CurrentHp=Remaining;
        }
    }
    if(bA3Covenant)
    {
        // Both portions, 30 + 45, are committed at preparation (not death).
        StateOfOwner.ScarletReserve-=75;
        StateOfOwner.PreparedCovenantTarget=Action.TargetUnitIds[0];
        StateOfOwner.CovenantExpiryOwnerAction=StateOfOwner.OwnerActions+3;
        StateOfOwner.bDayAttunement=true;
        StateOfOwner.bAttunementActive=true;
        StateOfOwner.AttunementExpiryAction=StateOfOwner.OwnerActions+3;
    }
    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;
    if(bDayRevivalPrepare)
    {
        StateOfOwner.ScarletReserve-=35;
        StateOfOwner.PreparedDayUltimateRevivalTarget=
            Action.TargetUnitIds[0];
        StateOfOwner.DayUltimateRevivalExpiryOwnerAction=
            NextOwnerAction+2;
    }
    if(bA2)
    {
        StateOfOwner.ScarletReserve-=BaseReserveCost;
        StateOfOwner.DiscountNextA2Reserve=0;
        StateOfOwner.NextA2ReadyOwnerAction=NextOwnerAction+3;
    }
    else if(bA3)
    {
        if(!bA3Covenant)StateOfOwner.ScarletReserve-=BaseReserveCost;
        StateOfOwner.NextA3ReadyOwnerAction=NextOwnerAction+4;
    }

    // Own-action regeneration: 4% of missing HP, living actor only.
    // No regeneration from corpse/succession or source combat-log replay.
    if(Source.IsAlive())
    {
        const FOGLargeNumber Restore=FOGLargeNumber::ScaleByBasisPoints(
            PositiveMissing(Source),400);
        if(!Restore.IsZero() &&
           !ApplyHealing(Source.UnitEntityId,Source.UnitEntityId,
                         Restore,Action.SkillId,OutError))
            return false;
    }

    ++StateOfOwner.OwnerActions;
    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;
    if(bDayRevivalPrepare)
        return true; // One legal preparedness action, no simultaneous AoE heal.
    if(StateOfOwner.bAttunementActive &&
       CurrentOwnerAction>StateOfOwner.AttunementExpiryAction)
        StateOfOwner.bAttunementActive=false; // Neutral, neither Night nor Day.
    // Source-authoritative Base v6 status lifetimes: Hemal Marks 3 owner
    // turns, Eclipse Claim 2, Dawn Oath 2. Never retain a hidden expired
    // mark after an enemy switches lane or the owner exits Day/Night.
    for(auto It=StateOfOwner.HemalMarks.CreateIterator();It;++It)
        if(StateOfOwner.HemalMarkExpiryOwnerAction.FindRef(It.Key())<
           CurrentOwnerAction)
        {
            StateOfOwner.HemalMarkExpiryOwnerAction.Remove(It.Key());
            StateOfOwner.PendingFourMarkTargetBonus.Remove(It.Key());
            It.RemoveCurrent();
        }
    for(auto It=StateOfOwner.EclipseClaimTargets.CreateIterator();It;++It)
        if(StateOfOwner.EclipseClaimExpiryOwnerAction.FindRef(*It)<
           CurrentOwnerAction)
        {
            StateOfOwner.EclipseClaimExpiryOwnerAction.Remove(*It);
            It.RemoveCurrent();
        }
    for(auto It=StateOfOwner.DawnOathByBeneficiary.CreateIterator();It;++It)
        if(It.Value()<CurrentOwnerAction)
            It.RemoveCurrent();

    // Status effects belong to this Manifestation, never all characters who
    // share the Luminous Identity. Resolve actual HP damage before any siphon.
    TArray<FOGEntityId> Expired;
    for(auto& Entry:StateOfOwner.HemorrhageStacks)
    {
        if(StateOfOwner.HemorrhageEndOwnerAction.FindRef(Entry.Key)<CurrentOwnerAction)
        {
            Expired.Add(Entry.Key);
            continue;
        }
        FOGCombatUnitState* T=FindMutableUnit(Entry.Key);
        if(!T || !T->IsAlive()){Expired.Add(Entry.Key);continue;}
        const FOGLargeNumber Before=T->CurrentHp;
        const FOGLargeNumber Bleed=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.Attack,1800*Entry.Value);
        if(!Bleed.IsZero() &&
           !ApplyDamage(Source.UnitEntityId,T->UnitEntityId,
               Bleed,Action.SkillId,OutError)) return false;
        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(T->UnitEntityId);
        const FOGLargeNumber Siphon=FOGLargeNumber::ScaleByBasisPoints(Actual,1200);
        const FOGLargeNumber Limited=MinNumber(Siphon,PositiveMissing(Source));
        if(!Limited.IsZero() &&
           !ApplyHealing(Source.UnitEntityId,Source.UnitEntityId,
                         Limited,Action.SkillId,OutError))return false;
        if(StateOfOwner.LastBleedReserveOwnerAction!=CurrentOwnerAction &&
           !Actual.IsZero())
        {
            StateOfOwner.ScarletReserve=FMath::Min(
                Kit.ScarletReserveMax,StateOfOwner.ScarletReserve+6);
            StateOfOwner.LastBleedReserveOwnerAction=CurrentOwnerAction;
        }
    }
    for(const FOGEntityId& E:Expired)
    {
        StateOfOwner.HemorrhageStacks.Remove(E);
        StateOfOwner.HemorrhageEndOwnerAction.Remove(E);
    }

    // Only the actual simulation's proven blood-bearing targets can receive
    // Hemal Marks/Hemorrhage/siphon. Unclassified enemies still take lawful
    // sword/spirit damage; no assumed universal blood physiology.
    TSet<FOGEntityId> BloodEligible;
    for(const FOGEntityId& TargetId:Action.TargetUnitIds)
        if(IsLuminousBloodStatusEligible(Source.UnitEntityId,TargetId,
            AcceptedBleedFamilyId))
            BloodEligible.Add(TargetId);

    const uint64 Seed=static_cast<uint64>(
        HashCombine(GetTypeHash(State.BattleId.ToString()),
                    HashCombine(GetTypeHash(Action.ActionId.ToString()),
                                GetTypeHash(Source.UnitEntityId.ToString()))));
    FOGDeterministicRng Rng(Seed);
    FOGLargeNumber ActionLifesteal;
    bool bGrantedOnHpDamage=false;

    auto Hit=[&](const FOGEntityId& TargetId,int32 AtkBps,
                 int32 SiphonBps,bool bMarkOnEligible,
                 bool bHemorrhageOnEligible)->bool
    {
        // No previous contact may count as this individual hit's HP damage.
        // Especially important for A2 mark consumption after a miss or shield.
        LastEffectiveHpDamage.Remove(TargetId);
        FOGCombatUnitState* Target=FindMutableUnit(TargetId);
        if(!Target || !Target->IsAlive())return true;
        FOGCombatStats EffectiveAttacker=Source.Stats;
        int32 DawnAtk=0,DawnSpd=0,DawnHeal=0,DawnReduction=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,
            DawnAtk,DawnSpd,DawnHeal,DawnReduction);
        if(DawnAtk>0)
            EffectiveAttacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                EffectiveAttacker.Attack,10000+DawnAtk);
        if(DawnSpd>0)
            EffectiveAttacker.Speed=EffectiveAttacker.Speed*
                (10000+DawnSpd)/10000;
        if(StateOfOwner.HemalMarks.FindRef(TargetId)>=2)
            EffectiveAttacker.CritDamageBonusBps+=1800; // Scarlet Rhythm P2
        if(CurrentOwnerAction<=StateOfOwner.RevivalAfterglowExpiresOwnerAction)
            EffectiveAttacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                EffectiveAttacker.Attack,11200); // Reconstitution P4
        if(StateOfOwner.bAttunementActive &&
           !StateOfOwner.bDayAttunement)
        {
            EffectiveAttacker.Speed=EffectiveAttacker.Speed*10800/10000;
            if(StateOfOwner.HemorrhageStacks.FindRef(TargetId)>0)
                EffectiveAttacker.DefensePenetrationBps+=1500;
        }
        FOGDamageRequest Request;
        Request.BaseDamage=FOGLargeNumber::ScaleByBasisPoints(
            EffectiveAttacker.Attack,AtkBps);
        Request.DefenseReference=Source.Stats.Attack;
        Request.bAllowHitOverflowReplication=false;
        Request.bCanCrit=true;
        if(StateOfOwner.bAttunementActive &&
           !StateOfOwner.bDayAttunement)
            Request.AdditionalMultiplierBps.Add(11000);
        // Night Ultimate's +20% penetration is confined to its own four
        // independently rolled strokes. No global passive is fabricated.
        if(bUlt && UltimateBranch==TEXT("night"))
            EffectiveAttacker.DefensePenetrationBps+=2000;
        const FOGDamageResolution Damage=FOGCombatMath::ResolveDamage(
            Request,EffectiveAttacker,Target->Stats,Rng);
        if(Damage.TotalDamage.IsZero())return true;
        const FOGLargeNumber Before=Target->CurrentHp;
        if(!ApplyDamage(Source.UnitEntityId,TargetId,
                        Damage.TotalDamage,Action.SkillId,OutError))
            return false;
        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(TargetId);
        if(Actual.IsZero())return true;
        if(!bGrantedOnHpDamage && BloodEligible.Contains(TargetId))
        {
            StateOfOwner.ScarletReserve=FMath::Min(Kit.ScarletReserveMax,
                                                  StateOfOwner.ScarletReserve+10);
            bGrantedOnHpDamage=true;
        }
        if(BloodEligible.Contains(TargetId))
        {
            if(bMarkOnEligible)
            {
                const int32 BeforeMarks=StateOfOwner.HemalMarks.FindRef(TargetId);
                const int32 AfterMarks=FMath::Min(4,BeforeMarks+1);
                StateOfOwner.HemalMarks.Add(TargetId,AfterMarks);
                StateOfOwner.HemalMarkExpiryOwnerAction.Add(
                    TargetId,CurrentOwnerAction+3);
                if(BeforeMarks<4 && AfterMarks==4)
                    StateOfOwner.PendingFourMarkTargetBonus.Add(TargetId);
            }
            if(bHemorrhageOnEligible)
            {
                StateOfOwner.HemorrhageStacks.FindOrAdd(TargetId)=
                    FMath::Min(2,StateOfOwner.HemorrhageStacks.FindRef(TargetId)+1);
                StateOfOwner.HemorrhageEndOwnerAction.Add(TargetId,CurrentOwnerAction+2);
            }
            if(SiphonBps>0)
            {
                const FOGLargeNumber Heal=FOGLargeNumber::ScaleByBasisPoints(Actual,SiphonBps);
                ActionLifesteal=FOGLargeNumber::Add(ActionLifesteal,Heal);
            }
        }
        return true;
    };

    if(bA1)
    {
        const FOGEntityId Id=Action.TargetUnitIds[0];
        const bool bHadBleedBeforeA1=
            StateOfOwner.HemorrhageStacks.FindRef(Id)>0;
        for(int32 HitIndex=0;HitIndex<3;++HitIndex)
        {
            constexpr int32 AtkBps[]={4000,6000,9000};
            if(!Hit(Id,AtkBps[HitIndex],2400,HitIndex<2,HitIndex==2))
                return false;
            if(HitIndex==2 &&
               FOGLargeNumber::Compare(GetLastEffectiveHpDamage(Id),
                                        FOGLargeNumber())>0)
            {
                StateOfOwner.bNextScarletCadenceDelayReduction=true;
                if(bHadBleedBeforeA1)
                {
                    StateOfOwner.DiscountNextA2Reserve=5;
                    // A1 Targeted Execution may extend its existing
                    // source Hemorrhage by ONE owner turn, never beyond 3.
                    if(StateOfOwner.HemorrhageStacks.FindRef(Id)>0)
                        StateOfOwner.HemorrhageEndOwnerAction.Add(
                            Id,CurrentOwnerAction+3);
                }
            }
        }
    }
    else if(bA2)
    {
        bool bA2DefeatedActualEnemy=false;
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)
        {
            const FOGEntityId Id=Action.TargetUnitIds[I];
            const int32 Marks=StateOfOwner.HemalMarks.FindRef(Id);
            const bool bConsume=I==0 && Marks>=3;
            const bool bFourMarkBonus=bConsume &&
                StateOfOwner.PendingFourMarkTargetBonus.Contains(Id);
            // Check each independently critting hit for ACTUAL HP damage;
            // refund mark costs on a wholly blocked/missed cast, never
            // manufacture an Eclipse Claim from shield-only contact.
            if(bConsume)StateOfOwner.HemalMarks.Add(Id,Marks-3);
            if(!Hit(Id,11500,2400,false,false))return false;
            bool bActualContact=FOGLargeNumber::Compare(
                GetLastEffectiveHpDamage(Id),FOGLargeNumber())>0;
            if(!Hit(Id,12500+(bConsume?10000:0)+
                    (bFourMarkBonus?2500:0),2400,false,false))return false;
            bActualContact=bActualContact ||
                FOGLargeNumber::Compare(GetLastEffectiveHpDamage(Id),
                                         FOGLargeNumber())>0;
            if(bConsume && bActualContact)
            {
                StateOfOwner.EclipseClaimTargets.Add(Id);
                StateOfOwner.EclipseClaimExpiryOwnerAction.Add(
                    Id,CurrentOwnerAction+2);
                if(bFourMarkBonus)
                    StateOfOwner.PendingFourMarkTargetBonus.Remove(Id);
            }
            else if(bConsume)
                StateOfOwner.HemalMarks.Add(Id,Marks);
            if(bNightBeforeA2 && bActualContact)
            {
                const FOGCombatUnitState* After=FindUnit(Id);
                if(After && !After->IsAlive())
                    bA2DefeatedActualEnemy=true;
            }
        }
        // Queen of Nightfall P3: a Night A2 must genuinely defeat an enemy;
        // one +12 percentage-point charge grant per owner's action.
        if(bA2DefeatedActualEnemy &&
           StateOfOwner.LastNightExecutionChargeOwnerAction!=
               CurrentOwnerAction)
        {
            StateOfOwner.UltimateChargePercent=FMath::Min(
                200,StateOfOwner.UltimateChargePercent+12);
            StateOfOwner.LastNightExecutionChargeOwnerAction=
                CurrentOwnerAction;
        }
        StateOfOwner.bDayAttunement=false;
        StateOfOwner.bAttunementActive=true;
        StateOfOwner.AttunementExpiryAction=CurrentOwnerAction+2;
    }
    else if((bA3 && !bA3Covenant) ||
            (bUlt && UltimateBranch==TEXT("day")))
    {
        const bool bDayUltimate=bUlt;
        // Benediction and Day Ultimate heal from actual caster ATK, including
        // only currently active, approved Dawn attack blessings on the caster.
        int32 CasterAtkBps=0,CasterSpeedBps=0,CasterHealBps=0,CasterMitigationBps=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,CasterAtkBps,
            CasterSpeedBps,CasterHealBps,CasterMitigationBps);
        const FOGLargeNumber HealerAttack=CasterAtkBps>0
            ?FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.Attack,10000+CasterAtkBps)
            :Source.Stats.Attack;
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)
        {
            const FOGEntityId Id=Action.TargetUnitIds[I];
            FOGCombatUnitState* T=FindMutableUnit(Id);
            if(!T || !T->IsAlive())continue;
            const int32 Scale=bDayUltimate?(I==0?22000:9500):
                                          (I==0?6500:2500);
            FOGLargeNumber Heal=FOGLargeNumber::ScaleByBasisPoints(
                HealerAttack,Scale);
            Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12400);
            const int32* OathExpiry=
                StateOfOwner.DawnOathByBeneficiary.Find(Id);
            const bool bConsumeDawnOath=bDayUltimate && OathExpiry &&
                *OathExpiry>=CurrentOwnerAction;
            if(bConsumeDawnOath)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12000);
            if(StateOfOwner.bAttunementActive && StateOfOwner.bDayAttunement)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,11200);
            // The actual shared healer applies target HealingReceived AFTER
            // this skill, and clamps at MaxHp. Never cap this amount first:
            // doing so double counts or loses overheal when a blessing is up.
            int32 RecipientAtk=0,RecipientSpeed=0;
            int32 RecipientHeal=0,RecipientReduction=0;
            GetApprovedDawnBlessingBps(Id,RecipientAtk,RecipientSpeed,
                RecipientHeal,RecipientReduction);
            const FOGLargeNumber GrossHealing=RecipientHeal>0
                ?FOGLargeNumber::ScaleByBasisPoints(
                    Heal,10000+RecipientHeal):Heal;
            const FOGLargeNumber BeforeHealing=T->CurrentHp;
            if(!Heal.IsZero() &&
               !ApplyHealing(Source.UnitEntityId,Id,Heal,Action.SkillId,OutError))
                return false;
            const FOGLargeNumber RealRestored=FOGLargeNumber::Add(
                T->CurrentHp,FOGLargeNumber(
                    -BeforeHealing.Significand,BeforeHealing.Exponent10));
            // P5 Dawn Sanctification converts 40% of genuinely unused
            // computed healing into its separately capped 8%-MaxHP barrier.
            const FOGLargeNumber UnusedHealing=FOGLargeNumber::Add(
                GrossHealing,FOGLargeNumber(
                    -RealRestored.Significand,RealRestored.Exponent10));
            if(FOGLargeNumber::Compare(UnusedHealing,FOGLargeNumber())>0)
                GrantAuthoredDawnOverhealBarrier(
                    Source.UnitEntityId,Id,UnusedHealing);
            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            StateOfOwner.BlessingCategory.Add(Id,bDayUltimate?2:1);
            if(bA3 && FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0)
                StateOfOwner.DawnOathByBeneficiary.Add(
                    Id,CurrentOwnerAction+2);
            if(bConsumeDawnOath && FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0)
                StateOfOwner.DawnOathByBeneficiary.Remove(Id);
            if(FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0 && !bGrantedOnHpDamage)
            {
                StateOfOwner.ScarletReserve=FMath::Min(
                    Kit.ScarletReserveMax,StateOfOwner.ScarletReserve+8);
                bGrantedOnHpDamage=true;
            }
        }
        if(bA3)
        {
            StateOfOwner.bDayAttunement=true;
            StateOfOwner.bAttunementActive=true;
            StateOfOwner.AttunementExpiryAction=CurrentOwnerAction+2;
        }
        StateOfOwner.DawnOathExpiry=CurrentOwnerAction+2;
    }
    else if(bUlt)
    {
        // Four independently resolved strikes. Night 25% siphon replaces
        // the ordinary 24% passive; never apply both to the same hit.
        constexpr int32 Strokes[]={9000,12000,15000,18000};
        const FOGEntityId Primary=Action.TargetUnitIds[0];
        const bool bEclipseClaim=
            StateOfOwner.EclipseClaimTargets.Contains(Primary);
        for(int32 K=0;K<4;++K)
            for(int32 I=0;I<Action.TargetUnitIds.Num();++I)
                if(!Hit(Action.TargetUnitIds[I],
                        I==0?Strokes[K]+(K==3 && bEclipseClaim?9000:0):
                             Strokes[K]*6000/10000,
                        2500,false,K==3))
                    return false;
        if(bEclipseClaim)
        {
            StateOfOwner.EclipseClaimTargets.Remove(Primary);
            StateOfOwner.EclipseClaimExpiryOwnerAction.Remove(Primary);
        }
        // Both Ultimates are independently selectable and preserve existing
        // attunement: they do NOT force the owner into Night.
    }

    if(!ActionLifesteal.IsZero())
    {
        const int32 CapBps=(bUlt && UltimateBranch==TEXT("night"))?2500:1800;
        const FOGLargeNumber Cap=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.MaxHp,CapBps);
        const FOGLargeNumber Heal=MinNumber(
            MinNumber(ActionLifesteal,Cap),PositiveMissing(Source));
        if(!Heal.IsZero() &&
           !ApplyHealing(Source.UnitEntityId,Source.UnitEntityId,
                         Heal,Action.SkillId,OutError))return false;
    }
    return true;
}

void FOGTurnBattle::TryResolveApprovedLuminousBaseRevival(
    const FOGEntityId& UnitId)
{
    FLuminousBaseBattleRuntime* Runtime=LuminousBaseStates.Find(UnitId);
    FOGCombatUnitState* Unit=FindMutableUnit(UnitId);
    if(!Runtime || !Unit || !IsBaseLuminous(*Unit) ||
       Runtime->bRevivalUsed || Runtime->ScarletReserve<40 ||
       !IsLuminousRevivalEligible(UnitId,UnitId,false) ||
       Unit->Presence!=EOGCombatPresence::Defeated ||
       Unit->OccupiedLaneIndex==INDEX_NONE)return;

    const FOGLargeNumber Restoration=FOGLargeNumber::ScaleByBasisPoints(
        Unit->Stats.MaxHp,2200);
    FString Error;
    if(Restoration.IsZero() || !ApplyApprovedLuminousRevivalHealing(
            UnitId,UnitId,Restoration,
            FOGContentId(TEXT("combat:luminous_valentine.base.skill.crimson_reconstitution")),
            Error))
        return;
    if(!ChangePresence(UnitId,EOGCombatPresence::Active,Error))
        return;
    Runtime->ScarletReserve-=40;
    Runtime->bRevivalUsed=true;
    Runtime->RevivalAfterglowExpiresOwnerAction=Runtime->OwnerActions+1;
}


void FOGTurnBattle::TryResolveApprovedLuminousAllyCovenant(
    const FOGEntityId& TargetUnitId)
{
    FOGCombatUnitState* Target=FindMutableUnit(TargetUnitId);
    if(!Target || Target->Presence!=EOGCombatPresence::Defeated ||
       Target->OccupiedLaneIndex==INDEX_NONE)return;
    // Multiple eligible Manifestations resolve in stable GUID order.
    TArray<FOGEntityId> Owners;
    LuminousBaseStates.GetKeys(Owners);
    Owners.Sort([](const FOGEntityId& A,const FOGEntityId& B)
    {
        return A.ToString()<B.ToString();
    });
    for(const FOGEntityId& OwnerId:Owners)
    {
        FLuminousBaseBattleRuntime* S=LuminousBaseStates.Find(OwnerId);
        const FOGCombatUnitState* Owner=FindUnit(OwnerId);
        if(!S || !Owner || !Owner->IsAlive() || !IsBaseLuminous(*Owner) ||
           S->bAllyRevivalSpent ||
           !IsLuminousRevivalEligible(OwnerId,TargetUnitId,true) ||
           Owner->TeamIndex!=Target->TeamIndex)continue;
        const bool bA3Prepared=
            S->PreparedCovenantTarget==TargetUnitId &&
            S->CovenantExpiryOwnerAction>=S->OwnerActions;
        const bool bUltPrepared=
            S->PreparedDayUltimateRevivalTarget==TargetUnitId &&
            S->DayUltimateRevivalExpiryOwnerAction>=S->OwnerActions;
        if(!bA3Prepared && !bUltPrepared)continue;
        const FOGLargeNumber Restore=FOGLargeNumber::ScaleByBasisPoints(
            Target->Stats.MaxHp,bUltPrepared?3000:1800);
        if(Restore.IsZero())return;
        FString Error;
        if(!ApplyApprovedLuminousRevivalHealing(OwnerId,TargetUnitId,Restore,
                bUltPrepared
                    ? FOGContentId(TEXT("combat:luminous_valentine.base.ultimate.twin_principles"))
                    : FOGContentId(TEXT("combat:luminous_valentine.base.skill.crimson_benediction")),
                Error))return;
        if(!ChangePresence(TargetUnitId,EOGCombatPresence::Active,Error))
            return;
        S->bAllyRevivalSpent=true;
        S->PreparedCovenantTarget=FOGEntityId();
        S->CovenantExpiryOwnerAction=-1;
        S->PreparedDayUltimateRevivalTarget=FOGEntityId();
        S->DayUltimateRevivalExpiryOwnerAction=-1;
        S->BlessingExpiry.Add(TargetUnitId,S->OwnerActions+2);
        S->BlessingCategory.Add(TargetUnitId,bUltPrepared?2:1);
        return;
    }
}


void FOGTurnBattle::GrantAuthoredDawnOverhealBarrier(
    const FOGEntityId& OwnerId,const FOGEntityId& BeneficiaryId,
    const FOGLargeNumber& ExcessHealing)
{
    FLuminousBaseBattleRuntime* OwnerState=LuminousBaseStates.Find(OwnerId);
    const FOGCombatUnitState* Target=FindUnit(BeneficiaryId);
    if(!OwnerState || !Target || !Target->IsAlive() ||
       FOGLargeNumber::Compare(ExcessHealing,FOGLargeNumber())<=0)
        return;
    const FOGLargeNumber MaxBarrier=FOGLargeNumber::ScaleByBasisPoints(
        Target->Stats.MaxHp,800);
    const FOGLargeNumber Gift=FOGLargeNumber::ScaleByBasisPoints(
        ExcessHealing,4000);
    for(FLuminousDawnBarrier& Barrier:LuminousDawnBarriers)
        if(Barrier.GrantingManifestation==OwnerId &&
           Barrier.Beneficiary==BeneficiaryId)
        {
            if(OwnerState->OwnerActions>Barrier.ExpiresAfterOwnerAction)
                Barrier.Remaining=FOGLargeNumber();
            const FOGLargeNumber Candidate=FOGLargeNumber::Add(
                Barrier.Remaining,Gift);
            Barrier.Remaining=FOGLargeNumber::Compare(Candidate,MaxBarrier)<0
                ?Candidate:MaxBarrier;
            Barrier.ExpiresAfterOwnerAction=OwnerState->OwnerActions+2;
            return;
        }
    FLuminousDawnBarrier Barrier;
    Barrier.GrantingManifestation=OwnerId;
    Barrier.Beneficiary=BeneficiaryId;
    Barrier.Remaining=FOGLargeNumber::Compare(Gift,MaxBarrier)<0
        ?Gift:MaxBarrier;
    Barrier.ExpiresAfterOwnerAction=OwnerState->OwnerActions+2;
    LuminousDawnBarriers.Add(MoveTemp(Barrier));
}
