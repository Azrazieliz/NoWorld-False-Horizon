from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
def edit(p,pairs):
 s=p.read_text(encoding="utf-8-sig")
 for old,new in pairs:
  n=s.count(old)
  if n==0 and new in s:continue # resume this bounded source edit without touching the already-updated header
  if n!=1:raise RuntimeError(f"{p.name}: expected one occurrence, found {n} of {old[:82]}")
  s=s.replace(old,new,1)
 p.write_text(s,encoding="utf8",newline="")
 print(p.name,len(pairs))
edit(r/"Public/Combat/OGTurnBattle.h",[
 ("""        FOGEntityId PreparedCovenantTarget;
        int32 CovenantExpiryOwnerAction = -1;""",
 """        FOGEntityId PreparedCovenantTarget;
        int32 CovenantExpiryOwnerAction = -1;
        // Approved Day Ultimate 35 Reserve/30% ally self-continuity rescue:
        // a prepared reaction, NOT free post-succession active resurrection.
        FOGEntityId PreparedDayUltimateRevivalTarget;
        int32 DayUltimateRevivalExpiryOwnerAction = -1;""")
])
edit(r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp",[
 ("""    FString A3Mode;
    if(bA3)Context->TryGetStringField(TEXT("mode"),A3Mode);
    const bool bA3Covenant=bA3 && A3Mode==TEXT("covenant");""",
 """    FString A3Mode;
    if(bA3)Context->TryGetStringField(TEXT("mode"),A3Mode);
    const bool bA3Covenant=bA3 && A3Mode==TEXT("covenant");
    FString UltimateMode;
    if(bUlt)Context->TryGetStringField(TEXT("mode"),UltimateMode);"""),
 ("""    FLuminousBaseBattleRuntime& StateOfOwner=
        LuminousBaseStates.FindOrAdd(Source.UnitEntityId);
    const int32 ChargeCost=bUlt""",
 """    const bool bDayRevivalPrepare=bUlt &&
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
    const int32 ChargeCost=bUlt"""),
 ("""    if(bA3Covenant && StateOfOwner.bAllyRevivalSpent)
    {
        OutError=TEXT("The shared ally revival ledger was already consumed in this encounter.");
        return false;
    }""",
 """    if((bA3Covenant || bDayRevivalPrepare) &&
       StateOfOwner.bAllyRevivalSpent)
    {
        OutError=TEXT("The shared ally revival ledger was already consumed in this encounter.");
        return false;
    }
    if(bDayRevivalPrepare && StateOfOwner.ScarletReserve<35)
    {
        OutError=TEXT("Day Ultimate prepared revival needs 35 Scarlet Reserve in addition to its Ultimate charge.");
        return false;
    }"""),
 ("""    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;
    const bool bNightBeforeA2=StateOfOwner.bAttunementActive &&
        !StateOfOwner.bDayAttunement;
    if(bA2)""",
 """    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;
    if(bDayRevivalPrepare)
    {
        StateOfOwner.ScarletReserve-=35;
        StateOfOwner.PreparedDayUltimateRevivalTarget=
            Action.TargetUnitIds[0];
        StateOfOwner.DayUltimateRevivalExpiryOwnerAction=
            NextOwnerAction+2;
    }
    if(bA2)"""),
 ("""    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;
    if(StateOfOwner.bAttunementActive &&""",
 """    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;
    if(bDayRevivalPrepare)
        return true; // One legal preparedness action, no simultaneous AoE heal.
    if(StateOfOwner.bAttunementActive &&"""),
 ("""           S->PreparedCovenantTarget!=TargetUnitId ||
           S->CovenantExpiryOwnerAction<S->OwnerActions ||
           Owner->TeamIndex!=Target->TeamIndex)continue;
        const FOGLargeNumber Restore=FOGLargeNumber::ScaleByBasisPoints(
            Target->Stats.MaxHp,1800);""",
 """           Owner->TeamIndex!=Target->TeamIndex)continue;
        const bool bA3Prepared=
            S->PreparedCovenantTarget==TargetUnitId &&
            S->CovenantExpiryOwnerAction>=S->OwnerActions;
        const bool bUltPrepared=
            S->PreparedDayUltimateRevivalTarget==TargetUnitId &&
            S->DayUltimateRevivalExpiryOwnerAction>=S->OwnerActions;
        if(!bA3Prepared && !bUltPrepared)continue;
        const FOGLargeNumber Restore=FOGLargeNumber::ScaleByBasisPoints(
            Target->Stats.MaxHp,bUltPrepared?3000:1800);"""),
 ("""        if(!ApplyApprovedLuminousRevivalHealing(OwnerId,TargetUnitId,Restore,
                FOGContentId(TEXT("combat:luminous_valentine.base.skill.crimson_benediction")),
                Error))return;""",
 """        if(!ApplyApprovedLuminousRevivalHealing(OwnerId,TargetUnitId,Restore,
                bUltPrepared
                    ? FOGContentId(TEXT("combat:luminous_valentine.base.ultimate.twin_principles"))
                    : FOGContentId(TEXT("combat:luminous_valentine.base.skill.crimson_benediction")),
                Error))return;"""),
 ("""        S->PreparedCovenantTarget=FOGEntityId();
        S->CovenantExpiryOwnerAction=-1;
        S->BlessingExpiry.Add(TargetUnitId,S->OwnerActions+2);""",
 """        S->PreparedCovenantTarget=FOGEntityId();
        S->CovenantExpiryOwnerAction=-1;
        S->PreparedDayUltimateRevivalTarget=FOGEntityId();
        S->DayUltimateRevivalExpiryOwnerAction=-1;
        S->BlessingExpiry.Add(TargetUnitId,S->OwnerActions+2);"""),
 ("""        S->BlessingCategory.Add(TargetUnitId,1);
        return;
    }
}""",
 """        S->BlessingCategory.Add(TargetUnitId,bUltPrepared?2:1);
        return;
    }
}""")
])
print("BASE_APPROVED_SHARED_A3_DAY_REVIVAL_35_RESERVE_30_PERCENT_PREPARED_REACTION_SOURCE_COMPLETE")
