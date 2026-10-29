#include "Combat/OGCombatActionJournal.h"

#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
const FOGCombatUnitState* FindUnit(
    const FOGTurnBattleState& State,
    const FOGEntityId& UnitId)
{
    return State.Units.FindByPredicate(
        [&UnitId](const FOGCombatUnitState& Unit)
        {
            return Unit.UnitEntityId == UnitId;
        });
}

void AddOutcomeState(const FOGTurnBattleState& State,const TSharedRef<FJsonObject>& Payload)
{
    Payload->SetNumberField(TEXT("battle_status_after"),static_cast<uint8>(State.Status));
    TArray<int32> Living;
    for (const auto& Unit:State.Units)
        if (Unit.IsAlive() && Unit.Presence!=EOGCombatPresence::Removed) Living.AddUnique(Unit.TeamIndex);
    Living.Sort(); TArray<TSharedPtr<FJsonValue>> Teams;
    for (const auto Team:Living) Teams.Add(MakeShared<FJsonValueNumber>(Team));
    Payload->SetArrayField(TEXT("living_team_indices_after"),MoveTemp(Teams));
}
bool AddUnitDelta(
    const FOGCombatUnitState& Before,
    const FOGCombatUnitState& After,
    TArray<TSharedPtr<FJsonValue>>& OutDeltas)
{
    if (Before.CurrentHp == After.CurrentHp &&
        Before.Presence == After.Presence &&
        Before.NextActionValue == After.NextActionValue &&
        Before.OccupiedLaneIndex == After.OccupiedLaneIndex &&
        Before.BattleResources == After.BattleResources && Before.NativeEffects == After.NativeEffects)
    {
        return false;
    }

    TSharedRef<FJsonObject> Delta = MakeShared<FJsonObject>();
    Delta->SetStringField(TEXT("unit_id"), After.UnitEntityId.ToString());
    Delta->SetStringField(TEXT("hp_before"), Before.CurrentHp.ToDebugString());
    Delta->SetStringField(TEXT("hp_after"), After.CurrentHp.ToDebugString());
    Delta->SetNumberField(TEXT("presence_before"),
        static_cast<double>(static_cast<uint8>(Before.Presence)));
    Delta->SetNumberField(TEXT("presence_after"),
        static_cast<double>(static_cast<uint8>(After.Presence)));
    Delta->SetStringField(TEXT("next_action_before"),
        LexToString(Before.NextActionValue));
    Delta->SetStringField(TEXT("next_action_after"),
        LexToString(After.NextActionValue));
    Delta->SetNumberField(TEXT("lane_before"),
        static_cast<double>(Before.OccupiedLaneIndex));
    Delta->SetNumberField(TEXT("lane_after"),
        static_cast<double>(After.OccupiedLaneIndex));
    auto Resources=[](const TArray<FOGCombatResourceValue>& Values)
    {
        TArray<TSharedPtr<FJsonValue>> Rows;
        for (const auto& Value:Values)
        {
            auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("resource_id"),Value.ResourceId.ToString());
            Row->SetStringField(TEXT("value_exact"),LexToString(Value.Value));
            Row->SetStringField(TEXT("maximum_exact"),LexToString(Value.Maximum));
            Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        return Rows;
    };
    Delta->SetArrayField(TEXT("battle_resources_before"),Resources(Before.BattleResources));
    Delta->SetArrayField(TEXT("battle_resources_after"),Resources(After.BattleResources));
    auto Effects=[](const TArray<FOGNativeEffectInstance>& Values)
    {
        TArray<TSharedPtr<FJsonValue>> Rows;
        for (const auto& I:Values)
        {
            auto Row=MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("instance_id"),I.InstanceId.ToString());
            Row->SetStringField(TEXT("effect_id"),I.EffectId.ToString());
            Row->SetStringField(TEXT("source_unit_id"),I.SourceUnitId.ToString());
            Row->SetStringField(TEXT("source_skill_id"),I.SourceSkillId.ToString());
            Row->SetStringField(TEXT("clock_unit_id"),I.ClockUnitId.ToString());
            Row->SetNumberField(TEXT("stacks"),I.Stacks);
            Row->SetStringField(TEXT("remaining_actions_exact"),LexToString(I.RemainingActions));
            Row->SetStringField(TEXT("applied_at_owner_action_exact"),LexToString(I.AppliedAtOwnerAction));
            Row->SetStringField(TEXT("expires_world_ms_exact"),LexToString(I.ExpiresAtWorldMilliseconds));
            Row->SetStringField(TEXT("next_world_tick_ms_exact"),LexToString(I.NextWorldTickMilliseconds));
            Row->SetStringField(TEXT("world_tick_interval_ms_exact"),LexToString(I.WorldTickIntervalMilliseconds));
            Row->SetStringField(TEXT("magnitude_per_stack"),I.MagnitudePerStack.ToDebugString());
            Row->SetStringField(TEXT("barrier_remaining"),I.BarrierRemaining.ToDebugString());
            Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        return Rows;
    };
    Delta->SetArrayField(TEXT("effects_before"),Effects(Before.NativeEffects));
    Delta->SetArrayField(TEXT("effects_after"),Effects(After.NativeEffects));
    OutDeltas.Add(MakeShared<FJsonValueObject>(Delta));
    return true;
}

bool AddNativeReactions(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
    const TArray<FOGNativeReactionRecord>& Reactions,const TSharedRef<FJsonObject>& Payload,
    TArray<FOGEntityId>& Related,FString& Error)
{
    TArray<TSharedPtr<FJsonValue>> Rows; TSet<int64> Queues;
    for (const auto& R:Reactions)
    {
        if (R.QueueSequence<0 || R.SourceSequence<0 || Queues.Contains(R.QueueSequence) ||
            !R.PassiveSkillId.IsValid() || !R.TriggerId.IsValid() || R.EventType.IsNone() || R.Outcome.IsEmpty() ||
            !FindUnit(Before,R.Action.SourceUnitId) || !FindUnit(After,R.Action.SourceUnitId) || !R.Action.SkillId.IsValid())
        { Error=TEXT("Native reaction receipt has invalid causal identity."); return false; }
        Queues.Add(R.QueueSequence);
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("queue_sequence_exact"),LexToString(R.QueueSequence));
        Row->SetStringField(TEXT("source_sequence_exact"),LexToString(R.SourceSequence));
        Row->SetStringField(TEXT("passive_skill_id"),R.PassiveSkillId.ToString());
        Row->SetStringField(TEXT("trigger_id"),R.TriggerId.ToString());
        Row->SetStringField(TEXT("event_type"),R.EventType.ToString());
        Row->SetStringField(TEXT("event_source_unit_id"),R.EventSourceUnitId.IsValid()?R.EventSourceUnitId.ToString():TEXT(""));
        Row->SetStringField(TEXT("event_target_unit_id"),R.EventTargetUnitId.IsValid()?R.EventTargetUnitId.ToString():TEXT(""));
        Row->SetStringField(TEXT("event_skill_id"),R.EventSkillId.ToString());
        Row->SetStringField(TEXT("source_unit_id"),R.Action.SourceUnitId.ToString());
        Row->SetStringField(TEXT("skill_id"),R.Action.SkillId.ToString());
        Row->SetStringField(TEXT("outcome"),R.Outcome); Row->SetBoolField(TEXT("executed"),R.bExecuted);
        if (R.bExecuted)
        {
            if (!R.Action.ActionId.IsValid() || R.Action.TargetUnitIds.IsEmpty() || R.Action.InterruptPriority<=0)
            { Error=TEXT("Executed native reaction has no concrete accepted action."); return false; }
            Row->SetStringField(TEXT("action_id"),R.Action.ActionId.ToString());
            Row->SetStringField(TEXT("resolution_json"),R.Action.ResolutionJson);
            Related.AddUnique(R.Action.SourceUnitId);
            TArray<TSharedPtr<FJsonValue>> Targets;
            for (const auto& Id:R.Action.TargetUnitIds)
            {
                if (!FindUnit(Before,Id) || !FindUnit(After,Id)) { Error=TEXT("Native reaction target is absent from recorded snapshots."); return false; }
                Related.AddUnique(Id); Targets.Add(MakeShared<FJsonValueString>(Id.ToString()));
            }
            Row->SetArrayField(TEXT("target_unit_ids"),MoveTemp(Targets));
        }
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Payload->SetArrayField(TEXT("native_reactions"),MoveTemp(Rows)); return true;
}
}

bool FOGCombatActionJournal::BuildResolvedActionEvent(
    const FOGTurnBattleState& Before,
    const FOGTurnBattleState& After,
    const FOGResolvedCombatAction& Action,
    const FOGEntityId& OwningRulerId,
    int64 WorldTick,
    FOGWorldEvent& OutEvent,
    FString& OutError)
{
    return BuildResolvedActionEvent(Before,After,Action,{},OwningRulerId,WorldTick,OutEvent,OutError);
}
bool FOGCombatActionJournal::BuildResolvedActionEvent(
    const FOGTurnBattleState& Before,const FOGTurnBattleState& After,const FOGResolvedCombatAction& Action,
    const TArray<FOGNativeReactionRecord>& Reactions,const FOGEntityId& OwningRulerId,int64 WorldTick,
    FOGWorldEvent& OutEvent,FString& OutError)
{
    OutEvent = FOGWorldEvent();
    OutError.Reset();

    if (!Before.BattleId.IsValid() ||
        After.BattleId != Before.BattleId ||
        !OwningRulerId.IsValid() ||
        WorldTick < 0 ||
        !Action.ActionId.IsValid() ||
        !Action.SourceUnitId.IsValid() ||
        Action.TargetUnitIds.IsEmpty() ||
        !Action.SkillId.IsValid())
    {
        OutError = TEXT("Canonical combat action event has incomplete identity.");
        return false;
    }

    if (!FindUnit(Before, Action.SourceUnitId) ||
        !FindUnit(After, Action.SourceUnitId))
    {
        OutError = TEXT("Canonical combat action source is absent from the battle snapshots.");
        return false;
    }

    TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("battle_id"), Before.BattleId.ToString());
    AddOutcomeState(After,Payload);
    Payload->SetStringField(TEXT("action_id"), Action.ActionId.ToString());
    Payload->SetStringField(TEXT("source_unit_id"), Action.SourceUnitId.ToString());
    Payload->SetStringField(TEXT("skill_id"), Action.SkillId.ToString());
    Payload->SetStringField(TEXT("action_delay"), LexToString(Action.ActionDelay));
    Payload->SetNumberField(TEXT("interrupt_priority"),
        static_cast<double>(Action.InterruptPriority));
    Payload->SetStringField(TEXT("resolution_json"), Action.ResolutionJson);
    Payload->SetNumberField(TEXT("action_value_before"),
        static_cast<double>(Before.CurrentActionValue));
    Payload->SetNumberField(TEXT("action_value_after"),
        static_cast<double>(After.CurrentActionValue));
    // Preserve the legacy numeric display fields while retaining every int64 digit.
    Payload->SetStringField(TEXT("action_value_before_exact"), LexToString(Before.CurrentActionValue));
    Payload->SetStringField(TEXT("action_value_after_exact"), LexToString(After.CurrentActionValue));

    TArray<TSharedPtr<FJsonValue>> Targets;
    for (const FOGEntityId& TargetId : Action.TargetUnitIds)
    {
        if (!TargetId.IsValid() || !FindUnit(Before, TargetId) ||
            !FindUnit(After, TargetId))
        {
            OutError = TEXT("Canonical combat action target is absent from the battle snapshots.");
            return false;
        }
        Targets.Add(MakeShared<FJsonValueString>(TargetId.ToString()));
    }
    Payload->SetArrayField(TEXT("target_unit_ids"), MoveTemp(Targets));

    TArray<TSharedPtr<FJsonValue>> UnitDeltas;
    TArray<FOGEntityId> AffectedUnits;
    for (const FOGCombatUnitState& BeforeUnit : Before.Units)
    {
        const FOGCombatUnitState* AfterUnit = FindUnit(After, BeforeUnit.UnitEntityId);
        if (AfterUnit)
        {
            if (AddUnitDelta(BeforeUnit, *AfterUnit, UnitDeltas))
                AffectedUnits.AddUnique(AfterUnit->UnitEntityId);
        }
    }
    Payload->SetArrayField(TEXT("unit_deltas"), MoveTemp(UnitDeltas));

    TArray<FOGEntityId> ReactionSubjects;
    if (!AddNativeReactions(Before,After,Reactions,Payload,ReactionSubjects,OutError)) return false;
    FString PayloadJson;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    if (!FJsonSerializer::Serialize(Payload, Writer))
    {
        OutError = TEXT("Failed to serialize canonical combat action event.");
        return false;
    }

    OutEvent.EventId = FOGEntityId::NewId();
    OutEvent.EventType = FName(TEXT("combat.turn_action_resolved"));
    OutEvent.WorldTick = WorldTick;
    OutEvent.PrimaryEntity = OwningRulerId;
    OutEvent.RelatedEntities.Add(Before.BattleId);
    OutEvent.RelatedEntities.AddUnique(Action.SourceUnitId);
    for (const FOGEntityId& TargetId : Action.TargetUnitIds)
    {
        OutEvent.RelatedEntities.AddUnique(TargetId);
    }
    // Secondary field/passive/reaction/succession effects also supply factual
    // subject evidence to quests and systemic interactions, beyond selected targets.
    for (const FOGEntityId& UnitId : AffectedUnits) OutEvent.RelatedEntities.AddUnique(UnitId);
    for (const auto& Id:ReactionSubjects) OutEvent.RelatedEntities.AddUnique(Id);
    OutEvent.PayloadJson = MoveTemp(PayloadJson);
    OutEvent.bChronicleEligible = true;
    return true;
}

bool FOGCombatActionJournal::BuildEncounterStartedEvent(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
    const TArray<FOGNativeReactionRecord>& Reactions,const FOGEntityId& Ruler,int64 Tick,FOGWorldEvent& Out,FString& Error)
{
    Out=FOGWorldEvent(); Error.Reset();
    if (!Before.BattleId.IsValid() || Before.BattleId!=After.BattleId || !Ruler.IsValid() || Tick<0 ||
        Before.Units.IsEmpty() || Before.Units.Num()!=After.Units.Num())
    { Error=TEXT("Native encounter opening has incomplete canonical identity."); return false; }
    auto Payload=MakeShared<FJsonObject>(); Payload->SetStringField(TEXT("battle_id"),After.BattleId.ToString());
    AddOutcomeState(After,Payload);
    TArray<TSharedPtr<FJsonValue>> Units; TArray<FOGEntityId> Related; TSet<FOGEntityId> Seen;
    for (const auto& Unit:After.Units)
    {
        const auto* Original=FindUnit(Before,Unit.UnitEntityId);
        if (!Original || !Unit.UnitEntityId.IsValid() || Seen.Contains(Unit.UnitEntityId))
        { Error=TEXT("Native opening changed participant identities."); return false; }
        Seen.Add(Unit.UnitEntityId); Related.Add(Unit.UnitEntityId);
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("unit_id"),Unit.UnitEntityId.ToString());
        Row->SetStringField(TEXT("identity_id"),Unit.IdentityId.ToString());
        Row->SetStringField(TEXT("version_id"),Unit.SkillSet.CurrentForm.ToString());
        Row->SetStringField(TEXT("hp_before"),Original->CurrentHp.ToDebugString());
        Row->SetStringField(TEXT("hp_after"),Unit.CurrentHp.ToDebugString());
        Row->SetStringField(TEXT("next_action_exact"),LexToString(Unit.NextActionValue));
        TArray<TSharedPtr<FJsonValue>> Resources;
        for (const auto& Value:Unit.BattleResources)
        {
            auto Resource=MakeShared<FJsonObject>(); Resource->SetStringField(TEXT("resource_id"),Value.ResourceId.ToString());
            Resource->SetStringField(TEXT("value_exact"),LexToString(Value.Value));
            Resource->SetStringField(TEXT("maximum_exact"),LexToString(Value.Maximum));
            Resources.Add(MakeShared<FJsonValueObject>(Resource));
        }
        Row->SetArrayField(TEXT("battle_resources"),MoveTemp(Resources)); Units.Add(MakeShared<FJsonValueObject>(Row));
    }
    Payload->SetArrayField(TEXT("opening_units"),MoveTemp(Units));
    if (!AddNativeReactions(Before,After,Reactions,Payload,Related,Error)) return false;
    if (!FJsonSerializer::Serialize(Payload.ToSharedRef(),TJsonWriterFactory<>::Create(&Out.PayloadJson)))
    { Error=TEXT("Cannot serialize native encounter opening."); return false; }
    FGuid Receipt;
    FGuid::ParseExact(FMD5::HashAnsiString(*(TEXT("combat.encounter_started|")+Before.BattleId.ToString())),EGuidFormats::Digits,Receipt);
    Out.EventId=FOGEntityId(Receipt); Out.EventType=TEXT("combat.encounter_started"); Out.WorldTick=Tick;
    Out.PrimaryEntity=Ruler; Out.RelatedEntities=MoveTemp(Related); Out.RelatedEntities.AddUnique(Before.BattleId);
    Out.bChronicleEligible=true; return true;
}

bool FOGCombatActionJournal::BuildWorldRuntimeEvent(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
    const TArray<FOGNativeReactionRecord>& Reactions,const FOGEntityId& Ruler,int64 Tick,FName Operation,
    const FOGResolvedCombatAction* Cause,int32 StepIndex,const TArray<FOGWorldAbilityCastView>& ActiveCasts,
    FOGWorldEvent& Out,FString& Error)
{
    Out=FOGWorldEvent(); Error.Reset();
    if (!Before.BattleId.IsValid() || Before.BattleId!=After.BattleId || !Ruler.IsValid() || Tick<0 ||
        (Operation!=TEXT("combat.world_mode_changed") && Operation!=TEXT("combat.world_cast_started") &&
         Operation!=TEXT("combat.world_impact_resolved") && Operation!=TEXT("combat.world_cast_cancelled") &&
         Operation!=TEXT("combat.world_clock_advanced")))
    { Error=TEXT("World combat receipt requires exact encounter, owner, clock and operation."); return false; }
    auto Payload=MakeShared<FJsonObject>(); Payload->SetStringField(TEXT("battle_id"),After.BattleId.ToString());
    Payload->SetNumberField(TEXT("mode_before"),static_cast<uint8>(Before.ExecutionMode));
    Payload->SetNumberField(TEXT("mode_after"),static_cast<uint8>(After.ExecutionMode));
    Payload->SetStringField(TEXT("world_milliseconds_before_exact"),LexToString(Before.WorldElapsedMilliseconds));
    Payload->SetStringField(TEXT("world_milliseconds_after_exact"),LexToString(After.WorldElapsedMilliseconds));
    AddOutcomeState(After,Payload); Payload->SetNumberField(TEXT("step_index"),StepIndex);
    TArray<FOGEntityId> Related; Related.Add(After.BattleId);
    if (Cause)
    {
        if (!Cause->ActionId.IsValid() || !FindUnit(Before,Cause->SourceUnitId) || !Cause->SkillId.IsValid())
        { Error=TEXT("World action cause has no actual admitted source."); return false; }
        Payload->SetStringField(TEXT("action_id"),Cause->ActionId.ToString());
        Payload->SetStringField(TEXT("source_unit_id"),Cause->SourceUnitId.ToString());
        Payload->SetStringField(TEXT("skill_id"),Cause->SkillId.ToString()); Related.AddUnique(Cause->SourceUnitId);
        TArray<TSharedPtr<FJsonValue>> Targets;
        for (const auto& Id:Cause->TargetUnitIds)
        { if (!FindUnit(Before,Id)) { Error=TEXT("World receipt target was not admitted."); return false; } Targets.Add(MakeShared<FJsonValueString>(Id.ToString())); Related.AddUnique(Id); }
        Payload->SetArrayField(TEXT("target_unit_ids"),MoveTemp(Targets));
    }
    TArray<TSharedPtr<FJsonValue>> Deltas,Casts;
    for (const auto& Unit:After.Units)
    {
        const auto* Previous=FindUnit(Before,Unit.UnitEntityId);
        if (!Previous) { Error=TEXT("World action cannot invent a new combat participant."); return false; }
        if (AddUnitDelta(*Previous,Unit,Deltas)) Related.AddUnique(Unit.UnitEntityId);
    }
    Payload->SetArrayField(TEXT("unit_deltas"),MoveTemp(Deltas));
    for (const auto& Cast:ActiveCasts)
    {
        auto Row=MakeShared<FJsonObject>(); Row->SetStringField(TEXT("cast_id"),Cast.CastId.ToString());
        Row->SetStringField(TEXT("source_unit_id"),Cast.SourceUnitId.ToString()); Row->SetStringField(TEXT("skill_id"),Cast.SkillId.ToString());
        Row->SetStringField(TEXT("started_ms_exact"),LexToString(Cast.StartedAtMilliseconds));
        Row->SetStringField(TEXT("ends_ms_exact"),LexToString(Cast.EndsAtMilliseconds));
        Row->SetNumberField(TEXT("completed_steps"),Cast.CompletedSteps); Row->SetBoolField(TEXT("costs_paid"),Cast.bCostsPaid);
        Casts.Add(MakeShared<FJsonValueObject>(Row));
    }
    Payload->SetArrayField(TEXT("active_world_casts"),MoveTemp(Casts));
    if (!AddNativeReactions(Before,After,Reactions,Payload,Related,Error)) return false;
    Out.EventId=FOGEntityId::NewId(); Out.EventType=Operation; Out.PrimaryEntity=Ruler; Out.RelatedEntities=MoveTemp(Related);
    Out.WorldTick=Tick; Out.bChronicleEligible=After.Status==EOGTurnBattleStatus::Completed;
    Out.PayloadJson.Reset(); return FJsonSerializer::Serialize(Payload.ToSharedRef(),TJsonWriterFactory<>::Create(&Out.PayloadJson));
}
