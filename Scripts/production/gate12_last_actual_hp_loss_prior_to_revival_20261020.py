from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
c=r/"Private/Combat/OGTurnBattle.cpp"
b=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
a=r/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
def patch(path,swaps):
 s=path.read_text(encoding="utf-8-sig")
 for old,new in swaps:
  count=s.count(old)
  if count!=1:raise RuntimeError(f"{path.name}: {count} matches for {old[:95]}")
  s=s.replace(old,new,1)
 path.write_text(s,encoding="utf8",newline="")
 print("CORRECTED",path.name)
patch(h,[
 ("""    bool ApplyHealing(
        const FOGEntityId& SourceUnitId,""",
  """    /** Exactly the HP lost by the most recently resolved damage event,
     * after barriers and overkill but before any defeat-triggered revival.
     * Never infer actual HP loss by inspecting HP after an OnDefeat reaction. */
    FOGLargeNumber GetLastEffectiveHpDamage(
        const FOGEntityId& TargetId) const
    {
        return LastEffectiveHpDamage.FindRef(TargetId);
    }

    bool ApplyHealing(
        const FOGEntityId& SourceUnitId,"""),
 ("""    TArray<FLuminousDawnBarrier> LuminousDawnBarriers;
    void GrantAuthoredDawnOverhealBarrier(""",
  """    TArray<FLuminousDawnBarrier> LuminousDawnBarriers;
    TMap<FOGEntityId,FOGLargeNumber> LastEffectiveHpDamage;
    void GrantAuthoredDawnOverhealBarrier("""),
])
patch(c,[
 ("""    LuminousDawnBarriers.Reset();
    LuminousAlterStates.Reset();""",
  """    LuminousDawnBarriers.Reset();
    LastEffectiveHpDamage.Reset();
    LuminousAlterStates.Reset();"""),
 ("""    FOGLargeNumber NegativeAmount(
        -EffectiveAmount.Significand,
        EffectiveAmount.Exponent10);

    Target->CurrentHp =""",
  """    // Snapshot actual HP loss BEFORE the OnDefeat trigger fires. Overkill
    // beyond remaining HP is not a vampirism/status/resource source.
    LastEffectiveHpDamage.Add(TargetUnitId,
        FOGLargeNumber::Compare(EffectiveAmount,Target->CurrentHp)<0
            ?EffectiveAmount:Target->CurrentHp);
    FOGLargeNumber NegativeAmount(
        -EffectiveAmount.Significand,
        EffectiveAmount.Exponent10);

    Target->CurrentHp ="""),
])
patch(b,[
 ("""        const FOGLargeNumber Actual=FOGLargeNumber::Add(
            Before,FOGLargeNumber(-T->CurrentHp.Significand,
                                   T->CurrentHp.Exponent10));""",
  """        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(T->UnitEntityId);"""),
 ("""        const FOGLargeNumber Actual=FOGLargeNumber::Add(
            Before,FOGLargeNumber(-Target->CurrentHp.Significand,
                                   Target->CurrentHp.Exponent10));""",
  """        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(TargetId);"""),
])
patch(a,[
 ("""            const FOGLargeNumber Actual=FOGLargeNumber::Add(
                Before,FOGLargeNumber(-T->CurrentHp.Significand,
                                      T->CurrentHp.Exponent10));""",
  """            const FOGLargeNumber Actual=GetLastEffectiveHpDamage(T->UnitEntityId);"""),
 ("""        const FOGLargeNumber Actual=FOGLargeNumber::Add(
            Before,FOGLargeNumber(-T->CurrentHp.Significand,
                                   T->CurrentHp.Exponent10));""",
  """        const FOGLargeNumber Actual=GetLastEffectiveHpDamage(Id);"""),
])
print("SHIELD_REVIVE_OVERKILL_DAMAGE_PROVENANCE_FIXED_IN_SHARED_EXECUTOR")
