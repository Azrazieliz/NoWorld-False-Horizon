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
    OutEvent.PayloadJson = MoveTemp(PayloadJson);
    OutEvent.bChronicleEligible = true;
    return true;
}
