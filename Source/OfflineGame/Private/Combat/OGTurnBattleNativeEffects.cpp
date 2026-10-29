#include "Combat/OGTurnBattle.h"
#include "Random/OGDeterministicRng.h"
#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FOGLargeNumber Times(const FOGLargeNumber& Value,int32 Count)
{ return FOGLargeNumber::Multiply(Value,FOGLargeNumber::FromInt64(Count)); }
FOGLargeNumber Subtract(const FOGLargeNumber& A,const FOGLargeNumber& B)
{ return FOGLargeNumber::Add(A,FOGLargeNumber(-B.Significand,B.Exponent10)); }
int32 ClampedInt(int64 Value)
{ return static_cast<int32>(FMath::Clamp<int64>(Value,0,MAX_int32)); }
FOGEntityId EffectInstanceId(const FOGEntityId& Battle,const FOGEntityId& Action,const FOGEntityId& Target,int32 Index)
{
    const FString Key=Battle.ToString()+TEXT("|effect|")+Action.ToString()+TEXT("|")+Target.ToString()+FString::FromInt(Index);
    FGuid Id; FGuid::ParseExact(FMD5::HashAnsiString(*Key),EGuidFormats::Digits,Id); return FOGEntityId(Id);
}
}
bool FOGTurnBattle::RegisterNativeEffect(const FOGNativeEffectDefinition& Definition,FString& Error)
{
    if (State.Status!=EOGTurnBattleStatus::Running || !Definition.Validate(Error))
    { if (Error.IsEmpty()) Error=TEXT("Effect admission requires a running encounter."); return false; }
    if (const auto* Previous=NativeEffectDefinitions.Find(Definition.Effect.EffectId))
    {
        if (*Previous==Definition) return true;
        Error=TEXT("An admitted shared effect cannot be replaced during an encounter."); return false;
    }
    NativeEffectDefinitions.Add(Definition.Effect.EffectId,Definition); ++NativeAbilityRevision; return true;
}
FOGCombatStats FOGTurnBattle::GetNativeEffectiveStats(const FOGEntityId& UnitId) const
{
    const auto* Unit=FindUnit(UnitId); if (!Unit) return FOGCombatStats();
    auto Result=Unit->Stats;
    struct FContribution { int64 Flat=0,Multiplier=0; };
    TMap<EOGNativeEffectStat,FContribution> Contributions;
    for (const auto& Instance:Unit->NativeEffects)
    {
        const auto* D=NativeEffectDefinitions.Find(Instance.EffectId);
        if (!D || D->Mechanic!=EOGNativeEffectMechanic::StatModifier) continue;
        for (const auto& M:D->Modifiers)
        {
            auto& C=Contributions.FindOrAdd(M.Stat);
            C.Flat+=static_cast<int64>(M.Flat)*Instance.Stacks;
            C.Multiplier+=static_cast<int64>(M.AdditiveMultiplierBps)*Instance.Stacks;
        }
    }
    for (const auto& Pair:Contributions)
    {
        const int32 Multiplier=ClampedInt(10000+Pair.Value.Multiplier);
        const auto Large=[&](const FOGLargeNumber& Base)
        {
            const auto V=FOGLargeNumber::Add(FOGLargeNumber::ScaleByBasisPoints(Base,Multiplier),
                FOGLargeNumber::FromInt64(Pair.Value.Flat));
            return V.GetSign()<0?FOGLargeNumber():V;
        };
        const auto Integer=[&](int32 Base)
        { return ClampedInt(static_cast<int64>(Base)*Multiplier/10000+Pair.Value.Flat); };
        switch (Pair.Key)
        {
        case EOGNativeEffectStat::Attack: Result.Attack=Large(Result.Attack); break;
        case EOGNativeEffectStat::Defense: Result.Defense=Large(Result.Defense); break;
        case EOGNativeEffectStat::Speed: Result.Speed=Integer(Result.Speed); break;
        case EOGNativeEffectStat::CritRate: Result.CritRateBps=Integer(Result.CritRateBps); break;
        case EOGNativeEffectStat::CritDamage: Result.CritDamageBonusBps=Integer(Result.CritDamageBonusBps); break;
        case EOGNativeEffectStat::CritRateResistance: Result.CritRateResistanceBps=Integer(Result.CritRateResistanceBps); break;
        case EOGNativeEffectStat::CritDamageResistance: Result.CritDamageResistanceBps=Integer(Result.CritDamageResistanceBps); break;
        case EOGNativeEffectStat::Hit: Result.HitBps=Integer(Result.HitBps); break;
        case EOGNativeEffectStat::Dodge: Result.DodgeBps=Integer(Result.DodgeBps); break;
        case EOGNativeEffectStat::BlockRate: Result.BlockRateBps=Integer(Result.BlockRateBps); break;
        case EOGNativeEffectStat::BlockReduction: Result.BlockReductionBps=Integer(Result.BlockReductionBps); break;
        case EOGNativeEffectStat::DefensePenetration: Result.DefensePenetrationBps=Integer(Result.DefensePenetrationBps); break;
        case EOGNativeEffectStat::DamageDealt: Result.DamageDealtMultiplierBps=Integer(Result.DamageDealtMultiplierBps); break;
        case EOGNativeEffectStat::DamageTaken: Result.DamageTakenMultiplierBps=Integer(Result.DamageTakenMultiplierBps); break;
        case EOGNativeEffectStat::TrueDamageResistance: Result.TrueDamageResistanceBps=Integer(Result.TrueDamageResistanceBps); break;
        }
    }
    return Result;
}
int64 FOGTurnBattle::NativeEffectActionDelay(const FOGCombatUnitState& Source,int64 Delay) const
{
    const int32 Base=Source.Stats.Speed,Effective=FMath::Max(1,GetNativeEffectiveStats(Source.UnitEntityId).Speed);
    if (Base<=0 || Effective<=0 || Base==Effective) return Delay;
    // Divide first: the continuous timeline remains integral without overflowing Delay * Speed.
    const int64 Whole=Delay/Effective;
    const int64 Remainder=(Delay%Effective)*static_cast<int64>(Base)/Effective;
    if (Whole>(MAX_int64-Remainder)/Base) return MAX_int64;
    return FMath::Max<int64>(1,Whole*Base+Remainder);
}
bool FOGTurnBattle::HasNativeEffect(const FOGEntityId& UnitId,const FOGContentId& Id,bool Family) const
{
    const auto* Unit=FindUnit(UnitId); if (!Unit) return false;
    return Unit->NativeEffects.ContainsByPredicate([&](const FOGNativeEffectInstance& I)
    {
        const auto* D=NativeEffectDefinitions.Find(I.EffectId);
        return D && (Family?D->Effect.FamilyId==Id:I.EffectId==Id);
    });
}
FString FOGTurnBattle::DescribeNativeEffects(const FOGEntityId& UnitId) const
{
    const auto* Unit=FindUnit(UnitId); if (!Unit) return FString();
    TArray<FString> Labels;
    for (const auto& I:Unit->NativeEffects)
    {
        const auto* D=NativeEffectDefinitions.Find(I.EffectId); if (!D) continue;
        FString Label=D->DisplayName.IsEmpty()?I.EffectId.ToString():D->DisplayName.ToString();
        Label+=FString::Printf(TEXT(" x%d"),I.Stacks);
        if (I.RemainingActions>=0) Label+=FString::Printf(TEXT(" (%lld actions)"),static_cast<long long>(I.RemainingActions));
        if (D->Mechanic==EOGNativeEffectMechanic::Barrier) Label+=TEXT(" [")+I.BarrierRemaining.ToDebugString()+TEXT("]");
        Labels.Add(MoveTemp(Label));
    }
    return FString::Join(Labels,TEXT(" / "));
}
void FOGTurnBattle::RecordNativeEffect(EOGCombatLogEventType Type,const FOGEntityId& Source,
    const FOGEntityId& Target,const FOGContentId& Skill,const FOGNativeEffectInstance& Instance,const TCHAR* Reason)
{
    auto Json=MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("effect_id"),Instance.EffectId.ToString());
    Json->SetStringField(TEXT("instance_id"),Instance.InstanceId.IsValid()?Instance.InstanceId.ToString():TEXT(""));
    Json->SetStringField(TEXT("reason"),Reason);
    Json->SetNumberField(TEXT("stacks"),Instance.Stacks);
    Json->SetStringField(TEXT("remaining_actions_exact"),LexToString(Instance.RemainingActions));
    Json->SetStringField(TEXT("barrier_remaining"),Instance.BarrierRemaining.ToDebugString());
    FOGCombatLogEvent Event; Event.Type=Type; Event.SourceUnitId=Source; Event.TargetUnitId=Target; Event.SkillId=Skill;
    Event.PayloadJson.Reset();
    FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Event.PayloadJson));
    const int64 Sequence=Log.GetNextSequence();
    Log.Append(MoveTemp(Event)); ++NativeAbilityRevision;
    const FName Trigger=Type==EOGCombatLogEventType::EffectApplied?FName(TEXT("OnEffectApplied")):
        Type==EOGCombatLogEventType::EffectRemoved?FName(TEXT("OnEffectRemoved")):
        Type==EOGCombatLogEventType::EffectTicked?FName(TEXT("OnEffectTicked")):FName(TEXT("OnEffectResisted"));
    if (State.Status==EOGTurnBattleStatus::Running) QueueTriggerEvent(Trigger,Source,Target,Skill,Sequence);
}
bool FOGTurnBattle::ApplyNativeEffect(const FOGNativeEffectDefinition& D,const FOGNativeAbilityEffectOperation& Op,
    const FOGResolvedCombatAction& Action,const FOGEntityId& TargetId,int32 OperationIndex,FString& Error)
{
    auto* Target=FindMutableUnit(TargetId);
    if (!Target || !Target->IsAlive() || Target->Presence==EOGCombatPresence::Removed) return true;
    if (!Action.ActionId.IsValid()) { Error=TEXT("Effect application requires the actual accepted action ID."); return false; }
    auto Same=[&](const FOGNativeEffectInstance& I)
    { return I.EffectId==D.Effect.EffectId && (!D.bPerSource || I.SourceUnitId==Action.SourceUnitId); };
    FOGNativeEffectInstance* Existing=D.Stacking==EOGNativeEffectStacking::Independent?nullptr:Target->NativeEffects.FindByPredicate(Same);
    const bool New=Existing==nullptr;
    FOGNativeEffectInstance I=Existing?*Existing:FOGNativeEffectInstance();
    I.InstanceId=Existing?Existing->InstanceId:EffectInstanceId(State.BattleId,Action.ActionId,TargetId,OperationIndex);
    I.EffectId=D.Effect.EffectId; I.SourceUnitId=Action.SourceUnitId; I.SourceSkillId=Action.SkillId;
    I.ClockUnitId=D.Clock==EOGNativeEffectClock::SourceActions?Action.SourceUnitId:TargetId;
    I.AppliedAtOwnerAction=NativeOwnerActions.FindRef(I.ClockUnitId);
    if (Action.InterruptPriority==0 && I.ClockUnitId==Action.SourceUnitId)
    {
        if (I.AppliedAtOwnerAction==MAX_int64) { Error=TEXT("Effect application clock exhausted."); return false; }
        ++I.AppliedAtOwnerAction; // The applying ordinary action never consumes its own new duration.
    }
    I.RemainingActions=D.Effect.Lifetime.Kind==EOGEffectLifetimeKind::Turns?D.Effect.Lifetime.Magnitude:-1;
    if (D.Stacking==EOGNativeEffectStacking::Independent)
    {
        int32 Occupied=0; for (const auto& Other:Target->NativeEffects) if (Same(Other)) Occupied+=Other.Stacks;
        I.Stacks=FMath::Min(Op.Stacks,D.Effect.MaxStacks-Occupied); if (I.Stacks<=0) return true;
    }
    else I.Stacks=FMath::Min(D.Effect.MaxStacks,Op.Stacks+
        ((!New && D.Stacking==EOGNativeEffectStacking::AddAndRefresh)?I.Stacks:0));
    const auto Stats=GetNativeEffectiveStats(Action.SourceUnitId);
    I.MagnitudePerStack=D.FlatMagnitude;
    I.MagnitudePerStack=FOGLargeNumber::Add(I.MagnitudePerStack,FOGLargeNumber::ScaleByBasisPoints(Stats.Attack,D.AttackScalingBps));
    I.MagnitudePerStack=FOGLargeNumber::Add(I.MagnitudePerStack,FOGLargeNumber::ScaleByBasisPoints(Stats.Defense,D.DefenseScalingBps));
    I.MagnitudePerStack=FOGLargeNumber::Add(I.MagnitudePerStack,FOGLargeNumber::ScaleByBasisPoints(Stats.MaxHp,D.MaxHpScalingBps));
    if (D.Mechanic==EOGNativeEffectMechanic::Barrier)
    {
        if (New || D.bRefillBarrierOnRefresh) I.BarrierRemaining=Times(I.MagnitudePerStack,I.Stacks);
        else if (D.Stacking==EOGNativeEffectStacking::AddAndRefresh)
        {
            const auto Added=Times(I.MagnitudePerStack,FMath::Max(0,I.Stacks-Existing->Stacks));
            I.BarrierRemaining=FOGLargeNumber::Add(I.BarrierRemaining,Added);
        }
        const auto Cap=Times(I.MagnitudePerStack,I.Stacks);
        if (FOGLargeNumber::Compare(I.BarrierRemaining,Cap)>0) I.BarrierRemaining=Cap;
    }
    if (New)
    {
        if (Target->NativeEffects.Num()>=4096) { Error=TEXT("Native effect instance capacity exceeded; action rolled back."); return false; }
        Target->NativeEffects.Add(I);
    }
    else *Existing=I;
    RecordNativeEffect(EOGCombatLogEventType::EffectApplied,Action.SourceUnitId,TargetId,Action.SkillId,I,New?TEXT("applied"):TEXT("refreshed"));
    return true;
}
bool FOGTurnBattle::ExecuteNativeEffects(const FOGResolvedCombatAction& Action,bool BeforePackets,FOGDeterministicRng& Rng,FString& Error,uint8 Scope)
{
    const auto* Source=FindUnit(Action.SourceUnitId);
    const auto* Ability=Source?FindNativeAbility(*Source,Action.SkillId):nullptr;
    if (!Ability) return true;
    for (int32 Index=0;Index<Ability->Effects.Num();++Index)
    {
        const auto& Op=Ability->Effects[Index]; if (Op.bBeforePackets!=BeforePackets ||
            (Scope==1 && !Op.bToSource) || (Scope==2 && Op.bToSource)) continue;
        const TArray<FOGEntityId> Targets=Op.bToSource?TArray<FOGEntityId>{Action.SourceUnitId}:Action.TargetUnitIds;
        for (const auto& TargetId:Targets)
        {
            auto* Target=FindMutableUnit(TargetId); if (!Target) { Error=TEXT("Effect target disappeared."); return false; }
            if (Op.Operation==EOGNativeEffectOperation::Apply)
            {
                const auto* D=NativeEffectDefinitions.Find(Op.Id);
                if (!D) { Error=TEXT("Effect has no admitted executor: ")+Op.Id.ToString(); return false; }
                if (!Target->IsAlive() || Target->Presence==EOGCombatPresence::Removed) continue;
                int32 Penetration=10000;
                if (!D->bBeneficial && Action.SourceUnitId!=TargetId &&
                    !ResolveRankSuppressionMultiplier(Action.SourceUnitId,TargetId,OGRankSuppressionChannels::EffectPenetration(),Penetration,Error)) return false;
                if (!FOGCombatMath::RollBasisPoints(static_cast<int32>(static_cast<int64>(Op.ApplicationChanceBps)*Penetration/10000),Rng))
                {
                    FOGNativeEffectInstance I; I.EffectId=Op.Id;
                    RecordNativeEffect(EOGCombatLogEventType::EffectResisted,Action.SourceUnitId,TargetId,Action.SkillId,I,TEXT("application_resisted"));
                    continue;
                }
                if (!ApplyNativeEffect(*D,Op,Action,TargetId,Index,Error)) return false;
            }
            else
            {
                TArray<FOGEntityId> Eligible;
                for (const auto& I:Target->NativeEffects)
                {
                    const auto* D=NativeEffectDefinitions.Find(I.EffectId);
                    if (!D || !D->bDispellable || (Op.bOnlyOwnSource && I.SourceUnitId!=Action.SourceUnitId) ||
                        (Op.Operation==EOGNativeEffectOperation::DispelEffect?I.EffectId!=Op.Id:D->Effect.FamilyId!=Op.Id) ||
                        (Op.DispelSelection==EOGNativeDispelSelection::Beneficial && !D->bBeneficial) ||
                        (Op.DispelSelection==EOGNativeDispelSelection::Harmful && D->bBeneficial) ||
                        FOGRulePriority::Compare(Op.RemovalPriority,D->Effect.RulePriority)<0) continue;
                    Eligible.Add(I.InstanceId);
                }
                Eligible.Sort([](const auto& A,const auto& B){return A.ToString()<B.ToString();});
                for (int32 R=0;R<FMath::Min(Op.MaximumRemovals,Eligible.Num());++R)
                {
                    const int32 At=Target->NativeEffects.IndexOfByPredicate([&](const auto& I){return I.InstanceId==Eligible[R];});
                    if (At==INDEX_NONE) continue;
                    const auto Removed=Target->NativeEffects[At]; Target->NativeEffects.RemoveAt(At);
                    RecordNativeEffect(EOGCombatLogEventType::EffectRemoved,Action.SourceUnitId,TargetId,Action.SkillId,Removed,TEXT("dispelled"));
                }
            }
        }
    }
    return true;
}
FOGLargeNumber FOGTurnBattle::AbsorbNativeBarriers(const FOGEntityId& TargetId,FOGLargeNumber& Amount)
{
    auto* Target=FindMutableUnit(TargetId); if (!Target) return FOGLargeNumber();
    TArray<int32> Layers;
    for (int32 I=0;I<Target->NativeEffects.Num();++I)
    {
        const auto* D=NativeEffectDefinitions.Find(Target->NativeEffects[I].EffectId);
        if (D && D->Mechanic==EOGNativeEffectMechanic::Barrier && Target->NativeEffects[I].BarrierRemaining.GetSign()>0) Layers.Add(I);
    }
    Layers.Sort([&](int32 A,int32 B)
    {
        const auto& Left=Target->NativeEffects[A]; const auto& Right=Target->NativeEffects[B];
        const int32 Priority=FOGRulePriority::Compare(NativeEffectDefinitions.FindChecked(Left.EffectId).Effect.RulePriority,
            NativeEffectDefinitions.FindChecked(Right.EffectId).Effect.RulePriority);
        return Priority?Priority>0:Left.InstanceId.ToString()<Right.InstanceId.ToString();
    });
    FOGLargeNumber Absorbed;
    for (const int32 Index:Layers)
    {
        if (Amount.IsZero()) break;
        auto& I=Target->NativeEffects[Index];
        const auto Used=FOGLargeNumber::Compare(Amount,I.BarrierRemaining)<0?Amount:I.BarrierRemaining;
        I.BarrierRemaining=Subtract(I.BarrierRemaining,Used); Amount=Subtract(Amount,Used);
        Absorbed=FOGLargeNumber::Add(Absorbed,Used);
    }
    for (int32 At=Target->NativeEffects.Num()-1;At>=0;--At)
    {
        const auto I=Target->NativeEffects[At]; const auto* D=NativeEffectDefinitions.Find(I.EffectId);
        if (D && D->Mechanic==EOGNativeEffectMechanic::Barrier && I.BarrierRemaining.IsZero())
        {
            Target->NativeEffects.RemoveAt(At);
            RecordNativeEffect(EOGCombatLogEventType::EffectRemoved,I.SourceUnitId,TargetId,I.SourceSkillId,I,TEXT("depleted"));
        }
    }
    return Absorbed;
}
bool FOGTurnBattle::ProcessNativeEffectOwnerAction(const FOGEntityId& Owner,FString& Error)
{
    struct FDue { FOGEntityId Target,Instance; };
    TArray<FDue> Due;
    for (const auto& Target:State.Units) for (const auto& I:Target.NativeEffects)
        if (I.ClockUnitId==Owner && NativeOwnerActions.FindRef(Owner)>I.AppliedAtOwnerAction)
        { FDue Entry; Entry.Target=Target.UnitEntityId; Entry.Instance=I.InstanceId; Due.Add(Entry); }
    Due.Sort([](const FDue& A,const FDue& B)
    { return A.Target==B.Target?A.Instance.ToString()<B.Instance.ToString():A.Target.ToString()<B.Target.ToString(); });
    for (const auto& Entry:Due)
    {
        auto* Target=FindMutableUnit(Entry.Target); if (!Target) continue;
        auto* Stored=Target->NativeEffects.FindByPredicate([&](const auto& I){return I.InstanceId==Entry.Instance;});
        if (!Stored) continue;
        const auto Instance=*Stored; const auto* D=NativeEffectDefinitions.Find(Instance.EffectId);
        if (!D) { Error=TEXT("An active effect lost its admitted definition."); return false; }
        if (Target->IsAlive() && Target->Presence!=EOGCombatPresence::Removed &&
            (D->Mechanic==EOGNativeEffectMechanic::PeriodicDamage || D->Mechanic==EOGNativeEffectMechanic::PeriodicHealing))
        {
            auto Amount=Times(Instance.MagnitudePerStack,Instance.Stacks);
            if (D->Mechanic==EOGNativeEffectMechanic::PeriodicDamage)
            {
                int32 Rank=10000;
                if (!ResolveRankSuppressionMultiplier(Instance.SourceUnitId,Entry.Target,OGRankSuppressionChannels::Damage(),Rank,Error)) return false;
                Amount=FOGLargeNumber::ScaleByBasisPoints(Amount,Rank);
                if (!ApplyDamage(Instance.SourceUnitId,Entry.Target,Amount,Instance.SourceSkillId,Error)) return false;
            }
            else if (!ApplyHealing(Instance.SourceUnitId,Entry.Target,Amount,Instance.SourceSkillId,Error)) return false;
            RecordNativeEffect(EOGCombatLogEventType::EffectTicked,Instance.SourceUnitId,Entry.Target,Instance.SourceSkillId,Instance,TEXT("owner_action_end"));
        }
        // Defeat/revival may have removed this exact instance. Never retain an invalid array pointer.
        Target=FindMutableUnit(Entry.Target);
        const int32 At=Target?Target->NativeEffects.IndexOfByPredicate([&](const auto& I){return I.InstanceId==Entry.Instance;}):INDEX_NONE;
        if (At==INDEX_NONE) continue;
        auto& I=Target->NativeEffects[At];
        if (I.RemainingActions>0 && --I.RemainingActions==0)
        {
            const auto Expired=I; Target->NativeEffects.RemoveAt(At);
            RecordNativeEffect(EOGCombatLogEventType::EffectRemoved,Expired.SourceUnitId,Entry.Target,Expired.SourceSkillId,Expired,TEXT("expired"));
        }
    }
    return true;
}
void FOGTurnBattle::RemoveNativeEffectsOnDefeat(const FOGEntityId& TargetId)
{
    auto* Target=FindMutableUnit(TargetId); if (!Target) return;
    for (int32 Index=Target->NativeEffects.Num()-1;Index>=0;--Index)
    {
        const auto I=Target->NativeEffects[Index]; const auto* D=NativeEffectDefinitions.Find(I.EffectId);
        if (!D || !D->bRemoveOnDefeat) continue;
        Target->NativeEffects.RemoveAt(Index);
        RecordNativeEffect(EOGCombatLogEventType::EffectRemoved,I.SourceUnitId,TargetId,I.SourceSkillId,I,TEXT("defeat"));
    }
}
void FOGTurnBattle::ClearNativeEncounterEffects()
{
    for (auto& Unit:State.Units)
    {
        const auto Previous=MoveTemp(Unit.NativeEffects); Unit.NativeEffects.Reset();
        for (const auto& I:Previous)
            RecordNativeEffect(EOGCombatLogEventType::EffectRemoved,I.SourceUnitId,Unit.UnitEntityId,I.SourceSkillId,I,TEXT("encounter_ended"));
    }
}
