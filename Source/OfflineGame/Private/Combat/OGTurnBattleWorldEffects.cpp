#include "Combat/OGTurnBattle.h"

bool FOGTurnBattle::ValidateNativeWorldEffectTiming(const FOGNativeEffectDefinition& D,FString& Error) const
{
    const bool Periodic=D.Mechanic==EOGNativeEffectMechanic::PeriodicDamage || D.Mechanic==EOGNativeEffectMechanic::PeriodicHealing;
    if ((D.Effect.Lifetime.Kind==EOGEffectLifetimeKind::Turns && D.WorldDurationMilliseconds<=0) ||
        (Periodic && D.WorldTickIntervalMilliseconds<=0))
    { Error=TEXT("Missing original World duration/periodic interval for effect: ")+D.Effect.EffectId.ToString(); return false; }
    return true;
}
int64 FOGTurnBattle::GetNextNativeWorldEffectTime() const
{
    int64 Next=MAX_int64;
    for (const auto& Unit:State.Units) for (const auto& I:Unit.NativeEffects)
    {
        if (I.ExpiresAtWorldMilliseconds>=0) Next=FMath::Min(Next,I.ExpiresAtWorldMilliseconds);
        if (I.NextWorldTickMilliseconds>=0 &&
            (I.ExpiresAtWorldMilliseconds<0 || I.NextWorldTickMilliseconds<=I.ExpiresAtWorldMilliseconds))
            Next=FMath::Min(Next,I.NextWorldTickMilliseconds);
    }
    return Next;
}
bool FOGTurnBattle::ProcessNativeWorldEffectClock(int64 At,FString& Error)
{
    // One deterministic event at a time. Resolve its reactions before selecting the next event:
    // a dispel, refresh or death can invalidate another event at the same millisecond.
    FOGEntityId TargetId,InstanceId; bool Tick=false; int64 Best=MAX_int64;
    FString BestKey;
    for (const auto& Unit:State.Units) for (const auto& I:Unit.NativeEffects)
    {
        const bool HasTick=I.NextWorldTickMilliseconds>=0 &&
            (I.ExpiresAtWorldMilliseconds<0 || I.NextWorldTickMilliseconds<=I.ExpiresAtWorldMilliseconds);
        const int64 Due=HasTick?I.NextWorldTickMilliseconds:I.ExpiresAtWorldMilliseconds;
        if (Due<0 || Due>At) continue;
        const FString Key=Unit.UnitEntityId.ToString()+TEXT("|")+I.InstanceId.ToString();
        if (!InstanceId.IsValid() || Due<Best || (Due==Best && Key<BestKey))
        { TargetId=Unit.UnitEntityId; InstanceId=I.InstanceId; Tick=HasTick; Best=Due; BestKey=Key; }
    }
    if (!InstanceId.IsValid()) return true;
    auto* Target=FindMutableUnit(TargetId);
    auto* Stored=Target?Target->NativeEffects.FindByPredicate([&](const auto& I){return I.InstanceId==InstanceId;}):nullptr;
    if (!Stored) { Error=TEXT("World effect clock lost its selected instance."); return false; }
    const auto Instance=*Stored; const auto* D=NativeEffectDefinitions.Find(Instance.EffectId);
    if (!D || !ValidateNativeWorldEffectTiming(*D,Error))
    { if (Error.IsEmpty()) Error=TEXT("World effect lost its admitted mechanic."); return false; }
    if (Tick)
    {
        if (Instance.WorldTickIntervalMilliseconds<=0 ||
            Instance.WorldTickIntervalMilliseconds>MAX_int64-Instance.NextWorldTickMilliseconds)
        { Error=TEXT("World periodic interval is invalid or exhausted."); return false; }
        Stored->NextWorldTickMilliseconds+=Instance.WorldTickIntervalMilliseconds;
        if (Target->IsAlive() && Target->Presence!=EOGCombatPresence::Removed)
        {
            auto Amount=FOGLargeNumber::Multiply(Instance.MagnitudePerStack,FOGLargeNumber::FromInt64(Instance.Stacks));
            if (D->Mechanic==EOGNativeEffectMechanic::PeriodicDamage)
            {
                int32 Rank=10000;
                if (!ResolveRankSuppressionMultiplier(Instance.SourceUnitId,TargetId,OGRankSuppressionChannels::Damage(),Rank,Error)) return false;
                Amount=FOGLargeNumber::ScaleByBasisPoints(Amount,Rank);
                if (!ApplyDamage(Instance.SourceUnitId,TargetId,Amount,Instance.SourceSkillId,Error)) return false;
            }
            else if (D->Mechanic==EOGNativeEffectMechanic::PeriodicHealing)
            { if (!ApplyHealing(Instance.SourceUnitId,TargetId,Amount,Instance.SourceSkillId,Error)) return false; }
            else { Error=TEXT("A nonperiodic mechanic cannot own a World tick."); return false; }
            RecordNativeEffect(EOGCombatLogEventType::EffectTicked,Instance.SourceUnitId,TargetId,
                Instance.SourceSkillId,Instance,TEXT("world_interval"));
        }
    }
    else
    {
        Target->NativeEffects.RemoveAll([&](const auto& I){return I.InstanceId==InstanceId;});
        RecordNativeEffect(EOGCombatLogEventType::EffectRemoved,Instance.SourceUnitId,TargetId,
            Instance.SourceSkillId,Instance,TEXT("world_expired"));
    }
    FlushTriggerRuntimeQueueToBattleQueue();
    return ResolveNativeReactions(Error) && ResolvePendingReplacements(Error);
}
