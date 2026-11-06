#include "Narrative/OGNarrativeSessionSubsystem.h"

#include "Content/OGDefinitionLibrary.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"

void UOGNarrativeSessionSubsystem::OnQuestLifecycleForeground()
{
    bQuestLifecycleForegroundRequested.Store(true);
}

void UOGNarrativeSessionSubsystem::RequestQuestLifecycleCatchup()
{
    bQuestLifecycleNeedsCatchup = true;
    QuestLifecycleRetryAfter = 0.0;
}

void UOGNarrativeSessionSubsystem::ResetQuestLifecycle()
{
    QuestLifecycleRuler = FOGEntityId();
    ClosingQuestDefinitions.Reset();
    QuestClosingEventTypes.Reset();
    PendingQuestClosingTicks.Reset();
    QuestLifecycleCatalogRevision = -1;
    bQuestLifecycleCatalogLoaded = false;
    RequestQuestLifecycleCatchup();
}

void UOGNarrativeSessionSubsystem::QueueQuestClosingEvents(const TArray<FOGWorldEvent>& Events)
{
    // GameCore delivers this on the game thread after the source command.
    // Keep the delegate read/write-free so a closure cannot reenter delivery.
    for (const FOGWorldEvent& Event : Events)
    {
        if (Event.EventId.IsValid() && Event.WorldTick >= 0 && QuestClosingEventTypes.Contains(Event.EventType))
            PendingQuestClosingTicks.FindOrAdd(Event.EventType).Add(Event.WorldTick);
    }
}

bool UOGNarrativeSessionSubsystem::TickQuestLifecycle(float)
{
    if (bQuestLifecycleForegroundRequested.Exchange(false))
        RequestQuestLifecycleCatchup();

    const UGameInstance* Game = GetGameInstance();
    const auto* Core = BoundCore.Get();
    const auto* Ruler = Game ? Game->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() ||
        !Ruler || !Ruler->IsBoundToStoredRuler() || !Ruler->BoundRulerId.IsValid())
    {
        RequestQuestLifecycleCatchup();
        return true;
    }
    if (Store->IsWriteTransactionActive()) return true;

    if (QuestLifecycleRuler != Ruler->BoundRulerId)
    {
        QuestLifecycleRuler = Ruler->BoundRulerId;
        RequestQuestLifecycleCatchup();
    }
    auto* Library = UOGDefinitionLibrary::Get();
    const int64 Revision = Library ? Library->GetRevision() : -1;
    const bool bReloadCatalog = !bQuestLifecycleCatalogLoaded || Revision != QuestLifecycleCatalogRevision;
    if (!bReloadCatalog && !bQuestLifecycleNeedsCatchup && PendingQuestClosingTicks.IsEmpty())
        return true;

    const double NowSeconds = FPlatformTime::Seconds();
    if (NowSeconds < QuestLifecycleRetryAfter) return true;
    const int64 Tick = Core->GetCanonicalWorldTick();
    if (Tick < 0) return true;

    FString Error;
    if (bReloadCatalog)
    {
        FOGNarrativeCatalog Catalog;
        if (!FOGNarrativeCatalog::LoadInstalled(Catalog, Error))
        {
            UE_LOG(LogTemp, Warning, TEXT("Quest lifecycle catalog could not load: %s"), *Error);
            QuestLifecycleRetryAfter = NowSeconds + 5.0;
            return true;
        }

        ClosingQuestDefinitions.Reset();
        QuestClosingEventTypes.Reset();
        for (FOGNarrativeQuestDefinition& Quest : Catalog.Quests)
        {
            if (Quest.ExpirationEvent.EventType.IsNone()) continue;
            QuestClosingEventTypes.Add(Quest.ExpirationEvent.EventType);
            ClosingQuestDefinitions.Add(MoveTemp(Quest));
        }
        for (auto It = PendingQuestClosingTicks.CreateIterator(); It; ++It)
            if (!QuestClosingEventTypes.Contains(It.Key())) It.RemoveCurrent();
        QuestLifecycleCatalogRevision = Library ? Library->GetRevision() : -1;
        bQuestLifecycleCatalogLoaded = true;
        bQuestLifecycleNeedsCatchup = true;
    }

    TSet<FName> ReadyEventTypes;
    for (const auto& Pending : PendingQuestClosingTicks)
    {
        for (const int64 EventTick : Pending.Value)
        {
            if (EventTick > Tick) continue;
            ReadyEventTypes.Add(Pending.Key);
            break;
        }
    }
    if (!bQuestLifecycleNeedsCatchup && ReadyEventTypes.IsEmpty()) return true;

    TSet<FName> RetryEventTypes;
    FOGNarrativeService Narrative(*Store);
    for (const FOGNarrativeQuestDefinition& Quest : ClosingQuestDefinitions)
    {
        const FName EventType = Quest.ExpirationEvent.EventType;
        if (!bQuestLifecycleNeedsCatchup && !ReadyEventTypes.Contains(EventType)) continue;

        FOGNarrativeQuestState State;
        bool bMissed = false;
        if (!Narrative.ReconcileQuestLifecycle(Quest, QuestLifecycleRuler, Tick, State, bMissed, Error))
        {
            UE_LOG(LogTemp, Warning, TEXT("Quest lifecycle could not reconcile %s: %s"), *Quest.QuestId.ToString(), *Error);
            RetryEventTypes.Add(EventType);
        }
    }

    bQuestLifecycleNeedsCatchup = false;
    for (auto It = PendingQuestClosingTicks.CreateIterator(); It; ++It)
    {
        for (auto PendingTick = It.Value().CreateIterator(); PendingTick; ++PendingTick)
            if (*PendingTick <= Tick) PendingTick.RemoveCurrent();
        if (It.Value().IsEmpty()) It.RemoveCurrent();
    }
    for (const FName EventType : RetryEventTypes)
        PendingQuestClosingTicks.FindOrAdd(EventType).Add(Tick);
    QuestLifecycleRetryAfter = RetryEventTypes.IsEmpty() ? 0.0 : NowSeconds + 5.0;
    return true;
}
