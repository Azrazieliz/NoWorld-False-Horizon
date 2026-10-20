#include "Combat/OGLuminousAlterV1Projection.h"

FOGLuminousAlterV1Projection
FOGLuminousAlterV1Projection::FromApprovedDesign()
{
    // Source: user-approved Gate04 2026-10-19 Alter v1 archive,
    // SHA-256 2c6474548541df55b036eaa5a605805f64bba6fe212d4c91c4013c7fdd1a254c.
    // These are the design-stage IDs, not fictional installed Registry IDs.
    FOGLuminousAlterV1Projection P;
    P.DraftVersionId=FOGContentId(TEXT("draft:character.luminous_valentine.alter"));
    P.HeroLevelOne.MaxHp=FOGLargeNumber::FromInt64(7650);
    P.HeroLevelOne.Attack=FOGLargeNumber::FromInt64(4830);
    P.HeroLevelOne.Defense=FOGLargeNumber::FromInt64(1040);
    P.HeroLevelOne.Speed=151;
    P.HeroLevelOne.CritRateBps=3700;
    P.HeroLevelOne.CritDamageBonusBps=13200;
    P.HeroLevelOne.CritRateResistanceBps=600;
    P.HeroLevelOne.CritDamageResistanceBps=700;
    P.HeroLevelOne.HitBps=12400;
    P.HeroLevelOne.DodgeBps=1900;
    P.HeroLevelOne.BlockRateBps=300;
    P.HeroLevelOne.BlockReductionBps=1400;
    P.HeroLevelOne.DefensePenetrationBps=1700;
    P.HeroLevelOne.DamageDealtMultiplierBps=10000;
    P.HeroLevelOne.DamageTakenMultiplierBps=10000;
    P.HeroLevelOne.TrueDamageResistanceBps=0;
    for(int32 N=1;N<=3;++N)
        P.Skills.ActiveSkills.Add(FOGContentId(
            FString::Printf(TEXT("draft:luminous_valentine.alter.active_%d"),N)));
    for(int32 N=1;N<=6;++N)
        P.Skills.PassiveSkills.Add(FOGContentId(
            FString::Printf(TEXT("draft:luminous_valentine.alter.passive_%d"),N)));
    P.Skills.UltimateSkill=FOGContentId(
        TEXT("draft:luminous_valentine.alter.ultimate.black_mass"));
    P.Skills.CurrentForm=P.DraftVersionId;
    return P;
}
