#include "Effects/OGNativeCombatEffect.h"
#include "Effects/OGEffectValidator.h"

namespace
{
bool SamePriority(const FOGRulePriority& A,const FOGRulePriority& B)
{ return A.Authority==B.Authority && A.Specificity==B.Specificity && A.SourceRank==B.SourceRank && A.SourceRuleId==B.SourceRuleId; }
}
bool FOGNativeEffectDefinition::Validate(FString& Error) const
{
    Error.Reset(); TArray<FString> Errors;
    if (!FOGEffectValidator::Validate(Effect,Errors))
    { Error=FString::Join(Errors,TEXT("; ")); return false; }
    if (!Effect.FamilyId.IsValid() || Effect.PotencyTier<0 || Effect.MaxStacks>10000 ||
        Effect.EffectId.Value.StartsWith(TEXT("diagnostic:")) ||
        OriginalSourceReference.TrimStartAndEnd().IsEmpty() ||
        static_cast<uint8>(Mechanic)>static_cast<uint8>(EOGNativeEffectMechanic::PeriodicHealing) ||
        static_cast<uint8>(Stacking)>static_cast<uint8>(EOGNativeEffectStacking::Independent) ||
        static_cast<uint8>(Clock)>static_cast<uint8>(EOGNativeEffectClock::TargetActions) ||
        (Effect.Lifetime.Kind!=EOGEffectLifetimeKind::Turns && Effect.Lifetime.Kind!=EOGEffectLifetimeKind::Encounter))
    { Error=TEXT("Native battle effects require sourced IDs, a supported mechanic and explicit turn/encounter lifetime."); return false; }
    TSet<EOGNativeEffectStat> Seen;
    for (const auto& M:Modifiers)
    {
        if (static_cast<uint8>(M.Stat)>static_cast<uint8>(EOGNativeEffectStat::TrueDamageResistance) ||
            Seen.Contains(M.Stat) || (M.Flat==0 && M.AdditiveMultiplierBps==0) ||
            M.AdditiveMultiplierBps < -10000 || M.AdditiveMultiplierBps>1000000)
        { Error=TEXT("Native modifiers require distinct stats and explicit bounded flat/scaled contributions."); return false; }
        Seen.Add(M.Stat);
    }
    if (Mechanic==EOGNativeEffectMechanic::StatModifier)
    {
        if (Modifiers.IsEmpty() || !FlatMagnitude.IsZero() || AttackScalingBps || DefenseScalingBps || MaxHpScalingBps)
        { Error=TEXT("A stat modifier requires stat contributions and no periodic/barrier magnitude."); return false; }
    }
    else if (!Modifiers.IsEmpty() || FlatMagnitude.GetSign()<0 ||
        AttackScalingBps<0 || DefenseScalingBps<0 || MaxHpScalingBps<0 ||
        (FlatMagnitude.IsZero() && !AttackScalingBps && !DefenseScalingBps && !MaxHpScalingBps))
    { Error=TEXT("A barrier or periodic effect requires a positive explicit magnitude and no stat modifiers."); return false; }
    return true;
}
bool FOGNativeEffectDefinition::operator==(const FOGNativeEffectDefinition& V) const
{
    return Effect.EffectId==V.Effect.EffectId && Effect.FamilyId==V.Effect.FamilyId &&
        Effect.MechanicKind==V.Effect.MechanicKind && Effect.PotencyTier==V.Effect.PotencyTier &&
        Effect.MaxStacks==V.Effect.MaxStacks && Effect.Lifetime.Kind==V.Effect.Lifetime.Kind &&
        Effect.Lifetime.Magnitude==V.Effect.Lifetime.Magnitude && SamePriority(Effect.RulePriority,V.Effect.RulePriority) &&
        Effect.MetadataTags==V.Effect.MetadataTags && OriginalSourceReference==V.OriginalSourceReference &&
        DisplayName.EqualTo(V.DisplayName) && Mechanic==V.Mechanic && Stacking==V.Stacking && Clock==V.Clock &&
        bPerSource==V.bPerSource && bBeneficial==V.bBeneficial && bDispellable==V.bDispellable &&
        bRemoveOnDefeat==V.bRemoveOnDefeat && bRefillBarrierOnRefresh==V.bRefillBarrierOnRefresh &&
        Modifiers==V.Modifiers && FlatMagnitude==V.FlatMagnitude && AttackScalingBps==V.AttackScalingBps &&
        DefenseScalingBps==V.DefenseScalingBps && MaxHpScalingBps==V.MaxHpScalingBps;
}
bool FOGNativeAbilityEffectOperation::Validate(FString& Error) const
{
    if (!Id.IsValid() || static_cast<uint8>(Operation)>static_cast<uint8>(EOGNativeEffectOperation::DispelFamily) ||
        static_cast<uint8>(DispelSelection)>static_cast<uint8>(EOGNativeDispelSelection::Harmful) ||
        Stacks<1 || Stacks>10000 || ApplicationChanceBps<0 || ApplicationChanceBps>10000 ||
        MaximumRemovals<1 || (Operation!=EOGNativeEffectOperation::Apply && !RemovalPriority.SourceRuleId.IsValid()))
    { Error=TEXT("Effect operations require a real ID, explicit stack/chance/removal bounds and sourced removal priority."); return false; }
    return true;
}
bool FOGNativeAbilityEffectOperation::operator==(const FOGNativeAbilityEffectOperation& V) const
{
    return Operation==V.Operation && Id==V.Id && bToSource==V.bToSource && bBeforePackets==V.bBeforePackets &&
        Stacks==V.Stacks && ApplicationChanceBps==V.ApplicationChanceBps && MaximumRemovals==V.MaximumRemovals &&
        bOnlyOwnSource==V.bOnlyOwnSource && DispelSelection==V.DispelSelection && SamePriority(RemovalPriority,V.RemovalPriority);
}
bool FOGNativeEffectInstance::operator==(const FOGNativeEffectInstance& V) const
{
    return InstanceId==V.InstanceId && EffectId==V.EffectId && SourceUnitId==V.SourceUnitId &&
        SourceSkillId==V.SourceSkillId && ClockUnitId==V.ClockUnitId && Stacks==V.Stacks &&
        RemainingActions==V.RemainingActions && AppliedAtOwnerAction==V.AppliedAtOwnerAction &&
        MagnitudePerStack==V.MagnitudePerStack && BarrierRemaining==V.BarrierRemaining;
}
bool UOGNativeCombatEffect::ValidateDefinition_Implementation(FString& Error) const
{
    return Super::ValidateDefinition_Implementation(Error) && System==TEXT("effect") &&
        DefinitionId==Specification.Effect.EffectId && Specification.Validate(Error);
}
