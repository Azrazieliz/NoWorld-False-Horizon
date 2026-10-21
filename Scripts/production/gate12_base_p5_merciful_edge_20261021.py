from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
def change(f,pairs):
 s=f.read_text(encoding="utf-8-sig")
 for a,b in pairs:
  if s.count(a)!=1:raise RuntimeError("Expected exact source anchor: "+f.name+" "+a[:90])
  s=s.replace(a,b,1)
 f.write_text(s,encoding="utf8",newline="")
 print("UPDATED",f.name)
change(r/"Public/Combat/OGTurnBattle.h",[
 ("""        int32 RevivalAfterglowExpiresOwnerAction = INDEX_NONE;
        int32 LastNightExecutionChargeOwnerAction = INDEX_NONE;""",
 """        int32 RevivalAfterglowExpiresOwnerAction = INDEX_NONE;
        int32 MercifulEdgeExpiresOwnerAction = INDEX_NONE;
        int32 LastNightExecutionChargeOwnerAction = INDEX_NONE;""")
])
change(r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp",[
 ("""    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;
    if(bDayRevivalPrepare)""",
 """    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;
    const bool bMercifulEdgeForThisCast=
        (bA1 || bA2 || (bUlt && UltimateBranch==TEXT("night"))) &&
        StateOfOwner.MercifulEdgeExpiresOwnerAction>=CurrentOwnerAction;
    if(bMercifulEdgeForThisCast)
        StateOfOwner.MercifulEdgeExpiresOwnerAction=INDEX_NONE;
    if(bDayRevivalPrepare)"""),
 ("""        if(CurrentOwnerAction<=StateOfOwner.RevivalAfterglowExpiresOwnerAction)
            EffectiveAttacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                EffectiveAttacker.Attack,11200); // Reconstitution P4""",
 """        if(CurrentOwnerAction<=StateOfOwner.RevivalAfterglowExpiresOwnerAction)
            EffectiveAttacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                EffectiveAttacker.Attack,11200); // Reconstitution P4
        if(bMercifulEdgeForThisCast)
            EffectiveAttacker.Attack=FOGLargeNumber::ScaleByBasisPoints(
                EffectiveAttacker.Attack,11000); // Dawn Sanctification P5"""),
 ("""            if(bA3 && FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0)
                StateOfOwner.DawnOathByBeneficiary.Add(
                    Id,CurrentOwnerAction+2);""",
 """            if(bA3 && FOGLargeNumber::Compare(
                RealRestored,FOGLargeNumber())>0)
            {
                StateOfOwner.DawnOathByBeneficiary.Add(
                    Id,CurrentOwnerAction+2);
                // P5 Merciful Edge requires an actual A3 rescue that moves
                // the ally from <=35% HP to >=35%. Ordinary full-HP buff
                // casts, self-only lifesteal and failed heals never grant it.
                const FOGLargeNumber Threshold=FOGLargeNumber::ScaleByBasisPoints(
                    T->Stats.MaxHp,3500);
                if(FOGLargeNumber::Compare(
                        BeforeHealing,Threshold)<=0 &&
                   FOGLargeNumber::Compare(T->CurrentHp,Threshold)>=0)
                    StateOfOwner.MercifulEdgeExpiresOwnerAction=
                        CurrentOwnerAction+1;
            }""")
])
print("Base P5 Merciful Edge: actual-crossing conditional +10% native Attack on next sword/blood cast; one cast, one owner-action deadline")
