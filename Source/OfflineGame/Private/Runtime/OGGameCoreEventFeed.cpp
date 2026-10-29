#include "Runtime/OGGameCoreSubsystem.h"
#include "Async/Async.h"

void UOGGameCoreSubsystem::BindWorldEventFeed()
{
    if (!bCoreReady || bReleasingCanonicalRuntime || !WorldStore || !WorldStore->IsOpen()) return;
    const auto Feed=WorldStore->GetCommittedEventFeed();
    if (Feed==ObservedWorldEventFeed) return;
    if (ObservedWorldEventFeed) ObservedWorldEventFeed->Remove(WorldEventFeedHandle);
    QueuedCommittedWorldEvents.Reset(); ObservedWorldEventFeed=Feed; ++WorldEventFeedGeneration;
    if (Feed) WorldEventFeedHandle=Feed->AddUObject(this,&ThisClass::QueueCommittedWorldEvents,WorldEventFeedGeneration);
}
void UOGGameCoreSubsystem::ReleaseWorldEventFeed()
{
    ++WorldEventFeedGeneration;
    if (ObservedWorldEventFeed) ObservedWorldEventFeed->Remove(WorldEventFeedHandle);
    ObservedWorldEventFeed.Reset(); WorldEventFeedHandle.Reset(); QueuedCommittedWorldEvents.Reset();
    EntityEventFeeds.Reset();
}
void UOGGameCoreSubsystem::QueueCommittedWorldEvents(const TArray<FOGWorldEvent>& Events,uint64 Generation)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UOGGameCoreSubsystem> Weak(this);
        AsyncTask(ENamedThreads::GameThread,[Weak,Batch=Events,Generation]()
        { if (auto* Core=Weak.Get()) Core->QueueCommittedWorldEvents(Batch,Generation); });
        return;
    }
    if (Generation==WorldEventFeedGeneration && bCoreReady && !bReleasingCanonicalRuntime)
        QueuedCommittedWorldEvents.Append(Events);
}
TSharedPtr<FOGWorldStoreCommittedEvents> UOGGameCoreSubsystem::GetEntityEventFeed(const FOGEntityId& Entity)
{
    if (!Entity.IsValid()) return nullptr;
    BindWorldEventFeed();
    auto& Feed=EntityEventFeeds.FindOrAdd(Entity);
    if (!Feed) Feed=MakeShared<FOGWorldStoreCommittedEvents>();
    return Feed;
}
void UOGGameCoreSubsystem::FlushCommittedWorldEvents()
{
    if (bFlushingCommittedWorldEvents || bSubmittingNativeAction || !bCoreReady || bReleasingCanonicalRuntime ||
        !WorldStore || WorldStore->IsWriteTransactionActive() || QueuedCommittedWorldEvents.IsEmpty()) return;
    TGuardValue<bool> Guard(bFlushingCommittedWorldEvents,true);
    const uint64 Generation=WorldEventFeedGeneration;
    auto Batch=MoveTemp(QueuedCommittedWorldEvents); QueuedCommittedWorldEvents.Reset();
    TMap<FOGEntityId,TArray<FOGWorldEvent>> ByEntity;
    for (const auto& Event:Batch)
    {
        TSet<FOGEntityId> Subjects; if (Event.PrimaryEntity.IsValid()) Subjects.Add(Event.PrimaryEntity);
        for (const auto& Id:Event.RelatedEntities) if (Id.IsValid()) Subjects.Add(Id);
        for (const auto& Id:Subjects)
            if (const auto* Feed=EntityEventFeeds.Find(Id); Feed && (*Feed)->IsBound()) ByEntity.FindOrAdd(Id).Add(Event);
    }
    // Readers see the committed Store and the caller's completed in-memory state, outside its submission guard.
    OnCanonicalWorldEventsCommitted.Broadcast(Batch);
    if (!bCoreReady || bReleasingCanonicalRuntime || Generation!=WorldEventFeedGeneration) return;
    TArray<FOGEntityId> Ordered; ByEntity.GetKeys(Ordered);
    Ordered.Sort([](const auto& A,const auto& B){return A.ToString()<B.ToString();});
    for (const auto& Id:Ordered)
    {
        const auto* Current=EntityEventFeeds.Find(Id); TSharedPtr<FOGWorldStoreCommittedEvents> Feed;
        if (Current) Feed=*Current;
        if (Feed) Feed->Broadcast(ByEntity.FindChecked(Id));
        if (!bCoreReady || bReleasingCanonicalRuntime || Generation!=WorldEventFeedGeneration) return;
    }
    for (auto It=EntityEventFeeds.CreateIterator();It;++It) if (!It.Value()->IsBound()) It.RemoveCurrent();
}
