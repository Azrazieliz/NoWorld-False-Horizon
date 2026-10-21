from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h";c=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
def patch(p,pairs):
 s=p.read_text(encoding="utf-8-sig")
 for old,new in pairs:
  n=s.count(old)
  if n!=1: raise RuntimeError(f"{p.name} found {n} expected 1: {old[:92]}")
  s=s.replace(old,new,1)
 p.write_text(s,encoding="utf8",newline="")
 print("CHANGED",p.name)
if False: patch(h,[
 ("""        bool bDayAttunement = false;
        bool bAllyRevivalSpent = false;""",
  """        bool bDayAttunement = false;
        bool bAttunementActive = true; // Initial Night from approved Base v6.
        int32 AttunementExpiryAction = 2;
        int32 NextA2ReadyOwnerAction = 0;
        int32 NextA3ReadyOwnerAction = 0;
        int32 DiscountNextA2Reserve = 0;
        int32 LastBleedReserveOwnerAction = INDEX_NONE;
        bool bAllyRevivalSpent = false;"""),
])
patch(c,[
 ("""    if(bA3Covenant && StateOfOwner.bAllyRevivalSpent)
    {""",
  """    const int32 NextOwnerAction=StateOfOwner.OwnerActions+1;
    if((bA2 && NextOwnerAction<StateOfOwner.NextA2ReadyOwnerAction) ||
       (bA3 && NextOwnerAction<StateOfOwner.NextA3ReadyOwnerAction))
    {
        OutError=TEXT("Approved Base v6 two/three-owner-action cooldown not ready.");
        return false;
    }
    const int32 BaseReserveCost=bA2?
        FMath::Max(0,30-StateOfOwner.DiscountNextA2Reserve):
        bA3Covenant?75:bA3?30:0;
    if(BaseReserveCost>StateOfOwner.ScarletReserve)
    {
        OutError=TEXT("Base v6 insufficient Scarlet Reserve for actual skill cost.");
        return false;
    }
    if(bA3Covenant && StateOfOwner.bAllyRevivalSpent)
    {"""),

 ("""    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;

    // Own-action regeneration""",
  """    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;
    if(bA2)
    {
        StateOfOwner.ScarletReserve-=BaseReserveCost;
        StateOfOwner.DiscountNextA2Reserve=0;
        StateOfOwner.NextA2ReadyOwnerAction=NextOwnerAction+3;
    }
    else if(bA3)
    {
        if(!bA3Covenant)StateOfOwner.ScarletReserve-=BaseReserveCost;
        StateOfOwner.NextA3ReadyOwnerAction=NextOwnerAction+4;
    }

    // Own-action regeneration"""),
 ("""    ++StateOfOwner.OwnerActions;
    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;""",
  """    ++StateOfOwner.OwnerActions;
    const int32 CurrentOwnerAction=StateOfOwner.OwnerActions;
    if(StateOfOwner.bAttunementActive &&
       CurrentOwnerAction>StateOfOwner.AttunementExpiryAction)
        StateOfOwner.bAttunementActive=false; // Neutral, neither Night nor Day."""),
 ("""        StateOfOwner.ScarletReserve=FMath::Min(
            Kit.ScarletReserveMax,StateOfOwner.ScarletReserve+6);""",
  """        if(StateOfOwner.LastBleedReserveOwnerAction!=CurrentOwnerAction &&
           !Actual.IsZero())
        {
            StateOfOwner.ScarletReserve=FMath::Min(
                Kit.ScarletReserveMax,StateOfOwner.ScarletReserve+6);
            StateOfOwner.LastBleedReserveOwnerAction=CurrentOwnerAction;
        }"""),
 ("""        if(StateOfOwner.bDayAttunement==false &&
           StateOfOwner.OwnerActions>0)""",
  """        if(StateOfOwner.bAttunementActive &&
           !StateOfOwner.bDayAttunement)"""),
 ("""        StateOfOwner.bDayAttunement=false;
    }
    else if((bA3 && !bA3Covenant) ||""",
  """        StateOfOwner.bDayAttunement=false;
        StateOfOwner.bAttunementActive=true;
        StateOfOwner.AttunementExpiryAction=CurrentOwnerAction+2;
    }
    else if(bA3 ||"""),
 ("""            if(StateOfOwner.bDayAttunement)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,11200);""",
  """            if(StateOfOwner.bAttunementActive && StateOfOwner.bDayAttunement)
                Heal=FOGLargeNumber::ScaleByBasisPoints(Heal,11200);"""),
 ("""        StateOfOwner.bDayAttunement=true;
        StateOfOwner.DawnOathExpiry=CurrentOwnerAction+2;""",
  """        if(bA3)
        {
            StateOfOwner.bDayAttunement=true;
            StateOfOwner.bAttunementActive=true;
            StateOfOwner.AttunementExpiryAction=CurrentOwnerAction+2;
        }
        StateOfOwner.DawnOathExpiry=CurrentOwnerAction+2;"""),
 ("""        StateOfOwner.bDayAttunement=false;
    }

    if(!ActionLifesteal.IsZero())""",
  """        // Both Ultimates are independently selectable and preserve existing
        // attunement: they do NOT force the owner into Night.
    }

    if(!ActionLifesteal.IsZero())"""),
])
print("BASE_V6_VERIFIED_SOURCE_COSTS_COOLDOWNS_ATTUNEMENT_AND_BLEED_DEDUP_SOURCE_ONLY")
