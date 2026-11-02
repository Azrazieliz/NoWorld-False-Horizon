from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
def edit(f,pairs):
 s=f.read_text(encoding="utf-8-sig")
 for a,b in pairs:
  n=s.count(a)
  if n!=1:raise RuntimeError(f.name+" expected one anchor, found "+str(n)+" "+a[:75])
  s=s.replace(a,b,1)
 f.write_text(s,encoding="utf8",newline="")
 print("ALTER_FILE_CHANGED",f.name)
edit(r/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp",[
 ("""    const int32 MinRemainingBps=bA1||bA2?800:bA3?1500:0;
    const FOGLargeNumber SelfCost=""",
 """    const int32 MinRemainingBps=bA1||bA2?800:0;
    const FOGLargeNumber SelfCost="""),
 ("""    if(Own.RedTithe<CostRed || Own.UltimateChargePercent<CostCharge ||
       FOGLargeNumber::Compare(PostPayment,
           FOGLargeNumber::ScaleByBasisPoints(Source.Stats.MaxHp,
                                              MinRemainingBps))<=0)""",
 """    const bool bA3SourceHpEligible=!bA3 ||
        FOGLargeNumber::Compare(Source.CurrentHp,
            FOGLargeNumber::ScaleByBasisPoints(
                Source.Stats.MaxHp,1500))>0;
    if(Own.RedTithe<CostRed || Own.UltimateChargePercent<CostCharge ||
       !bA3SourceHpEligible ||
       FOGLargeNumber::Compare(PostPayment,
           FOGLargeNumber::ScaleByBasisPoints(Source.Stats.MaxHp,
                                              MinRemainingBps))<=0)"""),
 ("""        const FOGLargeNumber Tick=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.Attack,1600*It.Value());""",
 """        FOGLargeNumber Tick=FOGLargeNumber::ScaleByBasisPoints(
            Source.Stats.Attack,1600*It.Value());
        // Martyr Crown P2: stance raises only the proven registered
        // Bleed-derived Severing Wound potency, never generic DoT.
        if(Current<=Own.MartyrStanceExpires)
            Tick=FOGLargeNumber::ScaleByBasisPoints(Tick,12200);"""),
 ("""        Attacker.CritDamageBonusBps+=Own.MartyrMomentum*800;
        if(Current<=Own.DeathPriceExpiresOwnerAction)""",
 """        Attacker.CritDamageBonusBps+=Own.MartyrMomentum*800;
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
        if(Current<=Own.DeathPriceExpiresOwnerAction)"""),
 ("""        if(Current<=Own.MartyrStanceExpires)
        {
            Req.AdditionalMultiplierBps.Add(12600);
            Attacker.Speed=Attacker.Speed*11200/10000;
            Attacker.CritRateBps+=1800;
        }""",
 """        // Martyr Crown is already included in Attacker's actual ATK,
        // CritRate, Speed and conditional Bleed penetration; don't apply
        // a second duplicate +26% damage multiplier.""")
])
edit(r/"Private/Combat/OGTurnBattleLuminousPreflight.cpp",[
 ("""        const int32 Threshold=bA1||bA2?800:bA3?1500:0;
        const FOGLargeNumber Remaining=""",
 """        const int32 Threshold=bA1||bA2?800:0;
        const bool bA3SourceHpEligible=!bA3 ||
            FOGLargeNumber::Compare(Source.CurrentHp,
                FOGLargeNumber::ScaleByBasisPoints(
                    Source.Stats.MaxHp,1500))>0;
        const FOGLargeNumber Remaining="""),
 ("""        if(AlterCurrentTithe<TitheCost || CurrentCharge<ChargeCost ||
           FOGLargeNumber::Compare(Remaining,""",
 """        if(AlterCurrentTithe<TitheCost || CurrentCharge<ChargeCost ||
           !bA3SourceHpEligible ||
           FOGLargeNumber::Compare(Remaining,""")
])
print("ALTER_FULL_MARTYR_STANCE_SELF_COST_STAT_AND_WOUND_POTENCY_TIED_TO_REAL_NATIVE_STATS")
