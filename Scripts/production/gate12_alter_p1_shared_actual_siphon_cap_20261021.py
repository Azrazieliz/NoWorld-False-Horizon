from pathlib import Path
p=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame\Private\Combat\OGTurnBattleLuminousAlterV1.cpp")
s=p.read_text(encoding="utf-8-sig")
swaps=[
("""    // Registered Severing Wound ticks. End at source-approved 2 owner turns.
    for(auto It=Own.SeveringWounds.CreateIterator();It;++It)""",
 """    // P1 Blood's Accounting: DIRECT SWORD AND REGISTERED WOUND TICKS share
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

    // Registered Severing Wound ticks. End at source-approved 2 owner turns.
    for(auto It=Own.SeveringWounds.CreateIterator();It;++It)"""),
("""            const FOGLargeNumber Heal=AlterMin(
                FOGLargeNumber::ScaleByBasisPoints(Actual,900),
                AlterMissing(Source));
            if(!Heal.IsZero() &&
               !ApplyHealing(Source.UnitEntityId,Source.UnitEntityId,
                              Heal,Action.SkillId,OutError))return false;""",
 """            // 9% Wound Siphon uses the SAME allowance as direct damage.
            const FOGLargeNumber Heal=FOGLargeNumber::ScaleByBasisPoints(
                Actual,900);
            if(!SiphonWithinAllowance(Heal))return false;"""),
("""    // Alter's P1 18% (Harvest replacement 22%), hard 16% owner max-HP
    // allowance, and only actual blood-eligible target HP loss.
    const FOGLargeNumber Cap=FOGLargeNumber::ScaleByBasisPoints(
        Source.Stats.MaxHp,1600);
    FOGLargeNumber Heal=AlterMin(AlterMin(SiphonSum,Cap),
                                 AlterMissing(Source));
    if(Current<=Own.DeathPriceExpiresOwnerAction)
        Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,7000);
    if(!Heal.IsZero() &&
       !ApplyHealing(Source.UnitEntityId,Source.UnitEntityId,
                     Heal,Action.SkillId,OutError))return false;
    return true;""",
 """    // Remaining direct sword siphon (18%, Harvest 22%) is applied once
    // using the unused portion of the exact same owner-action HP cap.
    if(!SiphonWithinAllowance(SiphonSum))return false;
    return true;""")
]
for a,b in swaps:
 n=s.count(a)
 if n!=1:raise RuntimeError(f"Alter P1 mismatch count={n} anchor={a[:85]}")
 s=s.replace(a,b,1)
# int basis point inverse accurate: denominator=10000+BuffHealing, basis 10000*10000/den
p.write_text(s,encoding="utf8",newline="")
print("IMPLEMENTED Alter P1 one shared 16%-max-HP cap: DoT and direct sword, DeathPrice and Dawn receive scaling.")
