from pathlib import Path
r=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
def edit(f,changes):
 s=f.read_text(encoding="utf-8-sig")
 for a,b in changes:
  n=s.count(a)
  if n!=1:raise RuntimeError(f"{f.name} expected one, found {n}: {a[:85]}")
  s=s.replace(a,b,1)
 f.write_text(s,encoding="utf8",newline="")
 print(f.name,len(changes))
edit(r/"Public/Combat/OGTurnBattle.h",[
 ("""        int32 NextFantasmReadyOwnerAction = 0;
    };""",
 """        int32 NextFantasmReadyOwnerAction = 0;
        // P6 Twilight Lawkeeper: source ContentIds from actually resolved
        // enemy actions targeted at this living Manifestation. Observing
        // is NOT acquiring permission to copy or automatically negate.
        TArray<FOGContentId> ObservedEnemyContentIds;
    };"""),
 ("""        bool bPursuitDelayPending = false;
        TMap<FOGEntityId,int32> RuptureSeals;""",
 """        bool bPursuitDelayPending = false;
        // P6 Witness Ledger: last three verified enemy effect ContentIds.
        TArray<FOGContentId> ObservedEnemyContentIds;
        TMap<FOGEntityId,int32> RuptureSeals;""")
])
edit(r/"Private/Combat/OGTurnBattle.cpp",[
 ("""    FOGCombatLogEvent Resolved;
    Resolved.Type = EOGCombatLogEventType::ActionResolved;""",
 """    // P6 observed-technique ledger: only actually resolved, enemy-owned,
    // explicitly targeting actions. Never infer invisible casts from a
    // globally enumerated skill library or observe on a rejected action.
    // An observed ContentId confers neither copying permission nor victory.
    for(const FOGEntityId& BeneficiaryId:Action.TargetUnitIds)
    {
        const FOGCombatUnitState* Observer=FindUnit(BeneficiaryId);
        if(!Observer || !Observer->IsAlive() ||
           Observer->TeamIndex==Source->TeamIndex ||
           !Action.SkillId.IsValid() ||
           Observer->IdentityId!=FOGContentId(
               TEXT("character:luminous_valentine.identity")))
            continue;
        TArray<FOGContentId>* Witnessed=nullptr;
        int32 Capacity=0;
        if(Observer->SkillSet.ActiveSkills.Contains(FOGContentId(
            TEXT("combat:luminous_valentine.base.skill.scarlet_cadence"))))
        {
            Witnessed=&LuminousBaseStates.FindOrAdd(
                BeneficiaryId).ObservedEnemyContentIds;
            Capacity=16; // Observation capacity != copied-technique limit 1.
        }
        else if(Observer->SkillSet.ActiveSkills.Contains(FOGContentId(
            TEXT("draft:luminous_valentine.alter.active_1"))))
        {
            Witnessed=&LuminousAlterStates.FindOrAdd(
                BeneficiaryId).ObservedEnemyContentIds;
            Capacity=3;
        }
        if(Witnessed && !Witnessed->Contains(Action.SkillId))
        {
            Witnessed->Add(Action.SkillId);
            if(Witnessed->Num()>Capacity)
                Witnessed->RemoveAt(0,Witnessed->Num()-Capacity);
        }
    }

    FOGCombatLogEvent Resolved;
    Resolved.Type = EOGCombatLogEventType::ActionResolved;""")
])
edit(r/"Private/Combat/OGTurnBattleLuminousWorldFantasm.cpp",[
 ("""        if((Target && Owner->TeamIndex==Target->TeamIndex) ||
           Field->bLastRoseUsed || !Contest.bProjectionCopyNotOriginal ||
           !Contest.bPreviouslyRecordedSameContentEffect)""",
 """        if((Target && Owner->TeamIndex==Target->TeamIndex) ||
           Field->bLastRoseUsed || !Contest.bProjectionCopyNotOriginal ||
           !Contest.bPreviouslyRecordedSameContentEffect ||
           !BaseState->ObservedEnemyContentIds.Contains(
               Contest.RegisteredEffectId))"""),
 ("""        if(Field->bAlterMirrorDebtUsed || !Contest.bProjectionCopyNotOriginal ||
           Contest.TargetUnitId!=Contest.OwnerManifestationId)""",
 """        if(Field->bAlterMirrorDebtUsed || !Contest.bProjectionCopyNotOriginal ||
           Contest.TargetUnitId!=Contest.OwnerManifestationId ||
           !AlterState->ObservedEnemyContentIds.Contains(
               Contest.RegisteredEffectId))""")
])
print("P6 actual per-Manifestation targeted witnessed technique provenance now authorizes observed projection rules")
