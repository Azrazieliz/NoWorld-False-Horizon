from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
base=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
battle=r/"Private/Combat/OGTurnBattle.cpp"
def change(f,swaps):
 s=f.read_text(encoding="utf-8-sig")
 for o,n in swaps:
  count=s.count(o)
  if count!=1:raise RuntimeError(f"{f.name} count={count} for {o[:100]}")
  s=s.replace(o,n,1)
 f.write_text(s,encoding="utf8",newline="")
 print("BOUND",f.name)
change(h,[
("""        int32 DiscountNextA2Reserve = 0;
        int32 LastBleedReserveOwnerAction = INDEX_NONE;""",
 """        int32 DiscountNextA2Reserve = 0;
        bool bNextScarletCadenceDelayReduction = false;
        int32 LastBleedReserveOwnerAction = INDEX_NONE;
        TSet<FOGEntityId> PendingFourMarkTargetBonus;
        TSet<FOGEntityId> EclipseClaimTargets;""")
])
change(base,[
("""        FOGDamageRequest Request;
        Request.BaseDamage=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.Attack,AtkBps);""",
 """        FOGCombatStats EffectiveAttacker=Source.Stats;
        if(StateOfOwner.HemalMarks.FindRef(TargetId)>=2)
            EffectiveAttacker.CritDamageBonusBps+=1800; // Scarlet Rhythm P2
        if(StateOfOwner.bAttunementActive &&
           !StateOfOwner.bDayAttunement)
        {
            EffectiveAttacker.Speed=EffectiveAttacker.Speed*10800/10000;
            if(StateOfOwner.HemorrhageStacks.FindRef(TargetId)>0)
                EffectiveAttacker.DefensePenetrationBps+=1500;
        }
        FOGDamageRequest Request;
        Request.BaseDamage=FOGLargeNumber::ScaleByBasisPoints(
            EffectiveAttacker.Attack,AtkBps);"""),
("""        const FOGDamageResolution Damage=FOGCombatMath::ResolveDamage(
            Request,Source.Stats,Target->Stats,Rng);""",
 """        const FOGDamageResolution Damage=FOGCombatMath::ResolveDamage(
            Request,EffectiveAttacker,Target->Stats,Rng);"""),
("""            if(bMarkOnEligible)
                StateOfOwner.HemalMarks.FindOrAdd(TargetId)=
                    FMath::Min(4,StateOfOwner.HemalMarks.FindRef(TargetId)+1);""",
 """            if(bMarkOnEligible)
            {
                const int32 BeforeMarks=StateOfOwner.HemalMarks.FindRef(TargetId);
                const int32 AfterMarks=FMath::Min(4,BeforeMarks+1);
                StateOfOwner.HemalMarks.Add(TargetId,AfterMarks);
                if(BeforeMarks<4 && AfterMarks==4)
                    StateOfOwner.PendingFourMarkTargetBonus.Add(TargetId);
            }"""),
("""        const FOGEntityId Id=Action.TargetUnitIds[0];
        for(int32 HitIndex=0;HitIndex<3;++HitIndex)
        {
            constexpr int32 AtkBps[]={4000,6000,9000};
            if(!Hit(Id,AtkBps[HitIndex],2400,HitIndex<2,HitIndex==2))
                return false;
        }""",
 """        const FOGEntityId Id=Action.TargetUnitIds[0];
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
                    StateOfOwner.DiscountNextA2Reserve=5;
            }
        }"""),
("""            const bool bConsume=I==0 && Marks>=3;
            if(bConsume)StateOfOwner.HemalMarks.Add(Id,Marks-3);
            if(!Hit(Id,11500,2400,false,false) ||
               !Hit(Id,12500+(bConsume?10000:0),2400,false,false))
                return false;""",
 """            const bool bConsume=I==0 && Marks>=3;
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
                if(bFourMarkBonus)
                    StateOfOwner.PendingFourMarkTargetBonus.Remove(Id);
            }
            else if(bConsume)
                StateOfOwner.HemalMarks.Add(Id,Marks);"""),
("""        constexpr int32 Strokes[]={9000,12000,15000,18000};
        for(int32 K=0;K<4;++K)
            for(int32 I=0;I<Action.TargetUnitIds.Num();++I)
                if(!Hit(Action.TargetUnitIds[I],
                        I==0?Strokes[K]:Strokes[K]*6000/10000,
                        2500,false,K==3))
                    return false;""",
 """        constexpr int32 Strokes[]={9000,12000,15000,18000};
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
            StateOfOwner.EclipseClaimTargets.Remove(Primary);"""),
])
change(battle,[
("""        Source->NextActionValue =
            State.CurrentActionValue +
            FMath::Max<int64>(
                1,
                Action.ActionDelay);""",
 """        int64 AuthoredDelay=FMath::Max<int64>(1,Action.ActionDelay);
        if(Action.SkillId==FOGContentId(
                TEXT("combat:luminous_valentine.base.skill.scarlet_cadence")))
            if(FLuminousBaseBattleRuntime* S=
                   LuminousBaseStates.Find(Source->UnitEntityId))
                if(S->bNextScarletCadenceDelayReduction)
                {
                    AuthoredDelay=FMath::Max<int64>(1,
                        AuthoredDelay*9000/10000);
                    S->bNextScarletCadenceDelayReduction=false;
                }
        Source->NextActionValue=State.CurrentActionValue+AuthoredDelay;""")
])
print("BASE_V6_SCARLET_RHYTHM_MARK_THRESHOLD_ECLIPSE_CLAIM_AND_LEGAL_TEMPO")
