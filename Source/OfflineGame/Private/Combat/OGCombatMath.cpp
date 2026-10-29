#include "Combat/OGCombatMath.h"
#include <cmath>

namespace
{
int32 ClampToInt32(int64 Value)
{
    return static_cast<int32>(
        FMath::Clamp<int64>(
            Value,
            static_cast<int64>(MIN_int32),
            static_cast<int64>(MAX_int32)));
}

int64 Pow10Int64(int32 Power)
{
    static constexpr int64 Values[] =
    {
        1LL,
        10LL,
        100LL,
        1000LL,
        10000LL,
        100000LL,
        1000000LL,
        10000000LL,
        100000000LL,
        1000000000LL
    };

    return Power >= 0 && Power <= 9 ? Values[Power] : 0;
}

int64 DivideRoundedNonNegative(
    int64 Value,
    int64 Divisor)
{
    if (Divisor <= 1)
    {
        return Value;
    }

    return (Value + Divisor / 2) / Divisor;
}
}

bool FOGCombatMath::RollBasisPoints(
    int32 ChanceBps,
    FOGDeterministicRng& Rng)
{
    if (ChanceBps <= 0)
    {
        return false;
    }

    if (ChanceBps >= 10000)
    {
        return true;
    }

    return Rng.NextRange(0, 10000) < ChanceBps;
}

FOGCritResolution FOGCombatMath::ResolveCrit(
    const FOGCombatStats& Attacker,
    const FOGCombatStats& Defender,
    FOGDeterministicRng& Rng)
{
    FOGCritResolution Result;

    // Settled order: opposing Crit Rate Resistance is deducted first.
    Result.EffectiveCritRateBps =
        ClampToInt32(FMath::Max<int64>(0,
            static_cast<int64>(Attacker.CritRateBps)-Defender.CritRateResistanceBps));

    Result.CritChanceBps =
        FMath::Min(Result.EffectiveCritRateBps, 10000);

    Result.OverflowCritRateBps =
        FMath::Max(
            0,
            Result.EffectiveCritRateBps - 10000);

    // Settled conversion: 1 Crit Rate point -> 2 Crit Damage points.
    Result.OverflowCritDamageBonusBps =
        ClampToInt32(
            static_cast<int64>(Result.OverflowCritRateBps) * 2LL);

    const int64 CritDamageAfterResistance =
        static_cast<int64>(Attacker.CritDamageBonusBps) -
        static_cast<int64>(Defender.CritDamageResistanceBps) +
        static_cast<int64>(Result.OverflowCritDamageBonusBps);

    // Crit Damage Resistance can erase the bonus, but ordinary baseline
    // critical resolution does not turn a crit into a damage penalty.
    Result.EffectiveCritDamageBonusBps =
        ClampToInt32(
            FMath::Max<int64>(0, CritDamageAfterResistance));

    Result.bCritical =
        RollBasisPoints(Result.CritChanceBps, Rng);

    return Result;
}

FOGHitResolution FOGCombatMath::ResolveHit(
    const FOGCombatStats& Attacker,
    const FOGCombatStats& Defender,
    bool bAllowOverflowReplication,
    FOGDeterministicRng& Rng)
{
    FOGHitResolution Result;

    Result.EffectiveHitBps =
        ClampToInt32(FMath::Max<int64>(0,static_cast<int64>(Attacker.HitBps)-Defender.DodgeBps));

    if (!bAllowOverflowReplication)
    {
        const int32 Chance =
            FMath::Min(Result.EffectiveHitBps, 10000);

        Result.GuaranteedHitInstances =
            Chance >= 10000 ? 1 : 0;
        Result.OverflowRemainderBps =
            Chance >= 10000 ? 0 : Chance;
        Result.ResolvedHitInstances =
            RollBasisPoints(Chance, Rng) ? 1 : 0;

        return Result;
    }

    Result.GuaranteedHitInstances =
        Result.EffectiveHitBps / 10000;

    Result.OverflowRemainderBps =
        Result.EffectiveHitBps % 10000;

    Result.ResolvedHitInstances =
        Result.GuaranteedHitInstances;

    if (RollBasisPoints(
            Result.OverflowRemainderBps,
            Rng))
    {
        ++Result.ResolvedHitInstances;
    }

    return Result;
}

FOGBlockResolution FOGCombatMath::ResolveBlock(
    const FOGCombatStats& Defender,
    bool bCanBeBlocked,
    FOGDeterministicRng& Rng)
{
    FOGBlockResolution Result;

    if (!bCanBeBlocked)
    {
        return Result;
    }

    Result.EffectiveBlockRateBps =
        FMath::Clamp(
            Defender.BlockRateBps,
            0,
            10000);

    Result.BlockReductionBps =
        FMath::Clamp(
            Defender.BlockReductionBps,
            0,
            10000);

    Result.bBlocked =
        RollBasisPoints(
            Result.EffectiveBlockRateBps,
            Rng);

    return Result;
}

int32 FOGCombatMath::RatioBps(
    const FOGLargeNumber& Numerator,
    const FOGLargeNumber& OtherPositiveTerm)
{
    if (Numerator.GetSign() <= 0)
    {
        return 0;
    }

    if (OtherPositiveTerm.GetSign() <= 0)
    {
        return 10000;
    }

    FOGLargeNumber A = Numerator;
    FOGLargeNumber B = OtherPositiveTerm;
    A.Normalize();
    B.Normalize();

    const int64 ExponentDelta =
        static_cast<int64>(A.Exponent10) - B.Exponent10;

    // More than 9 decimal orders apart is below our stored significand precision.
    if (ExponentDelta > 9)
    {
        return 10000;
    }

    if (ExponentDelta < -9)
    {
        return 0;
    }

    int64 AlignedA = A.Significand;
    int64 AlignedB = B.Significand;

    if (ExponentDelta > 0)
    {
        AlignedB =
            DivideRoundedNonNegative(
                AlignedB,
                Pow10Int64(static_cast<int32>(ExponentDelta)));
    }
    else if (ExponentDelta < 0)
    {
        AlignedA =
            DivideRoundedNonNegative(
                AlignedA,
                Pow10Int64(static_cast<int32>(-ExponentDelta)));
    }

    const int64 Denominator =
        AlignedA + AlignedB;

    if (Denominator <= 0)
    {
        return 10000;
    }

    const int64 Scaled =
        (AlignedA * 10000LL + Denominator / 2) /
        Denominator;

    return static_cast<int32>(
        FMath::Clamp<int64>(
            Scaled,
            0,
            10000));
}

FOGLargeNumber FOGCombatMath::ResolveDefenseMultiplier(
    const FOGLargeNumber& DefenseReference,const FOGLargeNumber& DefenderDefense,int32 DefensePenetrationBps)
{
    // The caller must supply the skill's positive authored reference; no universal fallback.
    if (DefenseReference.GetSign()<=0) return FOGLargeNumber();
    const int64 Remaining=10000LL-static_cast<int64>(DefensePenetrationBps);
    const auto Effective=FOGLargeNumber::Multiply(DefenderDefense,FOGLargeNumber(Remaining,-4));
    if (Effective.GetSign()==0) return FOGLargeNumber::FromInt64(1);
    if (Effective.GetSign()>0) return FOGLargeNumber(RatioBps(DefenseReference,Effective),-4);

    auto Magnitude=Effective; Magnitude.Significand=-Magnitude.Significand; Magnitude.Normalize();
    auto Reference=DefenseReference; Reference.Normalize();
    const double LogRatio=std::log10(static_cast<double>(Magnitude.Significand)/Reference.Significand)+
        (static_cast<int64>(Magnitude.Exponent10)-Reference.Exponent10);
    constexpr double Log10Two=0.30102999566398119521;
    const double MaximumLog10=static_cast<double>(MAX_int32)+std::log10(static_cast<double>(FOGLargeNumber::MaxSignificand));
    if (LogRatio>=std::log10(MaximumLog10/Log10Two))
        return FOGLargeNumber(FOGLargeNumber::MaxSignificand,MAX_int32);
    const double LogFactor=std::pow(10.0,LogRatio)*Log10Two;
    const double DecimalOrder=std::floor(LogFactor);
    const int64 Mantissa=static_cast<int64>(std::llround(std::pow(10.0,LogFactor-DecimalOrder+8.0)));
    return FOGLargeNumber(Mantissa,static_cast<int32>(DecimalOrder-8.0));
}
int32 FOGCombatMath::ResolveDefenseMultiplierBps(
    const FOGLargeNumber& DefenseReference,const FOGLargeNumber& DefenderDefense,int32 DefensePenetrationBps)
{
    auto Factor=ResolveDefenseMultiplier(DefenseReference,DefenderDefense,DefensePenetrationBps);
    if (Factor.GetSign()<=0) return 0;
    const int64 Order=static_cast<int64>(Factor.Exponent10)+4;
    if (Order>9) return MAX_int32;
    if (Order < -18) return 0;
    const double Value=static_cast<double>(Factor.Significand)*std::pow(10.0,static_cast<double>(Order));
    return Value>=MAX_int32?MAX_int32:static_cast<int32>(std::llround(Value));
}

FOGDamageResolution FOGCombatMath::ResolveDamage(
    const FOGDamageRequest& Request,
    const FOGCombatStats& Attacker,
    const FOGCombatStats& Defender,
    FOGDeterministicRng& Rng)
{
    FOGDamageResolution Result;

    Result.Hit =
        ResolveHit(
            Attacker,
            Defender,
            Request.bAllowHitOverflowReplication,
            Rng);

    if (Result.Hit.ResolvedHitInstances <= 0 ||
        Request.BaseDamage.GetSign() <= 0)
    {
        return Result;
    }

    Result.Crit =
        Request.bCanCrit
            ? ResolveCrit(Attacker, Defender, Rng)
            : FOGCritResolution();

    Result.Block =
        ResolveBlock(
            Defender,
            Request.bCanBeBlocked &&
            Request.DamageType != EOGBaseDamageType::TrueDamage,
            Rng);

    FOGLargeNumber Damage =
        Request.BaseDamage;

    if (Request.DamageType == EOGBaseDamageType::Physical)
    {
        Result.DefenseMultiplier=ResolveDefenseMultiplier(Request.DefenseReference,Defender.Defense,Attacker.DefensePenetrationBps);
        Result.DefenseMultiplierBps =
            ResolveDefenseMultiplierBps(
                Request.DefenseReference,
                Defender.Defense,
                Attacker.DefensePenetrationBps);

        Damage=FOGLargeNumber::Multiply(Damage,Result.DefenseMultiplier);

        Damage =
            FOGLargeNumber::ScaleByBasisPoints(
                Damage,
                Attacker.DamageDealtMultiplierBps);

        Damage =
            FOGLargeNumber::ScaleByBasisPoints(
                Damage,
                Defender.DamageTakenMultiplierBps);

        for (const int32 MultiplierBps : Request.AdditionalMultiplierBps)
        {
            Damage =
                FOGLargeNumber::ScaleByBasisPoints(
                    Damage,
                    MultiplierBps);
        }

        if (Result.Block.bBlocked)
        {
            Damage =
                FOGLargeNumber::ScaleByBasisPoints(
                    Damage,
                    10000 - Result.Block.BlockReductionBps);
        }
    }
    else
    {
        // True Damage ignores DEF, ordinary Damage Reduction and Block.
        Result.DefenseMultiplierBps = 10000;

        Damage =
            FOGLargeNumber::ScaleByBasisPoints(
                Damage,
                Attacker.DamageDealtMultiplierBps);

        for (const int32 MultiplierBps : Request.AdditionalMultiplierBps)
        {
            Damage =
                FOGLargeNumber::ScaleByBasisPoints(
                    Damage,
                    MultiplierBps);
        }

        Damage =
            FOGLargeNumber::ScaleByBasisPoints(
                Damage,
                10000 -
                FMath::Clamp(
                    Defender.TrueDamageResistanceBps,
                    0,
                    10000));
    }

    Result.RankSuppressionMultiplierBps =
        FMath::Clamp(
            Request.RankSuppressionMultiplierBps,
            0,
            10000);

    Damage =
        FOGLargeNumber::ScaleByBasisPoints(
            Damage,
            Result.RankSuppressionMultiplierBps);

    if (Result.Crit.bCritical)
    {
        Damage =
            FOGLargeNumber::ScaleByBasisPoints(
                Damage,
                ClampToInt32(
                    10000LL +
                    static_cast<int64>(
                        Result.Crit.EffectiveCritDamageBonusBps)));
    }

    Result.DamagePerHit = Damage;

    FOGLargeNumber Total;
    for (int32 HitIndex = 0;
         HitIndex < Result.Hit.ResolvedHitInstances;
         ++HitIndex)
    {
        Total =
            FOGLargeNumber::Add(
                Total,
                Damage);
    }

    Result.TotalDamage = Total;
    return Result;
}
