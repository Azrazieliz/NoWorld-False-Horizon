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
    if (!bA1 && !bA2 && !bA3 && !bUlt)
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
    bool bPreparedSoulVerified=false,bPreparedNegationClear=false;
    if(bA3Covenant)
    {
        if(!Context->TryGetBoolField(TEXT("verified_target_soul_continuity"),
                                     bPreparedSoulVerified) ||
           !bPreparedSoulVerified ||
           !Context->TryGetBoolField(TEXT("verified_no_superior_revival_negation"),
                                     bPreparedNegationClear) ||
           !bPreparedNegationClear)
        {
            OutError=TEXT("A3 Covenant requires explicit lawful soul and higher-order Authority verification.");
            return false;
        }
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
    if(bA3Covenant && StateOfOwner.bAllyRevivalSpent)
    {
        OutError=TEXT("The shared ally revival ledger was already consumed in this encounter.");
        return false;
    }
    if(bA3Covenant && StateOfOwner.ScarletReserve<75)
    {
        OutError=TEXT("A3 Covenant requires 75 actual Scarlet Reserve at preparation.");
        return false;
    }
    if(bA3Covenant)
    {
        // Both portions, 30 + 45, are committed at preparation (not death).
        StateOfOwner.ScarletReserve-=75;
        StateOfOwner.PreparedCovenantTarget=Action.TargetUnitIds[0];
        StateOfOwner.CovenantExpiryOwnerAction=StateOfOwner.OwnerActions+3;
        StateOfOwner.bCovenantSoulVerified=bPreparedSoulVerified;
        StateOfOwner.bCovenantNegationClear=bPreparedNegationClear;
        StateOfOwner.bDayAttunement=true;
    }
    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;

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

    bool bSoulVerified=false,bUnnegated=false;
    if(Context->TryGetBoolField(TEXT("verified_soul_continuity"),bSoulVerified) &&
       Context->TryGetBoolField(TEXT("verified_no_superior_revival_negation"),bUnnegated))
    {
        StateOfOwner.bSelfSoulContinuityVerified=bSoulVerified;
        StateOfOwner.bNoSuperiorRevivalNegationVerified=bUnnegated;
    }

    ++StateOfOwner.OwnerActions;
    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;
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
        const FOGLargeNumber Actual=MinNumber(Before,Bleed);
        const FOGLargeNumber Siphon=FOGLargeNumber::ScaleByBasisPoints(Actual,1200);
        const FOGLargeNumber Limited=MinNumber(Siphon,PositiveMissing(Source));
        if(!Limited.IsZero() &&
           !ApplyHealing(Source.UnitEntityId,Source.UnitEntityId,
                         Limited,Action.SkillId,OutError))return false;
        StateOfOwner.ScarletReserve=FMath::Min(
            Kit.ScarletReserveMax,StateOfOwner.ScarletReserve+6);
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
    const TArray<TSharedPtr<FJsonValue>>* BloodIds=nullptr;
    if(Context->TryGetArrayField(TEXT("verified_blood_eligible_targets"),BloodIds))
        for(const TSharedPtr<FJsonValue>& V:*BloodIds)
        {
            FGuid G;
            if(FGuid::Parse(V->AsString(),G))
                BloodEligible.Add(FOGEntityId(G));
        }

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
        FOGCombatUnitState* Target=FindMutableUnit(TargetId);
        if(!Target || !Target->IsAlive())return true;
        FOGDamageRequest Request;
        Request.BaseDamage=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.Attack,AtkBps);
        Request.DefenseReference=Source.Stats.Attack;
        Request.bAllowHitOverflowReplication=false;
        Request.bCanCrit=true;
        if(StateOfOwner.bDayAttunement==false &&
           StateOfOwner.OwnerActions>0)
            Request.AdditionalMultiplierBps.Add(11000);
        const FOGDamageResolution Damage=FOGCombatMath::ResolveDamage(
            Request,Source.Stats,Target->Stats,Rng);
        if(Damage.TotalDamage.IsZero())return true;
        const FOGLargeNumber Before=Target->CurrentHp;
        if(!ApplyDamage(Source.UnitEntityId,TargetId,
                        Damage.TotalDamage,Action.SkillId,OutError))
            return false;
        const FOGLargeNumber Actual=MinNumber(Before,Damage.TotalDamage);
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
                StateOfOwner.HemalMarks.FindOrAdd(TargetId)=
                    FMath::Min(4,StateOfOwner.HemalMarks.FindRef(TargetId)+1);
            if(bHemorrhageOnEligible)
            {
                StateOfOwner.HemorrhageStacks.FindOrAdd(TargetId)=
                    FMath::Min(2,StateOfOwner.HemorrhageStacks.FindRef(TargetId)+1);
                StateOfOwner.HemorrhageEndOwnerAction.Add(TargetId,CurrentOwnerAction+3);
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
        for(int32 HitIndex=0;HitIndex<3;++HitIndex)
        {
            constexpr int32 AtkBps[]={4000,6000,9000};
            if(!Hit(Id,AtkBps[HitIndex],2400,HitIndex<2,HitIndex==2))
                return false;
        }
    }
    else if(bA2)
    {
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)
        {
            const FOGEntityId Id=Action.TargetUnitIds[I];
            const int32 Marks=StateOfOwner.HemalMarks.FindRef(Id);
            const bool bConsume=I==0 && Marks>=3;
            if(bConsume)StateOfOwner.HemalMarks.Add(Id,Marks-3);
            if(!Hit(Id,11500,2400,false,false) ||
               !Hit(Id,12500+(bConsume?10000:0),2400,false,false))
                return false;
        }
        StateOfOwner.bDayAttunement=false;
    }
    else if((bA3 && !bA3Covenant) ||
            (bUlt && UltimateBranch==TEXT("day")))
    {
        const bool bDayUltimate=bUlt;
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)
        {
            const FOGEntityId Id=Action.TargetUnitIds[I];
            FOGCombatUnitState* T=FindMutableUnit(Id);
            if(!T || !T->IsAlive())continue;
            const FOGLargeNumber Missing=PositiveMissing(*T);
            if(Missing.IsZero())continue;
            const int32 Scale=bDayUltimate?(I==0?22000:9500):
                                          (I==0?6500:2500);
            FOGLargeNumber Heal=FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.Attack,Scale);
            Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,12400);
            if(StateOfOwner.bDayAttunement)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,11200);
            Heal=MinNumber(Heal,Missing);
            if(!Heal.IsZero() &&
               !ApplyHealing(Source.UnitEntityId,Id,Heal,Action.SkillId,OutError))
                return false;
            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            if(!bGrantedOnHpDamage)
            {
                StateOfOwner.ScarletReserve=FMath::Min(
                    Kit.ScarletReserveMax,StateOfOwner.ScarletReserve+8);
                bGrantedOnHpDamage=true;
            }
        }
        StateOfOwner.bDayAttunement=true;
        StateOfOwner.DawnOathExpiry=CurrentOwnerAction+2;
    }
    else if(bUlt)
    {
        // Four independently resolved strikes. Night 25% siphon replaces
        // the ordinary 24% passive; never apply both to the same hit.
        constexpr int32 Strokes[]={9000,12000,15000,18000};
        for(int32 K=0;K<4;++K)
            for(int32 I=0;I<Action.TargetUnitIds.Num();++I)
                if(!Hit(Action.TargetUnitIds[I],
                        I==0?Strokes[K]:Strokes[K]*6000/10000,
                        2500,false,K==3))
                    return false;
        StateOfOwner.bDayAttunement=false;
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
       !Runtime->bSelfSoulContinuityVerified ||
       !Runtime->bNoSuperiorRevivalNegationVerified ||
       Unit->Presence!=EOGCombatPresence::Defeated ||
       Unit->OccupiedLaneIndex==INDEX_NONE)return;

    const FOGLargeNumber Restoration=FOGLargeNumber::ScaleByBasisPoints(
        Unit->Stats.MaxHp,2200);
    FString Error;
    if(Restoration.IsZero() || !ApplyHealing(
            UnitId,UnitId,Restoration,
            FOGContentId(TEXT("combat:luminous_valentine.base.skill.crimson_reconstitution")),
            Error))
        return;
    if(!ChangePresence(UnitId,EOGCombatPresence::Active,Error))
        return;
    Runtime->ScarletReserve-=40;
    Runtime->bRevivalUsed=true;
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
           S->bAllyRevivalSpent || !S->bCovenantSoulVerified ||
           !S->bCovenantNegationClear ||
           S->PreparedCovenantTarget!=TargetUnitId ||
           S->CovenantExpiryOwnerAction<S->OwnerActions ||
           Owner->TeamIndex!=Target->TeamIndex)continue;
        const FOGLargeNumber Restore=FOGLargeNumber::ScaleByBasisPoints(
            Target->Stats.MaxHp,1800);
        if(Restore.IsZero())return;
        FString Error;
        if(!ApplyHealing(OwnerId,TargetUnitId,Restore,
                FOGContentId(TEXT("combat:luminous_valentine.base.skill.crimson_benediction")),
                Error))return;
        if(!ChangePresence(TargetUnitId,EOGCombatPresence::Active,Error))
            return;
        S->bAllyRevivalSpent=true;
        S->PreparedCovenantTarget=FOGEntityId();
        S->CovenantExpiryOwnerAction=-1;
        S->BlessingExpiry.Add(TargetUnitId,S->OwnerActions+2);
        return;
    }
}
