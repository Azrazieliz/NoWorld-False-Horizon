from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=root/"Public/Combat/OGTurnBattle.h"
a=root/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
def edit(p,swaps):
 s=p.read_text(encoding="utf-8-sig")
 for before,after in swaps:
  n=s.count(before)
  if n!=1:raise ValueError(f"{p.name}: {n} matches {before[:55]}")
  s=s.replace(before,after,1)
 p.write_text(s,encoding="utf-8",newline="")
 print("UPDATED",p.name)
edit(h,[
("""        int32 MartyrMomentum = 0;
        int32 MartyrStanceExpires = -1;""",
 """        int32 MartyrMomentum = 0;
        int32 MomentumExpiresOwnerAction = -1;
        int32 MartyrStanceExpires = -1;
        bool bMartyrNextFinisherPending = false;
        int32 DeathPriceExpiresOwnerAction = -1;""")
])
edit(a,[
("""    Own.OwnerActions=Current;
    if(!bA1 && !SelfCost.IsZero())""",
 """    Own.OwnerActions=Current;
    if(Current>Own.MomentumExpiresOwnerAction)
        Own.MartyrMomentum=0;
    if(!bA1 && !SelfCost.IsZero())"""),
("""        Own.MartyrMomentum=FMath::Min(3,Own.MartyrMomentum+1);
    }
    if(bA2)""",
 """        Own.MartyrMomentum=FMath::Min(3,Own.MartyrMomentum+1);
        Own.MomentumExpiresOwnerAction=Current+2;
    }
    if(bA2)"""),
("""        Own.MartyrStanceExpires=Current+2;
        return true;""",
 """        Own.MartyrStanceExpires=Current+2;
        Own.bMartyrNextFinisherPending=true;
        return true;"""),
("""        Attacker.CritDamageBonusBps+=Own.MartyrMomentum*800;""",
 """        Attacker.CritDamageBonusBps+=Own.MartyrMomentum*800;
        if(Current<=Own.DeathPriceExpiresOwnerAction)
            Attacker.CritDamageBonusBps+=2000;"""),
("""        }
        // The v1 vitality offering is committed AFTER the third blade""",
 """        }
        if(Own.bMartyrNextFinisherPending &&
           Current<=Own.MartyrStanceExpires)
        {
            Own.bMartyrNextFinisherPending=false;
            if(!Strike(Id,5000,1800,false,false))return false;
        }
        // The v1 vitality offering is committed AFTER the third blade"""),
("""            Own.MartyrMomentum=FMath::Min(3,Own.MartyrMomentum+1);
        }
    }
    else if(bA2)""",
 """            Own.MartyrMomentum=FMath::Min(3,Own.MartyrMomentum+1);
            Own.MomentumExpiresOwnerAction=Current+2;
        }
    }
    else if(bA2)"""),
("""    const FOGLargeNumber Heal=AlterMin(AlterMin(SiphonSum,Cap),
                                     AlterMissing(Source));""",
 """    FOGLargeNumber Heal=AlterMin(AlterMin(SiphonSum,Cap),
                                 AlterMissing(Source));
    if(Current<=Own.DeathPriceExpiresOwnerAction)
        Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,7000);"""),
("""    R->RedTithe-=75;
    R->bSelfRevived=true;""",
 """    R->RedTithe-=75;
    R->bSelfRevived=true;
    R->DeathPriceExpiresOwnerAction=R->OwnerActions+1;"""),
])
print("ALTER_PASSIVE_TIMED_BUFFS_AND_AFTER_REVIVAL_PRICE_ADDED_NO_BUILD")
