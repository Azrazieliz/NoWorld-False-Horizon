from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
def edit(name,pairs):
 f=root/name;s=f.read_text(encoding="utf-8-sig")
 for a,b in pairs:
  n=s.count(a)
  if n!=1:raise RuntimeError(f"{name}: found {n} required 1 for {a[:90]!r}")
  s=s.replace(a,b,1)
 f.write_text(s,encoding="utf8",newline="")
 print("SOURCE_UPDATED",name)
edit("Public/Combat/OGTurnBattle.h",[
 ("""        TMap<FOGEntityId,int32> BlessingExpiry;
        int32 DawnOathExpiry = 0;""",
 """        TMap<FOGEntityId,int32> BlessingExpiry;
        // 1=A3 Benediction/Covenant; 2=Day Ultimate; 2 owner turns.
        TMap<FOGEntityId,uint8> BlessingCategory;
        int32 DawnOathExpiry = 0;"""),
 ("""    void GrantAuthoredDawnOverhealBarrier(
        const FOGEntityId& OwnerId,""",
 """    void GetApprovedDawnBlessingBps(
        const FOGEntityId& BeneficiaryId,
        int32& OutAttackBonusBps,
        int32& OutSpeedBonusBps,
        int32& OutHealingReceivedBonusBps,
        int32& OutDamageReductionBps) const;

    void GrantAuthoredDawnOverhealBarrier(
        const FOGEntityId& OwnerId,""")
])
edit("Private/Combat/OGTurnBattleLuminousBaseV6.cpp",[
 ("""        FOGCombatStats EffectiveAttacker=Source.Stats;
        if(StateOfOwner.HemalMarks.FindRef(TargetId)>=2)""",
 """        FOGCombatStats EffectiveAttacker=Source.Stats;
        int32 DawnAtk=0,DawnSpd=0,DawnHeal=0,DawnReduction=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,
            DawnAtk,DawnSpd,DawnHeal,DawnReduction);
        if(DawnAtk>0)
            EffectiveAttacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                EffectiveAttacker.Attack,10000+DawnAtk);
        if(DawnSpd>0)
            EffectiveAttacker.Speed=EffectiveAttacker.Speed*
                (10000+DawnSpd)/10000;
        if(StateOfOwner.HemalMarks.FindRef(TargetId)>=2)"""),
 ("""            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            if(bA3 && !Heal.IsZero())""",
 """            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            StateOfOwner.BlessingCategory.Add(Id,bDayUltimate?2:1);
            if(bA3 && !Heal.IsZero())"""),
 ("""        S->BlessingExpiry.Add(TargetUnitId,S->OwnerActions+2);
        return;""",
 """        S->BlessingExpiry.Add(TargetUnitId,S->OwnerActions+2);
        S->BlessingCategory.Add(TargetUnitId,1);
        return;""")
])
edit("Private/Combat/OGTurnBattleLuminousAlterV1.cpp",[
 ("""        FOGCombatStats Attacker=Source.Stats;
        Attacker.CritDamageBonusBps+=Own.MartyrMomentum*800;""",
 """        FOGCombatStats Attacker=Source.Stats;
        int32 DawnAtk=0,DawnSpd=0,DawnHeal=0,DawnReduction=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,
            DawnAtk,DawnSpd,DawnHeal,DawnReduction);
        if(DawnAtk>0)
            Attacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                Attacker.Attack,10000+DawnAtk);
        if(DawnSpd>0)
            Attacker.Speed=Attacker.Speed*(10000+DawnSpd)/10000;
        Attacker.CritDamageBonusBps+=Own.MartyrMomentum*800;"""),
 ("""        Req.BaseDamage=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.Attack,AtkBps);""",
 """        Req.BaseDamage=FOGLargeNumber::ScaleByBasisPoints(
            Attacker.Attack,AtkBps);""")
])
edit("Private/Combat/OGTurnBattle.cpp",[
 ("""    FOGLargeNumber EffectiveAmount=Amount;
    FOGLargeNumber Absorbed;""",
 """    int32 DawnAtk=0,DawnSpeed=0,DawnHealing=0,DawnReduction=0;
    GetApprovedDawnBlessingBps(TargetUnitId,
        DawnAtk,DawnSpeed,DawnHealing,DawnReduction);
    // Day Ultimate typed 14% mitigation acts before Dawn barriers.
    FOGLargeNumber EffectiveAmount=DawnReduction>0
        ?FOGLargeNumber::ScaleByBasisPoints(Amount,10000-DawnReduction):Amount;
    FOGLargeNumber Absorbed;"""),
 ("""    const FOGLargeNumber HpBefore=Target->CurrentHp;
    const FOGLargeNumber NominalRestored=FOGLargeNumber::Add(HpBefore,Amount);""",
 """    const FOGLargeNumber HpBefore=Target->CurrentHp;
    int32 DawnAtk=0,DawnSpeed=0,DawnHealing=0,DawnReduction=0;
    GetApprovedDawnBlessingBps(TargetUnitId,
        DawnAtk,DawnSpeed,DawnHealing,DawnReduction);
    const FOGLargeNumber Received=DawnHealing>0
        ?FOGLargeNumber::ScaleByBasisPoints(Amount,10000+DawnHealing):Amount;
    const FOGLargeNumber NominalRestored=FOGLargeNumber::Add(HpBefore,Received);""")
])
print("AUTHORED_A3_DAY_ULT_TURN_CORE_BUFF_WIRING")
