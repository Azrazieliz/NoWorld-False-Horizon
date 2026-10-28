#include "Combat/OGCombatActionJournal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FOGCombatUnitState MakeJournalUnit(
    const FOGEntityId& UnitId,
    int64 Hp)
{
    FOGCombatUnitState Unit;
    Unit.UnitEntityId = UnitId;
    Unit.TeamIndex = 0;
    Unit.Presence = EOGCombatPresence::Active;
    Unit.CurrentHp = FOGLargeNumber::FromInt64(Hp);
    Unit.Stats.MaxHp = FOGLargeNumber::FromInt64(100);
    return Unit;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGCombatActionJournalRecordsCanonicalDelta,
    "OfflineGame.Combat.Persistence.ActionJournalRecordsCanonicalDelta",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::EngineFilter)

bool FOGCombatActionJournalRecordsCanonicalDelta::RunTest(
    const FString& Parameters)
{
    const FOGEntityId BattleId = FOGEntityId::NewId();
    const FOGEntityId RulerId = FOGEntityId::NewId();
    const FOGEntityId SourceId = FOGEntityId::NewId();
    const FOGEntityId TargetId = FOGEntityId::NewId();
    const FOGEntityId SecondaryId = FOGEntityId::NewId();
    const FOGEntityId UnchangedId = FOGEntityId::NewId();

    FOGTurnBattleState Before;
    Before.BattleId = BattleId;
    Before.Status = EOGTurnBattleStatus::Running;
    Before.Units =
    {
        MakeJournalUnit(SourceId, 100),
        MakeJournalUnit(TargetId, 100),
        MakeJournalUnit(SecondaryId, 100),
        MakeJournalUnit(UnchangedId, 100)
    };

    FOGTurnBattleState After = Before;
    After.CurrentActionValue = 1200;
    After.Units[1].CurrentHp = FOGLargeNumber::FromInt64(40);
    After.Units[2].CurrentHp = FOGLargeNumber::FromInt64(75);
    Before.CurrentActionValue = 9007199254740993LL;
    After.CurrentActionValue = Before.CurrentActionValue + 1200;

    FOGResolvedCombatAction Action;
    Action.ActionId = FOGEntityId::NewId();
    Action.SourceUnitId = SourceId;
    Action.TargetUnitIds.Add(TargetId);
    Action.SkillId = FOGContentId(TEXT("combat:test.skill"));
    Action.ActionDelay = 1200;

    FOGWorldEvent Event;
    FString Error;
    TestTrue(
        TEXT("Builds an event from an accepted authoritative action"),
        FOGCombatActionJournal::BuildResolvedActionEvent(
            Before, After, Action, RulerId, 77, Event, Error));
    TestEqual(TEXT("Event type is the canonical action event"),
        Event.EventType, FName(TEXT("combat.turn_action_resolved")));
    TestEqual(TEXT("World tick is preserved"), Event.WorldTick, int64(77));
    TestTrue(TEXT("Ruler is the primary causal entity"),
        Event.PrimaryEntity == RulerId);
    TestTrue(TEXT("Payload contains battle and action identity"),
        Event.PayloadJson.Contains(BattleId.ToString()) &&
        Event.PayloadJson.Contains(Action.ActionId.ToString()));
    TestTrue(TEXT("Payload contains the authoritative HP delta"),
        Event.PayloadJson.Contains(TEXT("4e1")));
    TestTrue(TEXT("Secondary resolved effects are related factual subjects"), Event.RelatedEntities.Contains(SecondaryId));
    TestFalse(TEXT("An unaffected unit is not fabricated as a factual subject"), Event.RelatedEntities.Contains(UnchangedId));
    TSharedPtr<FJsonObject> Payload;
    if (TestTrue(TEXT("Actual journal payload parses"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Event.PayloadJson), Payload)))
    {
        TestEqual(TEXT("Large action value retains all exact digits"), Payload->GetStringField(TEXT("action_value_before_exact")), LexToString(Before.CurrentActionValue));
        TestEqual(TEXT("Resolved exact action value retains all exact digits"), Payload->GetStringField(TEXT("action_value_after_exact")), LexToString(After.CurrentActionValue));
    }

    return true;
}

#endif
