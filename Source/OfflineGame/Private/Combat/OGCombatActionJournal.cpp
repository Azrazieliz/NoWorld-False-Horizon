#include "Combat/OGCombatActionJournal.h"

#include "Dom/JsonObject.h"
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

void AddUnitDelta(
    const FOGCombatUnitState& Before,
    const FOGCombatUnitState& After,
    TArray<TSharedPtr<FJsonValue>>& OutDeltas)
{
    if (Before.CurrentHp == After.CurrentHp &&
        Before.Presence == After.Presence &&
        Before.NextActionValue == After.NextActionValue &&
        Before.OccupiedLaneIndex == After.OccupiedLaneIndex)
    {
        return;
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
    OutDeltas.Add(MakeShared<FJsonValueObject>(Delta));
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
    for (const FOGCombatUnitState& BeforeUnit : Before.Units)
    {
        const FOGCombatUnitState* AfterUnit = FindUnit(After, BeforeUnit.UnitEntityId);
        if (AfterUnit)
        {
            AddUnitDelta(BeforeUnit, *AfterUnit, UnitDeltas);
        }
    }
    Payload->SetArrayField(TEXT("unit_deltas"), MoveTemp(UnitDeltas));

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
    OutEvent.PayloadJson = MoveTemp(PayloadJson);
    OutEvent.bChronicleEligible = true;
    return true;
}
