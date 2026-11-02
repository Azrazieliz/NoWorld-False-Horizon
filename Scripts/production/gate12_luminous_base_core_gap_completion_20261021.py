from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
def swap(path,pairs):
 s=path.read_text(encoding="utf-8-sig")
 for a,b in pairs:
  n=s.count(a)
  if n!=1:raise RuntimeError(f"{path.name} expected one patch anchor, found {n}: {a[:100]}")
  s=s.replace(a,b,1)
 path.write_text(s,encoding="utf8",newline="")
 print(path.name,len(pairs),"source changes")
h=r/"Public/Combat/OGTurnBattle.h"
swap(h,[
 ("""        bool bRevivalUsed = false;
        bool bDayAttunement = false;""",
 """        bool bRevivalUsed = false;
        int32 RevivalAfterglowExpiresOwnerAction = INDEX_NONE;
        int32 LastNightExecutionChargeOwnerAction = INDEX_NONE;
        bool bDayAttunement = false;""")
])
base=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
swap(base,[
 ("""    if(bA3Covenant)
    {
        // Both portions, 30 + 45, are committed at preparation (not death).""",
 """    // A2 Bloodless Cost is lawful only when a selected enemy lacks the
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
        // Both portions, 30 + 45, are committed at preparation (not death)."""),
 ("""    if(bA2)
    {
        StateOfOwner.ScarletReserve-=BaseReserveCost;""",
 """    const bool bNightBeforeA2=StateOfOwner.bAttunementActive &&
        !StateOfOwner.bDayAttunement;
    if(bA2)
    {
        StateOfOwner.ScarletReserve-=BaseReserveCost;"""),
 ("""        if(StateOfOwner.HemalMarks.FindRef(TargetId)>=2)
            EffectiveAttacker.CritDamageBonusBps+=1800; // Scarlet Rhythm P2""",
 """        if(StateOfOwner.HemalMarks.FindRef(TargetId)>=2)
            EffectiveAttacker.CritDamageBonusBps+=1800; // Scarlet Rhythm P2
        if(CurrentOwnerAction<=StateOfOwner.RevivalAfterglowExpiresOwnerAction)
            EffectiveAttacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                EffectiveAttacker.Attack,11200); // Reconstitution P4"""),
 ("""                if(bHadBleedBeforeA1)
                    StateOfOwner.DiscountNextA2Reserve=5;""",
 """                if(bHadBleedBeforeA1)
                {
                    StateOfOwner.DiscountNextA2Reserve=5;
                    // A1 Targeted Execution may extend its existing
                    // source Hemorrhage by ONE owner turn, never beyond 3.
                    if(StateOfOwner.HemorrhageStacks.FindRef(Id)>0)
                        StateOfOwner.HemorrhageEndOwnerAction.Add(
                            Id,CurrentOwnerAction+3);
                }"""),
 ("""    else if(bA2)
    {
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)""",
 """    else if(bA2)
    {
        bool bA2DefeatedActualEnemy=false;
        for(int32 I=0;I<Action.TargetUnitIds.Num();++I)"""),
 ("""            else if(bConsume)
                StateOfOwner.HemalMarks.Add(Id,Marks);
        }
        StateOfOwner.bDayAttunement=false;""",
 """            else if(bConsume)
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
        StateOfOwner.bDayAttunement=false;"""),
 ("""    Runtime->ScarletReserve-=40;
    Runtime->bRevivalUsed=true;""",
 """    Runtime->ScarletReserve-=40;
    Runtime->bRevivalUsed=true;
    Runtime->RevivalAfterglowExpiresOwnerAction=Runtime->OwnerActions+1;""")
])
print("AUTHORED_BASE_A1_HEMORRHAGE_A2_BLOODLESS_P3_EXECUTION_P4_AFTERGLOW_SOURCE_IMPLEMENTED")
