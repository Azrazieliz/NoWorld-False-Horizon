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

bool AddUnitDelta(
    const FOGCombatUnitState& Before,
    const FOGCombatUnitState& After,
    TArray<TSharedPtr<FJsonValue>>& OutDeltas)
{
    if (Before.CurrentHp == After.CurrentHp &&
        Before.Presence == After.Presence &&
        Before.NextActionValue == After.NextActionValue &&
        Before.OccupiedLaneIndex == After.OccupiedLaneIndex &&
        Before.BattleResources == After.BattleResources)
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
