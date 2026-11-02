from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
a=r/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
b=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
def replace(file,items):
 s=file.read_text(encoding="utf-8-sig")
 for old,new in items:
  n=s.count(old)
  if n!=1:raise RuntimeError(f"{file.name} expected one: {n}: {old[:80]}")
  s=s.replace(old,new,1)
 file.write_text(s,encoding="utf-8",newline="")
 print("CHANGED",file.name)
replace(h,[
("        TMap<FOGEntityId,int32> RuptureSeals;",
 "        TMap<FOGEntityId,int32> RuptureSeals;\n"
 "        TMap<FOGEntityId,int32> SealExpiryOwnerAction;")
])
replace(a,[
("""    if(!SelfCost.IsZero())
    {
        // A cost, not enemy damage.""",
 """    if(!bA1 && !SelfCost.IsZero())
    {
        // A cost, not enemy damage."""),
("""    // Registered Severing Wound ticks. End at source-approved 2 owner turns.
    for(auto It=Own.SeveringWounds.CreateIterator();It;++It)""",
 """    // Alter-owned Rupture Seals expire after three owner turns; an
    // expired seal cannot fuel a delayed bonus or World Fantasm contest.
    for(auto It=Own.RuptureSeals.CreateIterator();It;++It)
    {
        if(Own.SealExpiryOwnerAction.FindRef(It.Key())<Current)
        {
            Own.SealExpiryOwnerAction.Remove(It.Key());
            It.RemoveCurrent();
        }
    }
    // Registered Severing Wound ticks. End at source-approved 2 owner turns.
    for(auto It=Own.SeveringWounds.CreateIterator();It;++It)"""),
("""            if(bSeal)Own.RuptureSeals.FindOrAdd(Id)=
                FMath::Min(3,Own.RuptureSeals.FindRef(Id)+1);""",
 """            if(bSeal)
            {
                Own.RuptureSeals.FindOrAdd(Id)=
                    FMath::Min(3,Own.RuptureSeals.FindRef(Id)+1);
                Own.SealExpiryOwnerAction.Add(Id,Current+3);
            }"""),
("""                Own.RuptureSeals.Add(Id,0);
                if(!Strike(Id,7500,1800,false,false))return false;""",
 """                Own.RuptureSeals.Remove(Id);
                Own.SealExpiryOwnerAction.Remove(Id);
                if(!Strike(Id,7500,1800,false,false))return false;"""),
("""        }
    }
    else if(bA2)
    {
        const bool bFinisher=Own.RuptureSeals.FindRef(Action.TargetUnitIds[0])>=3;""",
 """        }
        // The v1 vitality offering is committed AFTER the third blade
        // swing, once even if that swing missed. It is not free opening
        // Momentum on the earlier two strikes.
        const FOGLargeNumber FinalPost=FOGLargeNumber::Add(
            Source.CurrentHp,AlterMinus(SelfCost));
        if(FOGLargeNumber::Compare(FinalPost,FOGLargeNumber())>0)
        {
            Source.CurrentHp=FinalPost;
            Own.RedTithe=FMath::Min(100,Own.RedTithe+8);
            Own.MartyrMomentum=FMath::Min(3,Own.MartyrMomentum+1);
        }
    }
    else if(bA2)
    {
        const bool bFinisher=Own.RuptureSeals.FindRef(Action.TargetUnitIds[0])>=3;"""),
("""        if(bFinisher)Own.RuptureSeals.Add(Action.TargetUnitIds[0],0);""",
 """        if(bFinisher)
        {
            Own.RuptureSeals.Remove(Action.TargetUnitIds[0]);
            Own.SealExpiryOwnerAction.Remove(Action.TargetUnitIds[0]);
        }"""),
("""        if(bFinisher)Own.RuptureSeals.Add(Id,0);""",
 """        if(bFinisher)
        {
            Own.RuptureSeals.Remove(Id);
            Own.SealExpiryOwnerAction.Remove(Id);
        }"""),
])
replace(b,[
("""    if(bA3Covenant && StateOfOwner.ScarletReserve<75)
    {""",
 """    if(bA3Covenant && StateOfOwner.bAllyRevivalSpent)
    {
        OutError=TEXT("The shared ally revival ledger was already consumed in this encounter.");
        return false;
    }
    if(bA3Covenant && StateOfOwner.ScarletReserve<75)
    {""")
])
print("EXACT_REVISION_SOURCE_ONLY_NO_COMPILE")
