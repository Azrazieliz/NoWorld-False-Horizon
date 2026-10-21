#include "Combat/OGTurnBattle.h"

// Source-owned, manifestation-scoped effects from approved Luminous Base v6.
// A3 Benediction / prepared Covenant: +22% ATK, +12% speed, +20% healing
// received. Day Ultimate: +25% ATK, -14% damage taken, +24% healing received.
// All 2 owner turns; independent approved effects may coexist but multiple
// Luminous Manifestations never multiply the same bonus without a higher
// Authority rule contest.
void FOGTurnBattle::GetApprovedDawnBlessingBps(
    const FOGEntityId& BeneficiaryId,
    int32& OutAttackBonusBps,
    int32& OutSpeedBonusBps,
    int32& OutHealingReceivedBonusBps,
    int32& OutDamageReductionBps) const
{
    OutAttackBonusBps=0;
    OutSpeedBonusBps=0;
    OutHealingReceivedBonusBps=0;
    OutDamageReductionBps=0;
    for(const auto& Row:LuminousBaseStates)
    {
        const FLuminousBaseBattleRuntime& Owner=Row.Value;
        const int32* Expiry=Owner.BlessingExpiry.Find(BeneficiaryId);
        const uint8* Kind=Owner.BlessingCategory.Find(BeneficiaryId);
        if(!Expiry || !Kind || Owner.OwnerActions>*Expiry)
            continue;
        const FOGCombatUnitState* OwnerUnit=FindUnit(Row.Key);
        if(!OwnerUnit || !OwnerUnit->IsAlive())
            continue;
        if(*Kind==1)
        {
            OutAttackBonusBps=FMath::Max(OutAttackBonusBps,2200);
            OutSpeedBonusBps=FMath::Max(OutSpeedBonusBps,1200);
            OutHealingReceivedBonusBps=FMath::Max(
                OutHealingReceivedBonusBps,2000);
        }
        else if(*Kind==2)
        {
            OutAttackBonusBps=FMath::Max(OutAttackBonusBps,2500);
            OutDamageReductionBps=FMath::Max(OutDamageReductionBps,1400);
            OutHealingReceivedBonusBps=FMath::Max(
                OutHealingReceivedBonusBps,2400);
        }
    }
}
