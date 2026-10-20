#include "Combat/OGTurnBattle.h"

bool FOGTurnBattle::SetResolvedLuminousUltimateCharge(
    const FOGEntityId& ManifestationId,int32 ChargePercent,
    FString& OutError)
{
    OutError.Reset();
    if(State.Status!=EOGTurnBattleStatus::Running ||
       !ManifestationId.IsValid() ||
       ChargePercent<0 || ChargePercent>200)
    {
        OutError=TEXT("Invalid battle, Manifestation or per-character ultimate charge.");
        return false;
    }
    const FOGCombatUnitState* Unit=FindUnit(ManifestationId);
    if(!Unit ||
       Unit->IdentityId!=FOGContentId(TEXT("character:luminous_valentine.identity")))
    {
        OutError=TEXT("Ultimate charge owner is not an acquired Luminous combat unit.");
        return false;
    }
    if(Unit->SkillSet.ActiveSkills.Contains(
        FOGContentId(TEXT("combat:luminous_valentine.base.skill.scarlet_cadence"))))
    {
        LuminousBaseStates.FindOrAdd(ManifestationId).UltimateChargePercent=
            ChargePercent;
        return true;
    }
    if(Unit->SkillSet.ActiveSkills.Contains(
        FOGContentId(TEXT("draft:luminous_valentine.alter.active_1"))))
    {
        LuminousAlterStates.FindOrAdd(ManifestationId).UltimateChargePercent=
            ChargePercent;
        return true;
    }
    OutError=TEXT("No exact approved Luminous version skill set for the charge owner.");
    return false;
}

int32 FOGTurnBattle::GetResolvedLuminousUltimateCharge(
    const FOGEntityId& ManifestationId) const
{
    if(const FLuminousBaseBattleRuntime* S=
           LuminousBaseStates.Find(ManifestationId))
        return S->UltimateChargePercent;
    if(const FLuminousAlterBattleRuntime* S=
           LuminousAlterStates.Find(ManifestationId))
        return S->UltimateChargePercent;
    return 0;
}
