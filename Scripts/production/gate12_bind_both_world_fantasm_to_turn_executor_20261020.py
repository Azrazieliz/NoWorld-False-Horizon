from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
battle=r/"Private/Combat/OGTurnBattle.cpp"
base=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
alter=r/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
def patch(p,pairs):
 s=p.read_text(encoding="utf-8-sig")
 for before,after in pairs:
  count=s.count(before)
  if count!=1:raise RuntimeError(f"{p.name} expected one, found {count}: {before[:92]}")
  s=s.replace(before,after,1)
 p.write_text(s,encoding="utf8",newline="")
 print("WIRED",p.name)
patch(battle,[
 ("""    LuminousAlterStates.Reset();
    IdentityContext =""",
  """    LuminousAlterStates.Reset();
    LuminousFields.Reset();
    LuminousFieldEligibility={};
    IdentityContext ="""),
])
patch(base,[
 ("""    const bool bUlt=Action.SkillId==Kit.CoreSkills.UltimateSkill;
    if (!bA1 && !bA2 && !bA3 && !bUlt)
        return true;""",
  """    const bool bUlt=Action.SkillId==Kit.CoreSkills.UltimateSkill;
    const bool bField=Action.SkillId==Kit.WorldFantasmId;
    if (!bA1 && !bA2 && !bA3 && !bUlt && !bField)
        return true;"""),
 ("""    TSharedPtr<FJsonObject> Context;
    if (!FJsonSerializer::Deserialize(""",
  """    if(bField)
        return TryActivateLuminousAuthoredField(Source,Kit.WorldFantasmId,OutError);

    TSharedPtr<FJsonObject> Context;
    if (!FJsonSerializer::Deserialize("""),
 ("""        StateOfOwner.bDayAttunement=true;
    }
    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;""",
  """        StateOfOwner.bDayAttunement=true;
        StateOfOwner.bAttunementActive=true;
        StateOfOwner.AttunementExpiryAction=StateOfOwner.OwnerActions+3;
    }
    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;"""),
 ("""            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            if(!bGrantedOnHpDamage)""",
  """            StateOfOwner.BlessingExpiry.Add(Id,CurrentOwnerAction+2);
            if(bA3)
                StateOfOwner.DawnOathByBeneficiary.Add(Id,CurrentOwnerAction+2);
            if(!bGrantedOnHpDamage)"""),
])
patch(alter,[
 ("""    const bool bUlt=Action.SkillId==Ultimate;
    if(!bA1&&!bA2&&!bA3&&!bUlt)return true;""",
  """    const bool bUlt=Action.SkillId==Ultimate;
    const bool bField=Action.SkillId==Approved.WorldFantasmId;
    if(!bA1&&!bA2&&!bA3&&!bUlt&&!bField)return true;"""),
 ("""    TSharedPtr<FJsonObject> Context;
    if(!FJsonSerializer::Deserialize(""",
  """    if(bField)
        return TryActivateLuminousAuthoredField(
            Source,Approved.WorldFantasmId,OutError);

    TSharedPtr<FJsonObject> Context;
    if(!FJsonSerializer::Deserialize("""),
])
print("BOTH_FIELD_ACTIVATION_PATHS_CONNECTED")
