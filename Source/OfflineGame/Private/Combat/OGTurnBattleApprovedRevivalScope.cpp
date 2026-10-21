#include "Combat/OGTurnBattle.h"

// Authorized Base/Alter resurrection handlers enter this scope only AFTER
// the native Soul/Authority gate, required resources, one-use ledger and
// occupied-lane proof have succeeded. General healing cannot revive.
bool FOGTurnBattle::ApplyApprovedLuminousRevivalHealing(
    const FOGEntityId& SourceId,
    const FOGEntityId& DefeatedTargetId,
    const FOGLargeNumber& Amount,
    const FOGContentId& ResurrectionSkill,
    FString& OutError)
{
    OutError.Reset();
    const FOGCombatUnitState* Target=FindUnit(DefeatedTargetId);
    if(!Target ||
       Target->Presence!=EOGCombatPresence::Defeated ||
       Target->OccupiedLaneIndex==INDEX_NONE ||
       bApprovedRevivalHealInProgress)
    {
        OutError=TEXT("Revival healing requires one valid defeated occupant and an unoccupied approved restoration scope.");
        return false;
    }
    bApprovedRevivalHealInProgress=true;
    const bool bRestored=ApplyHealing(
        SourceId,DefeatedTargetId,Amount,ResurrectionSkill,OutError);
    bApprovedRevivalHealInProgress=false;
    return bRestored;
}
