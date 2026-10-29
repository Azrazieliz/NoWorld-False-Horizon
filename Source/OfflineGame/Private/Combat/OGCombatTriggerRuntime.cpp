#include "Combat/OGCombatTriggerRuntime.h"

void FOGCombatTriggerRuntime::SetBindings(
    TArray<FOGCombatTriggerBinding> InBindings)
{
    Bindings = MoveTemp(InBindings);

    Bindings.Sort(
        [](const FOGCombatTriggerBinding& A,
           const FOGCombatTriggerBinding& B)
        {
            const int32 TriggerCompare =
                A.Trigger.TriggerId.Value.Compare(
                    B.Trigger.TriggerId.Value,
                    ESearchCase::CaseSensitive);

            if (TriggerCompare != 0)
            {
                return TriggerCompare < 0;
            }

            return A.OwnerUnitId.ToString().Compare(
                B.OwnerUnitId.ToString(),
                ESearchCase::CaseSensitive) < 0;
        });
}

void FOGCombatTriggerRuntime::QueueForEvent(
    const FOGCombatTriggerContext& Context,const FOGCombatConditionEvaluator* Override)
{
    if (Context.EventType.IsNone())
    {
        return;
    }

    for (const FOGCombatTriggerBinding& Binding : Bindings)
    {
        if ((Context.bReactionOrigin && !Binding.bAllowReactionOrigin) ||
            Binding.Trigger.EventType != Context.EventType ||
            !ConditionsPass(Binding, Context,Override))
        {
            continue;
        }

        for (const FOGContentId& ActionId : Binding.Trigger.ActionIds)
        {
            if (!ActionId.IsValid())
            {
                continue;
            }

            FOGQueuedTriggeredAction Queued;
            Queued.QueueSequence = NextQueueSequence++;
            Queued.OwnerUnitId = Binding.OwnerUnitId;
            Queued.TriggerId = Binding.Trigger.TriggerId;
            Queued.ActionId = ActionId;
            Queued.SourceSequence = Context.SourceSequence;
            Queued.EventType=Context.EventType; Queued.EventSourceUnitId=Context.SourceUnitId;
            Queued.EventTargetUnitId=Context.TargetUnitId; Queued.EventSkillId=Context.SkillId;
            QueuedActions.Add(MoveTemp(Queued));
        }
    }
}

TArray<FOGQueuedTriggeredAction>
FOGCombatTriggerRuntime::DrainQueuedActions()
{
    TArray<FOGQueuedTriggeredAction> Result =
        MoveTemp(QueuedActions);
    QueuedActions.Reset();
    return Result;
}

void FOGCombatTriggerRuntime::Reset()
{
    Bindings.Reset();
    QueuedActions.Reset();
    ConditionEvaluator = nullptr;
    NextQueueSequence = 0;
}

bool FOGCombatTriggerRuntime::ConditionsPass(
    const FOGCombatTriggerBinding& Binding,
    const FOGCombatTriggerContext& Context,const FOGCombatConditionEvaluator* Override) const
{
    if (Binding.Trigger.Conditions.IsEmpty())
    {
        return true;
    }

    const auto& Evaluate=Override?*Override:ConditionEvaluator;
    if (!Evaluate)
    {
        return false;
    }

    for (const FOGConditionDefinition& Condition : Binding.Trigger.Conditions)
    {
        bool bResult =
            Evaluate(
                Binding,
                Condition,
                Context);

        if (Condition.bInvert)
        {
            bResult = !bResult;
        }

        if (!bResult)
        {
            return false;
        }
    }

    return true;
}
