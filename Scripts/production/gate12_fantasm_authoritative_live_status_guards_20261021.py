from pathlib import Path
f=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame\Private\Combat\OGTurnBattleLuminousWorldFantasm.cpp")
s=f.read_text(encoding="utf-8-sig")
pairs=[
("""        for(const auto& Row:Owner.HemalMarks)
            if(Row.Value>0){bPrepared=true;break;}""",
 """        // A mark that already expired by the activation action does not
        // satisfy the source's actual preparation requirement.
        for(const auto& Row:Owner.HemalMarks)
            if(Row.Value>0 &&
               Owner.HemalMarkExpiryOwnerAction.FindRef(Row.Key)>=ActionNumber)
            {bPrepared=true;break;}"""),
("""           BaseState->HemalMarks.FindRef(Contest.TargetUnitId)<3 ||
           Field->NocturneConsumedTargets.Contains(Contest.TargetUnitId))""",
 """           BaseState->HemalMarks.FindRef(Contest.TargetUnitId)<3 ||
           BaseState->HemalMarkExpiryOwnerAction.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->NocturneConsumedTargets.Contains(Contest.TargetUnitId))"""),
("""           BaseState->DawnOathByBeneficiary.FindRef(Contest.TargetUnitId)<CurrentAction ||
           Field->RekindledAllies.Contains(Contest.TargetUnitId) ||""",
 """           !BaseState->DawnOathByBeneficiary.Contains(Contest.TargetUnitId) ||
           BaseState->DawnOathByBeneficiary.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->RekindledAllies.Contains(Contest.TargetUnitId) ||"""),
("""           AlterState->SeveringWounds.FindRef(Contest.TargetUnitId)<2 ||
           Field->AlterBloodReversalSuccessCount>=2 ||""",
 """           AlterState->SeveringWounds.FindRef(Contest.TargetUnitId)<2 ||
           AlterState->WoundExpiryOwnerAction.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->AlterBloodReversalSuccessCount>=2 ||"""),
("""           AlterState->RuptureSeals.FindRef(Contest.TargetUnitId)<3 ||
           Field->AlterThornConsumedTargets.Contains(Contest.TargetUnitId))""",
 """           AlterState->RuptureSeals.FindRef(Contest.TargetUnitId)<3 ||
           AlterState->SealExpiryOwnerAction.FindRef(
               Contest.TargetUnitId)<CurrentAction ||
           Field->AlterThornConsumedTargets.Contains(Contest.TargetUnitId))"""),
("""        BaseState->HemalMarks.FindOrAdd(Contest.TargetUnitId)-=3;
        Field->NocturneConsumedTargets.Add(Contest.TargetUnitId);""",
 """        // Deplete source-owned eligible marks only AFTER the callback has
        // actually removed the one authority-approved registered effect.
        BaseState->HemalMarks.FindOrAdd(Contest.TargetUnitId)-=3;
        if(BaseState->HemalMarks.FindRef(Contest.TargetUnitId)<=0)
        {
            BaseState->HemalMarks.Remove(Contest.TargetUnitId);
            BaseState->HemalMarkExpiryOwnerAction.Remove(
                Contest.TargetUnitId);
            BaseState->PendingFourMarkTargetBonus.Remove(
                Contest.TargetUnitId);
        }
        Field->NocturneConsumedTargets.Add(Contest.TargetUnitId);""")
]
for a,b in pairs:
 n=s.count(a)
 if n!=1:raise RuntimeError(f"source field patch target must occur once; found {n}: {a[:100]}")
 s=s.replace(a,b,1)
f.write_text(s,encoding="utf8",newline="")
print("CHECKPOINT_2: 4 field contest statuses source-owned and live; zero-mark cleanup; no generic immunity added")
