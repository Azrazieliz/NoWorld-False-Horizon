from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
targets={
"Public/Combat/OGTurnBattle.h":[
("""        bool bSelfRevived = false;
        TMap<FOGEntityId,int32> RuptureSeals;""",
 """        bool bSelfRevived = false;
        // Eclipse Harrier P4 Pursuit: once per owner turn after a genuinely
        // defeated, source-owned bleeding target. Applies to next legal
        // native action-value scheduling, never generic death events.
        int32 LastPursuitOwnerAction = -1;
        bool bPursuitDelayPending = false;
        TMap<FOGEntityId,int32> RuptureSeals;""")],
"Private/Combat/OGTurnBattleLuminousAlterV1.cpp":[
("""    Own.OwnerActions=Current;
    if(Current>Own.MomentumExpiresOwnerAction)""",
 """    Own.OwnerActions=Current;
    if(Current>Own.MomentumExpiresOwnerAction)"""),
("""    // Registered Severing Wound ticks. End at source-approved 2 owner turns.
    for(auto It=Own.SeveringWounds.CreateIterator();It;++It)""",
 """    // P4 Pursuit must be based on an actual final HP hit, not projected
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
    for(auto It=Own.SeveringWounds.CreateIterator();It;++It)"""),
("""            const FOGLargeNumber Actual=GetLastEffectiveHpDamage(T->UnitEntityId);
            if(Actual.IsZero())continue;
            Own.RedTithe=FMath::Min(100,Own.RedTithe+5);""",
 """            const FOGLargeNumber Actual=GetLastEffectiveHpDamage(T->UnitEntityId);
            if(Actual.IsZero())continue;
            RegisterPursuitOnDefeat(T->UnitEntityId);
            Own.RedTithe=FMath::Min(100,Own.RedTithe+5);"""),
("""        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(Id);
        if(!Actual.IsZero()&&Eligible.Contains(Id))""",
 """        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(Id);
        if(!Actual.IsZero())
            RegisterPursuitOnDefeat(Id);
        if(!Actual.IsZero()&&Eligible.Contains(Id))""")],
"Private/Combat/OGTurnBattle.cpp":[
("""    if (!ExecuteApprovedLuminousBaseAction(Action,*Source,OutError) ||
        !ExecuteApprovedLuminousAlterAction(Action,*Source,OutError))
        return false;

    FOGCombatLogEvent Resolved;""",
 """    if (!ExecuteApprovedLuminousBaseAction(Action,*Source,OutError) ||
        !ExecuteApprovedLuminousAlterAction(Action,*Source,OutError))
        return false;

    // Alter v1 Eclipse Harrier P4: apply only after verified final HP damage
    // by Alter to an actively source-bleeding enemy, once per owner action.
    // The executor runs after timeline scheduling: adjust the real recorded
    // NextActionValue here (never a cosmetic Cobalt speed variable).
    if(!bInterrupt)
        if(FLuminousAlterBattleRuntime* Alter=
               LuminousAlterStates.Find(Source->UnitEntityId))
            if(Alter->bPursuitDelayPending)
            {
                const int64 Remaining=FMath::Max<int64>(
                    1,Source->NextActionValue-State.CurrentActionValue);
                Source->NextActionValue=State.CurrentActionValue+
                    FMath::Max<int64>(1,Remaining*9200/10000);
                Alter->bPursuitDelayPending=false;
            }

    FOGCombatLogEvent Resolved;""")]
}
for name,pairs in targets.items():
 file=root/name
 s=file.read_text(encoding="utf-8-sig")
 for old,new in pairs:
  if s.count(old)!=1:raise RuntimeError(f"{name}: target count {s.count(old)} for {old[:115]!r}")
  s=s.replace(old,new,1)
 file.write_text(s,encoding="utf8",newline="")
 print("SOURCE_UPDATED",name)
print("ALTER_P4_PURSUIT_NATIVE_REAL_HP_KILL_8_PERCENT_ACTION_VALUE_IMPLEMENTED")
