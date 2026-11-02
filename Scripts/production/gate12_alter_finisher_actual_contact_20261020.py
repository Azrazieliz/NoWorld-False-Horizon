from pathlib import Path
p=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame\Private\Combat\OGTurnBattleLuminousAlterV1.cpp")
s=p.read_text(encoding="utf-8-sig")
patches=[
("""        constexpr int32 Steps[]={4800,6400,10500};
        for(int32 I=0;I<3;++I)
        {""",
 """        constexpr int32 Steps[]={4800,6400,10500};
        bool bActualThirdHit=false;
        for(int32 I=0;I<3;++I)
        {"""),
("""            if(!Strike(Id,Steps[I],1800,I==0||I==2,I==2 && Before>=2))
                return false;
            if(I==2 && Before>=3)""",
 """            if(!Strike(Id,Steps[I],1800,I==0||I==2,I==2 && Before>=2))
                return false;
            if(I==2)
                bActualThirdHit=FOGLargeNumber::Compare(
                    GetLastEffectiveHpDamage(Id),FOGLargeNumber())>0;
            if(I==2 && Before>=3 && bActualThirdHit)"""),
("""        if(Own.bMartyrNextFinisherPending &&
           Current<=Own.MartyrStanceExpires)
        {""",
 """        if(Own.bMartyrNextFinisherPending &&
           Current<=Own.MartyrStanceExpires && bActualThirdHit)
        {"""),
("""        const FOGEntityId Id=Action.TargetUnitIds[0];
        const bool bFinisher=Own.RuptureSeals.FindRef(Id)>=3;
        if(bFinisher)
        {
            Own.RuptureSeals.Remove(Id);
            Own.SealExpiryOwnerAction.Remove(Id);
        }
        constexpr int32 Steps[]={5500,6500,7500,9000,10500,13500};
        for(int32 I=0;I<6;++I)""",
 """        const FOGEntityId Id=Action.TargetUnitIds[0];
        const bool bEligibleFinisher=Own.RuptureSeals.FindRef(Id)>=3;
        bool bFirstHitConfirmed=false;
        constexpr int32 Steps[]={5500,6500,7500,9000,10500,13500};
        for(int32 I=0;I<6;++I)"""),
("""            if(!Strike(Id,Scale,2200,false,false))return false;
        }
        if(bFinisher && !Strike(Id,13000,2200,false,false))return false;""",
 """            if(!Strike(Id,Scale,2200,false,false))return false;
            if(I==0 && bEligibleFinisher &&
               FOGLargeNumber::Compare(GetLastEffectiveHpDamage(Id),
                                         FOGLargeNumber())>0)
            {
                // Hollow Crown v1 consumes three seals on a successful
                // first hit ONLY; no mark loss on blocked/missed opener.
                bFirstHitConfirmed=true;
                Own.RuptureSeals.Remove(Id);
                Own.SealExpiryOwnerAction.Remove(Id);
            }
        }
        if(bFirstHitConfirmed &&
           !Strike(Id,13000,2200,false,false))return false;""")
]
for old,new in patches:
 n=s.count(old)
 if n!=1:raise RuntimeError(f"Alter finisher mismatch {n}: {old[:100]}")
 s=s.replace(old,new,1)
p.write_text(s,encoding="utf8",newline="")
print("ALTER_V1_FINISHERS_REQUIRE_REAL_THIRD_AND_FIRST_HP_CONTACT_SOURCE_ONLY")
