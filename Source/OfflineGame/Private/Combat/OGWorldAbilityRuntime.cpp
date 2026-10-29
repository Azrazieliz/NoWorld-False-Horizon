#include "UObject/Class.h"
#include "Combat/OGTurnBattle.h"
#include "Random/OGDeterministicRng.h"
#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString WorldKey(const FOGEntityId& Unit,const FOGContentId& Skill)
{ return Unit.ToString()+TEXT("|")+Skill.ToString(); }
uint64 WorldSeed(const FOGEntityId& Battle,const FOGEntityId& Unit,const FOGContentId& Skill,int64 Sequence)
{
    const FString Text=Battle.ToString()+Unit.ToString()+Skill.ToString()+LexToString(Sequence);
    FTCHARToUTF8 Bytes(*Text); uint64 Hash=14695981039346656037ull;
    for (int32 I=0;I<Bytes.Length();++I) { Hash^=static_cast<uint8>(Bytes.Get()[I]); Hash*=1099511628211ull; }
    return Hash;
}
}
const FOGWorldAbilitySpecification* FOGTurnBattle::FindWorldAbility(const FOGCombatUnitState& Unit,const FOGContentId& Skill) const
{ const auto* Found=WorldAbilities.Find(Skill); return Found && Found->Matches(Unit)?Found:nullptr; }
bool FOGTurnBattle::RegisterWorldAbility(const FOGWorldAbilitySpecification& Spec,FString& Error)
{
    if (State.Status!=EOGTurnBattleStatus::Running || !Spec.Validate(Error)) return false;
    bool Used=false;
    for (const auto& Unit:State.Units) if (Spec.Matches(Unit) && CanAdmitNativeAbility(Unit,Spec.SkillId))
    {
        const auto* Ability=FindNativeAbility(Unit,Spec.SkillId);
        if (!Ability)
        { Error=TEXT("This specialized kit requires its exact World executor; shared packet staging cannot replace it."); return false; }
        if (!Spec.ValidatePackets(*Ability,Error)) return false;
        Used=true;
    }
    if (!Used) { Error=TEXT("World timing has no matching actual ability owner."); return false; }
    if (const auto* Previous=WorldAbilities.Find(Spec.SkillId))
    {
        if (FOGWorldAbilitySpecification::StaticStruct()->CompareScriptStruct(Previous,&Spec,0)) return true;
        Error=TEXT("An admitted World ability cannot change during an encounter."); return false;
    }
    WorldAbilities.Add(Spec.SkillId,Spec); ++NativeAbilityRevision; return true;
}
int64 FOGTurnBattle::GetWorldAbilityReadyAt(const FOGEntityId& Unit,const FOGContentId& Skill) const
{ return WorldAbilityReadyAt.FindRef(WorldKey(Unit,Skill)); }
FOGEntityId FOGTurnBattle::GetUnitWorldCast(const FOGEntityId& Unit) const
{ for (const auto& Pair:WorldCasts) if (Pair.Value.Root.SourceUnitId==Unit) return Pair.Key; return FOGEntityId(); }
bool FOGTurnBattle::ReadWorldCast(const FOGEntityId& Id,FOGWorldAbilityCastView& Out) const
{
    Out=FOGWorldAbilityCastView(); const auto* Cast=WorldCasts.Find(Id); if (!Cast) return false;
    Out.CastId=Id; Out.SourceUnitId=Cast->Root.SourceUnitId; Out.SkillId=Cast->Root.SkillId;
    Out.StartedAtMilliseconds=Cast->StartedAt; Out.EndsAtMilliseconds=Cast->StartedAt+Cast->Specification.DurationMilliseconds;
    Out.CompletedSteps=Cast->CompletedSteps.Num(); Out.bCostsPaid=Cast->bCostsPaid; return true;
}
bool FOGTurnBattle::FilterWorldContacts(const FOGResolvedCombatAction& Root,const TArray<FOGEntityId>& Raw,
    TArray<FOGEntityId>& Out,FString& Error) const
{
    Out.Reset(); const auto* Source=FindUnit(Root.SourceUnitId);
    const auto* D=Source?FindNativeAbility(*Source,Root.SkillId):nullptr;
    if (!D || !IsNativeAbilityAuthorized(*Source,Root.SkillId))
    { Error=TEXT("World contact source has no admitted owned ability."); return false; }
    const bool Self=D->Targets==EOGNativeAbilityTargets::Self;
    const bool Allies=Self || D->Targets==EOGNativeAbilityTargets::SingleAlly || D->Targets==EOGNativeAbilityTargets::AllAllies;
    const bool Single=D->Targets==EOGNativeAbilityTargets::SingleEnemy || D->Targets==EOGNativeAbilityTargets::SingleAlly;
    for (const auto& Id:Raw)
    {
        const auto* Target=FindUnit(Id);
        if (!Target || !Target->CanAct() || (Self && Id!=Source->UnitEntityId) ||
            (!Self && ((Target->TeamIndex==Source->TeamIndex)!=Allies)) ||
            (!Self && Id==Source->UnitEntityId && !D->bIncludeSelf) ||
            (Single && !Root.TargetUnitIds.IsEmpty() && Id!=Root.TargetUnitIds[0])) continue;
        Out.AddUnique(Id); if (Single) break;
    }
    Out.Sort([](const auto& A,const auto& B){return A.ToString()<B.ToString();});
    return true;
}
bool FOGTurnBattle::SetExecutionMode(EOGCombatExecutionMode Mode,FString& Error)
{
    FOGTurnBattle Candidate=*this;
    if (!Candidate.SetExecutionModeMutating(Mode,Error)) return false;
    *this=MoveTemp(Candidate); return true;
}
bool FOGTurnBattle::SetExecutionModeMutating(EOGCombatExecutionMode Mode,FString& Error)
{
    Error.Reset();
    if (State.Status!=EOGTurnBattleStatus::Running || !WorldCasts.IsEmpty() ||
        !OutstandingTriggeredActionSequences.IsEmpty() || static_cast<uint8>(Mode)>static_cast<uint8>(EOGCombatExecutionMode::World))
    { Error=TEXT("Combat mode changes require a running encounter at a completed action boundary."); return false; }
    if (Mode!=State.ExecutionMode)
        for (const auto& Unit:State.Units) for (const auto& I:Unit.NativeEffects)
        {
            const auto* D=NativeEffectDefinitions.Find(I.EffectId);
            if (!D || D->Effect.Lifetime.Kind!=EOGEffectLifetimeKind::Encounter)
            { Error=TEXT("Complete finite status lifetimes before changing combat clocks."); return false; }
        }
    if (Mode==EOGCombatExecutionMode::World)
    {
        for (const auto& Pair:NativeEffectDefinitions) if (!ValidateNativeWorldEffectTiming(Pair.Value,Error)) return false;
        for (const auto& Unit:State.Units)
        {
            if (!Unit.CanAct()) continue;
            TArray<FOGContentId> Skills=Unit.SkillSet.ActiveSkills; Skills.Append(Unit.SkillSet.GetUltimateSkills());
            for (const auto& Skill:Skills)
            {
                if (!FindWorldAbility(Unit,Skill)) { Error=TEXT("Missing exact World staging for owned skill: ")+Skill.ToString(); return false; }
                if (NativeOwnerActions.FindRef(Unit.UnitEntityId)+1<NativeAbilityReadyActions.FindRef(WorldKey(Unit.UnitEntityId,Skill)))
                { Error=TEXT("Finish active own-action cooldowns before changing combat mode."); return false; }
            }
        }
    }
    else
        for (const auto& Pair:WorldAbilityReadyAt) if (Pair.Value>State.WorldElapsedMilliseconds)
        { Error=TEXT("Finish active World cooldowns before changing combat mode."); return false; }
    if (State.ExecutionMode==Mode) return true;
    const auto Previous=State.ExecutionMode; State.ExecutionMode=Mode;
    if (Mode==EOGCombatExecutionMode::Turn && !ValidateInitialState(Error)) { State.ExecutionMode=Previous; return false; }
    for (auto& Unit:State.Units) for (auto& I:Unit.NativeEffects)
    {
        const auto& D=NativeEffectDefinitions.FindChecked(I.EffectId);
        const int64 Interval=Mode==EOGCombatExecutionMode::World?D.WorldTickIntervalMilliseconds:0;
        if (Interval>MAX_int64-State.WorldElapsedMilliseconds)
        { State.ExecutionMode=Previous; Error=TEXT("World effect interval exhausted."); return false; }
        I.WorldTickIntervalMilliseconds=Interval;
        I.NextWorldTickMilliseconds=Interval>0?State.WorldElapsedMilliseconds+Interval:-1;
    }
    LastNativeReactions.Reset(); ++NativeAbilityRevision; return true;
}
bool FOGTurnBattle::BeginWorldAbility(const FOGResolvedCombatAction& Root,FString& Error)
{
    Error.Reset(); const auto* Source=FindUnit(Root.SourceUnitId);
    const auto* Spec=Source?FindWorldAbility(*Source,Root.SkillId):nullptr;
    if (State.Status!=EOGTurnBattleStatus::Running || State.ExecutionMode!=EOGCombatExecutionMode::World ||
        !Root.ActionId.IsValid() || WorldCasts.Contains(Root.ActionId) || !Source || !Source->CanAct() || !Spec ||
        !IsNativeAbilityAuthorized(*Source,Root.SkillId) || Root.InterruptPriority!=0 ||
        GetUnitWorldCast(Source->UnitEntityId).IsValid() || !OutstandingTriggeredActionSequences.IsEmpty() ||
        Spec->DurationMilliseconds>MAX_int64-State.WorldElapsedMilliseconds ||
        Spec->CooldownMilliseconds>MAX_int64-State.WorldElapsedMilliseconds)
    { Error=TEXT("No ready real World ability owner/staging or another cast is active."); return false; }
    FOGTurnBattle Candidate=*this; Candidate.LastNativeReactions.Reset(); Candidate.WorldExecutionProof=Root;
    if (!Candidate.ValidateNativeAbility(Root,*Candidate.FindMutableUnit(Root.SourceUnitId),Error)) return false;
    Candidate.WorldExecutionProof.Reset();
    if (!Candidate.BeginNativeOwnerAction(Root.SourceUnitId,Error)) return false;
    FWorldCast Cast; Cast.Root=Root; Cast.Specification=*Spec; Cast.StartedAt=State.WorldElapsedMilliseconds;
    Cast.Seed=WorldSeed(State.BattleId,Root.SourceUnitId,Root.SkillId,Log.GetNextSequence());
    auto* Owner=Candidate.FindMutableUnit(Root.SourceUnitId);
    if (Spec->CostTiming==EOGWorldAbilityCostTiming::AcceptedStart)
    {
        FOGDeterministicRng Rng(Cast.Seed);
        if (!Candidate.SpendNativeAbilityCosts(Root,*Owner,Error) ||
            !Candidate.ExecuteNativeEffects(Root,true,Rng,Error,1)) return false;
        Cast.bCostsPaid=true;
    }
    Candidate.WorldCasts.Add(Root.ActionId,Cast);
    Candidate.WorldAbilityReadyAt.Add(WorldKey(Root.SourceUnitId,Root.SkillId),State.WorldElapsedMilliseconds+Spec->CooldownMilliseconds);
    FOGCombatLogEvent Event; Event.Type=EOGCombatLogEventType::ActionDeclared; Event.SourceUnitId=Root.SourceUnitId;
    Event.SkillId=Root.SkillId; Event.PayloadJson=TEXT("{\"world_cast\":true}"); Candidate.Log.Append(MoveTemp(Event));
    if (!Candidate.ResolveNativeReactions(Error,&Candidate.WorldCasts.FindChecked(Root.ActionId).Activations)) return false;
    *this=MoveTemp(Candidate); return true;
}
bool FOGTurnBattle::ApplyWorldAbilityStep(const FOGEntityId& Id,int32 StepIndex,const TArray<FOGEntityId>& Contacts,FString& Error)
{
    const auto* Current=WorldCasts.Find(Id);
    if (!Current || !Current->Specification.Steps.IsValidIndex(StepIndex) || Current->CompletedSteps.Contains(StepIndex))
    { Error=TEXT("World impact step is absent or was already committed."); return false; }
    const auto& Step=Current->Specification.Steps[StepIndex];
    if (State.WorldElapsedMilliseconds<Current->StartedAt+Step.OpensAtMilliseconds ||
        (!Contacts.IsEmpty() && (State.WorldElapsedMilliseconds>Current->StartedAt+Current->Specification.DurationMilliseconds ||
         (Step.ClosesAtMilliseconds>Step.OpensAtMilliseconds && State.WorldElapsedMilliseconds>Current->StartedAt+Step.ClosesAtMilliseconds))))
    { Error=TEXT("World impact is outside its accepted cast lifetime."); return false; }
    FOGTurnBattle Candidate=*this; Candidate.LastNativeReactions.Reset();
    auto& Cast=Candidate.WorldCasts.FindChecked(Id); auto* Source=Candidate.FindMutableUnit(Cast.Root.SourceUnitId);
    if (!Source || !Source->CanAct()) { Error=TEXT("World impact source is no longer active."); return false; }
    FOGResolvedCombatAction Action=Cast.Root; Action.TargetUnitIds=Contacts;
    TSet<FOGEntityId> Unique;
    for (const auto& Target:Contacts)
    {
        if (Unique.Contains(Target)) { Error=TEXT("Duplicate physical impact target."); return false; }
        Unique.Add(Target);
    }
    if (!Contacts.IsEmpty())
    {
        Candidate.WorldExecutionProof=Action;
        if (!Candidate.ValidateNativeAbility(Action,*Source,Error)) return false;
        FOGDeterministicRng Rng(Cast.Seed^(static_cast<uint64>(StepIndex+1)*0x9e3779b97f4a7c15ull));
        if (!Cast.bCostsPaid)
        {
            if (!Candidate.SpendNativeAbilityCosts(Action,*Source,Error) ||
                !Candidate.ExecuteNativeEffects(Action,true,Rng,Error,1)) return false;
            Cast.bCostsPaid=true;
        }
        auto BeforeTargets=Action; BeforeTargets.TargetUnitIds.Reset();
        for (const auto& Target:Contacts)
        {
            Cast.ContactTargets.AddUnique(Target);
            if (!Cast.BeforeEffectTargets.Contains(Target)) { Cast.BeforeEffectTargets.Add(Target); BeforeTargets.TargetUnitIds.Add(Target); }
        }
        if (!BeforeTargets.TargetUnitIds.IsEmpty() && !Candidate.ExecuteNativeEffects(BeforeTargets,true,Rng,Error,2)) return false;
        if (!Candidate.ExecuteNativePacketSlices(Action,*Source,Step.Packets,Rng,Error)) return false;
        Cast.bHadContact=true;
        Candidate.WorldExecutionProof.Reset();
    }
    Cast.CompletedSteps.Add(StepIndex);
    if (Cast.CompletedSteps.Num()==Cast.Specification.Steps.Num() &&
        Candidate.State.WorldElapsedMilliseconds>=Cast.StartedAt+Cast.Specification.DurationMilliseconds)
    {
        if (!Candidate.FinishWorldAbility(Id,Error)) return false;
        *this=MoveTemp(Candidate); return true;
    }
    Candidate.FlushTriggerRuntimeQueueToBattleQueue();
    if (!Candidate.ResolveNativeReactions(Error,&Cast.Activations)) return false;
    *this=MoveTemp(Candidate); return true;
}
bool FOGTurnBattle::CompleteWorldOwnerAction(const FOGResolvedCombatAction& Root,FString& Error,TMap<FString,int32>* RootActivations)
{
    auto& Count=NativeOwnerActions.FindOrAdd(Root.SourceUnitId);
    if (Count==MAX_int64) { Error=TEXT("Owner action clock exhausted."); return false; }
    ++Count;
    if (!ProcessNativeEffectOwnerAction(Root.SourceUnitId,Error)) return false;
    FOGCombatLogEvent Event; Event.Type=EOGCombatLogEventType::ActionResolved; Event.SourceUnitId=Root.SourceUnitId;
    Event.SkillId=Root.SkillId; Event.PayloadJson=TEXT("{\"world_action_complete\":true}");
    const int64 Sequence=Log.GetNextSequence(); Log.Append(MoveTemp(Event));
    QueueTriggerEvent(TEXT("OnActionResolved"),Root.SourceUnitId,Root.TargetUnitIds.IsEmpty()?FOGEntityId():Root.TargetUnitIds[0],Root.SkillId,Sequence);
    QueueTriggerEvent(TEXT("OnOwnerActionEnd"),Root.SourceUnitId,Root.SourceUnitId,Root.SkillId,Sequence);
    FlushTriggerRuntimeQueueToBattleQueue(); return ResolveNativeReactions(Error,RootActivations) && ResolvePendingReplacements(Error);
}
bool FOGTurnBattle::FinishWorldAbility(const FOGEntityId& Id,FString& Error)
{
    const auto* Found=WorldCasts.Find(Id); if (!Found) return true;
    if (State.WorldElapsedMilliseconds<Found->StartedAt+Found->Specification.DurationMilliseconds ||
        Found->CompletedSteps.Num()!=Found->Specification.Steps.Num())
    { Error=TEXT("World cast still has uncompleted impact/recovery intervals."); return false; }
    const auto Cast=*Found; WorldCasts.Remove(Id);
    auto* Source=FindMutableUnit(Cast.Root.SourceUnitId); if (!Source || !Source->CanAct()) return ResolvePendingReplacements(Error);
    auto Root=Cast.Root; Root.TargetUnitIds=Cast.ContactTargets;
    if (Cast.bCostsPaid)
    {
        FOGDeterministicRng Rng(Cast.Seed^0xd1b54a32d192ed03ull);
        if (!ExecuteNativeEffects(Root,false,Rng,Error) || !GrantNativeAbilityResources(Root,*Source,Error)) return false;
    }
    auto Activations=Cast.Activations; return CompleteWorldOwnerAction(Root,Error,&Activations);
}
bool FOGTurnBattle::CancelWorldAbilityMutating(const FOGEntityId& Id,FString& Error)
{
    const auto* Found=WorldCasts.Find(Id); if (!Found) return true;
    const auto Cast=*Found; WorldCasts.Remove(Id); auto* Owner=FindMutableUnit(Cast.Root.SourceUnitId);
    FOGCombatLogEvent Cancelled; Cancelled.Type=EOGCombatLogEventType::ActionResolved;
    Cancelled.SourceUnitId=Cast.Root.SourceUnitId; Cancelled.SkillId=Cast.Root.SkillId;
    Cancelled.PayloadJson=TEXT("{\"world_cast_cancelled\":true}"); Log.Append(MoveTemp(Cancelled));
    if (Cast.bCostsPaid && Cast.Specification.bRefundCostsOnCancel && Owner)
    {
        const auto* Ability=FindNativeAbility(*Owner,Cast.Root.SkillId);
        if (!Ability) { Error=TEXT("Cancelled cast lost its admitted resource contract."); return false; }
        for (const auto& Cost:Ability->ResourceCosts)
        {
            auto* Resource=Owner->BattleResources.FindByPredicate([&](const auto& R){return R.ResourceId==Cost.ResourceId;});
            if (!Resource) { Error=TEXT("Cancelled cast lost its admitted resource owner."); return false; }
            Resource->Value=Cost.Amount>Resource->Maximum-Resource->Value?Resource->Maximum:Resource->Value+Cost.Amount;
        }
    }
    if (Cast.Specification.bCancelledCastCountsAsOwnerAction && Owner && Owner->CanAct())
    { auto Activations=Cast.Activations; return CompleteWorldOwnerAction(Cast.Root,Error,&Activations); }
    return ResolvePendingReplacements(Error);
}
bool FOGTurnBattle::CancelWorldAbility(const FOGEntityId& Id,FString& Error)
{
    FOGTurnBattle Candidate=*this; Candidate.LastNativeReactions.Reset();
    if (!Candidate.CancelWorldAbilityMutating(Id,Error)) return false;
    *this=MoveTemp(Candidate); return true;
}
bool FOGTurnBattle::AdvanceWorldCombatClock(int64 Target,FString& Error)
{
    if (State.ExecutionMode!=EOGCombatExecutionMode::World || State.Status!=EOGTurnBattleStatus::Running ||
        Target<State.WorldElapsedMilliseconds) { Error=TEXT("World combat clock must advance monotonically in a live World encounter."); return false; }
    FOGTurnBattle Candidate=*this; Candidate.LastNativeReactions.Reset();
    // Bounded catch-up preserves the unconsumed elapsed time in GameCore; no periodic packets are dropped.
    constexpr int32 MaximumEventsPerAdvance=2048;
    for (int32 EventIndex=0;EventIndex<MaximumEventsPerAdvance && Candidate.State.Status==EOGTurnBattleStatus::Running;++EventIndex)
    {
        TArray<FOGEntityId> CastIds; Candidate.WorldCasts.GetKeys(CastIds);
        CastIds.Sort([](const auto& A,const auto& B){return A.ToString()<B.ToString();});
        FOGEntityId ReadyCast; int64 NextCast=MAX_int64;
        for (const auto& Id:CastIds)
        {
            const auto& Cast=Candidate.WorldCasts.FindChecked(Id);
            const auto* Owner=Candidate.FindUnit(Cast.Root.SourceUnitId);
            const int64 Due=(!Owner || !Owner->CanAct())?Candidate.State.WorldElapsedMilliseconds:
                (Cast.CompletedSteps.Num()==Cast.Specification.Steps.Num()?Cast.StartedAt+Cast.Specification.DurationMilliseconds:MAX_int64);
            if (!ReadyCast.IsValid() || Due<NextCast) { ReadyCast=Id; NextCast=Due; }
        }
        const int64 NextEffect=Candidate.GetNextNativeWorldEffectTime();
        const int64 Next=FMath::Min(NextCast,NextEffect);
        if (Next==MAX_int64 || Next>Target)
        { Candidate.State.WorldElapsedMilliseconds=Target; break; }
        Candidate.State.WorldElapsedMilliseconds=FMath::Max(Candidate.State.WorldElapsedMilliseconds,Next);
        if (NextEffect<=NextCast)
        { if (!Candidate.ProcessNativeWorldEffectClock(Next,Error)) return false; }
        else
        {
            const auto* Cast=Candidate.WorldCasts.Find(ReadyCast);
            const auto* Owner=Cast?Candidate.FindUnit(Cast->Root.SourceUnitId):nullptr;
            if (!Owner || !Owner->CanAct())
            { if (!Candidate.CancelWorldAbilityMutating(ReadyCast,Error)) return false; }
            else if (!Candidate.FinishWorldAbility(ReadyCast,Error)) return false;
        }
    }
    if (Candidate.Log.GetNextSequence()==Log.GetNextSequence()) Candidate.LastNativeReactions=LastNativeReactions;
    *this=MoveTemp(Candidate); return true;
}

bool FOGTurnBattle::IsWorldDamagingStep(const FOGCombatUnitState& Unit,const FOGContentId& Skill,int32 Step) const
{
    const auto* World=FindWorldAbility(Unit,Skill); const auto* Ability=FindNativeAbility(Unit,Skill);
    if (!World || !Ability || !World->Steps.IsValidIndex(Step) || World->Steps[Step].bIgnoreDodgeInvulnerability) return false;
    for (const auto& Slice:World->Steps[Step].Packets)
        if (Ability->Packets.IsValidIndex(Slice.PacketIndex) && Ability->Packets[Slice.PacketIndex].Effect==EOGNativeAbilityEffect::Damage) return true;
    return false;
}
