from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=root/"Public/Combat/OGTurnBattle.h"
c=root/"Private/Combat/OGTurnBattleLuminousBaseV6.cpp"
battle=root/"Private/Combat/OGTurnBattle.cpp"
def edit(path, patches):
 s=path.read_text(encoding="utf-8-sig")
 for old,new in patches:
  n=s.count(old)
  if n!=1:raise RuntimeError(f"{path.name}: expected 1 found {n} {old[:100]!r}")
  s=s.replace(old,new,1)
 path.write_text(s,encoding="utf-8",newline="")
 print("MODIFIED",path.name)
edit(h,[
("""    void TryResolveApprovedLuminousBaseRevival(
        const FOGEntityId& UnitId);""",
 """    void TryResolveApprovedLuminousBaseRevival(
        const FOGEntityId& UnitId);
    void TryResolveApprovedLuminousAllyCovenant(
        const FOGEntityId& TargetUnitId);""")
])
edit(battle,[
("""        TryResolveApprovedLuminousBaseRevival(TargetUnitId);
    }

    return true;""",
 """        TryResolveApprovedLuminousBaseRevival(TargetUnitId);
        TryResolveApprovedLuminousAllyCovenant(TargetUnitId);
    }

    return true;""")
])
edit(c,[
("""    const int32 MaxTargets=bA1?1:bA2?3:bA3?3:3;""",
 """    FString A3Mode;
    if(bA3)Context->TryGetStringField(TEXT("mode"),A3Mode);
    const bool bA3Covenant=bA3 && A3Mode==TEXT("covenant");
    const int32 MaxTargets=bA1?1:bA2?3:bA3Covenant?1:3;"""),
("""    if(bUlt)
    {
        if (!Context->TryGetStringField(TEXT("branch"),UltimateBranch)""",
 """    bool bPreparedSoulVerified=false,bPreparedNegationClear=false;
    if(bA3Covenant)
    {
        if(!Context->TryGetBoolField(TEXT("verified_target_soul_continuity"),
                                     bPreparedSoulVerified) ||
           !bPreparedSoulVerified ||
           !Context->TryGetBoolField(TEXT("verified_no_superior_revival_negation"),
                                     bPreparedNegationClear) ||
           !bPreparedNegationClear)
        {
            OutError=TEXT("A3 Covenant requires explicit lawful soul and higher-order Authority verification.");
            return false;
        }
    }
    if(bUlt)
    {
        if (!Context->TryGetStringField(TEXT("branch"),UltimateBranch)"""),
("""    if(ChargeCost>StateOfOwner.UltimateChargePercent)
    {
        OutError=TEXT("Manifestation lacks its own required ultimate charge.");
        return false;
    }
    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;""",
 """    if(ChargeCost>StateOfOwner.UltimateChargePercent)
    {
        OutError=TEXT("Manifestation lacks its own required ultimate charge.");
        return false;
    }
    if(bA3Covenant && StateOfOwner.ScarletReserve<75)
    {
        OutError=TEXT("A3 Covenant requires 75 actual Scarlet Reserve at preparation.");
        return false;
    }
    if(bA3Covenant)
    {
        // Both portions, 30 + 45, are committed at preparation (not death).
        StateOfOwner.ScarletReserve-=75;
        StateOfOwner.PreparedCovenantTarget=Action.TargetUnitIds[0];
        StateOfOwner.CovenantExpiryOwnerAction=StateOfOwner.OwnerActions+3;
        StateOfOwner.bCovenantSoulVerified=bPreparedSoulVerified;
        StateOfOwner.bCovenantNegationClear=bPreparedNegationClear;
        StateOfOwner.bDayAttunement=true;
    }
    if(bUlt)StateOfOwner.UltimateChargePercent-=ChargeCost;"""),
("""    else if(bA3 || (bUlt && UltimateBranch==TEXT("day")))
    {
        const bool bDayUltimate=bUlt;""",
 """    else if((bA3 && !bA3Covenant) ||
            (bUlt && UltimateBranch==TEXT("day")))
    {
        const bool bDayUltimate=bUlt;"""),
])
append = '''

void FOGTurnBattle::TryResolveApprovedLuminousAllyCovenant(
    const FOGEntityId& TargetUnitId)
{
    FOGCombatUnitState* Target=FindMutableUnit(TargetUnitId);
    if(!Target || Target->Presence!=EOGCombatPresence::Defeated ||
       Target->OccupiedLaneIndex==INDEX_NONE)return;
    // Multiple eligible Manifestations resolve in stable GUID order.
    TArray<FOGEntityId> Owners;
    LuminousBaseStates.GetKeys(Owners);
    Owners.Sort([](const FOGEntityId& A,const FOGEntityId& B)
    {
        return A.ToString()<B.ToString();
    });
    for(const FOGEntityId& OwnerId:Owners)
    {
        FLuminousBaseBattleRuntime* S=LuminousBaseStates.Find(OwnerId);
        const FOGCombatUnitState* Owner=FindUnit(OwnerId);
        if(!S || !Owner || !Owner->IsAlive() || !IsBaseLuminous(*Owner) ||
           S->bAllyRevivalSpent || !S->bCovenantSoulVerified ||
           !S->bCovenantNegationClear ||
           S->PreparedCovenantTarget!=TargetUnitId ||
           S->CovenantExpiryOwnerAction<S->OwnerActions ||
           Owner->TeamIndex!=Target->TeamIndex)continue;
        const FOGLargeNumber Restore=FOGLargeNumber::ScaleByBasisPoints(
            Target->Stats.MaxHp,1800);
        if(Restore.IsZero())return;
        FString Error;
        if(!ApplyHealing(OwnerId,TargetUnitId,Restore,
                FOGContentId(TEXT("combat:luminous_valentine.base.skill.crimson_benediction")),
                Error))return;
        if(!ChangePresence(TargetUnitId,EOGCombatPresence::Active,Error))
            return;
        S->bAllyRevivalSpent=true;
        S->PreparedCovenantTarget=FOGEntityId();
        S->CovenantExpiryOwnerAction=-1;
        S->BlessingExpiry.Add(TargetUnitId,S->OwnerActions+2);
        return;
    }
}
'''
with c.open("a",encoding="utf-8",newline="") as stream:stream.write(append)
print("ADDED_PREPARED_COVENANT_REVIVAL_AND_POST_DEFEAT_DISPATCH")
