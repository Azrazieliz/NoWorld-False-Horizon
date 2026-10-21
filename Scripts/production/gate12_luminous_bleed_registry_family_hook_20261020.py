from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=r/"Public/Combat/OGTurnBattle.h"
b=r/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
a=r/"Private/Combat/OGTurnBattleLuminousAlterV1.cpp"
def patch(p,items):
 s=p.read_text(encoding="utf-8-sig")
 for old,new in items:
  ct=s.count(old)
  if ct!=1:raise Exception(f"{p.name} {ct} matches for {old[:100]}")
  s=s.replace(old,new,1)
 p.write_text(s,encoding="utf-8",newline="")
 print("UPDATED",p.name)
patch(h,[
 ("""        FOGLuminousNativeBloodEligibility InBlood,
        FOGLuminousNativeRevivalEligibility InRevival)
    {
        LuminousBloodEligibility=MoveTemp(InBlood);
        LuminousRevivalEligibility=MoveTemp(InRevival);
    }""",
  """        FOGLuminousNativeBloodEligibility InBlood,
        FOGLuminousNativeRevivalEligibility InRevival,
        FOGContentId InAcceptedBleedFamilyId)
    {
        LuminousBloodEligibility=MoveTemp(InBlood);
        LuminousRevivalEligibility=MoveTemp(InRevival);
        AcceptedBleedFamilyId=MoveTemp(InAcceptedBleedFamilyId);
    }"""),
 ("""    FOGLuminousNativeRevivalEligibility LuminousRevivalEligibility;
    bool IsLuminousBloodStatusEligible(""",
  """    FOGLuminousNativeRevivalEligibility LuminousRevivalEligibility;
    FOGContentId AcceptedBleedFamilyId;
    bool IsLuminousBloodStatusEligible("""),
 ("""        if(!LuminousBloodEligibility)return false;
        const FOGCombatUnitState* Attacker=FindUnit(AttackerId);""",
  """        // Bleed family ID must be provided from installed Registry; a
        // hand-made draft token is not permission to create blood statuses.
        if(!LuminousBloodEligibility || !AcceptedBleedFamilyId.IsValid() ||
           StatusFamily!=AcceptedBleedFamilyId)return false;
        const FOGCombatUnitState* Attacker=FindUnit(AttackerId);""")
])
patch(b,[
 ("""            FOGContentId(TEXT("combat:bleed"))))""",
  """            AcceptedBleedFamilyId))""")
])
patch(a,[
 ("""            FOGContentId(TEXT("combat:bleed"))))""",
  """            AcceptedBleedFamilyId))""")
])
patch(r/"Private/Combat/OGTurnBattle.cpp",[
 ("""    LuminousRevivalEligibility=FOGLuminousNativeRevivalEligibility();
    IdentityContext =""",
  """    LuminousRevivalEligibility=FOGLuminousNativeRevivalEligibility();
    AcceptedBleedFamilyId=FOGContentId();
    IdentityContext =""")
])
print("REGISTRY_BINDING_FOR_BLEED_FAMILY_INSTEAD_OF_FABRICATED_CONTENT_ID")
