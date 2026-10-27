#include "Combat/OGCombatActionJournal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

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

    FOGTurnBattleState Before;
    Before.BattleId = BattleId;
    Before.Status = EOGTurnBattleStatus::Running;
    Before.Units =
    {
        MakeJournalUnit(SourceId, 100),
        MakeJournalUnit(TargetId, 100)
    };

    FOGTurnBattleState After = Before;
    After.CurrentActionValue = 1200;
    After.Units[1].CurrentHp = FOGLargeNumber::FromInt64(40);

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

    return true;
}

#endif
