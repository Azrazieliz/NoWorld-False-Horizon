#if WITH_DEV_AUTOMATION_TESTS
#include "Content/OGSystemBindingComponent.h"
#include "Misc/AutomationTest.h"

struct FOGSystemBindingEventTestAccess
{
    static void Observe(UOGSystemBindingComponent& Binding,TSharedPtr<FOGWorldStoreCommittedEvents> Feed)
    {
        Binding.CanonicalEntityId=FOGEntityId::NewId();
        Binding.bAutoRefreshEnabled=true; Binding.CanonicalEntityFeed=MoveTemp(Feed);
        Binding.BoundProfile=FOGContentId(TEXT("test:binding.existing"));
        Binding.EntityFeedHandle=Binding.CanonicalEntityFeed->AddUObject(&Binding,&UOGSystemBindingComponent::HandleEntityCommitted);
    }
    static bool Queued(const UOGSystemBindingComponent& Binding) { return Binding.bRefreshRequested; }
    static bool Enabled(const UOGSystemBindingComponent& Binding) { return Binding.bAutoRefreshEnabled; }
    static uint64 Generation(const UOGSystemBindingComponent& Binding) { return Binding.BindingGeneration; }
    static void Preparing(UOGSystemBindingComponent& Binding,bool Value) { Binding.bBinding=Value; }
    static void LibraryChanged(UOGSystemBindingComponent& Binding) { Binding.HandleLibraryChanged(7); }
    static bool KeepsProfile(const UOGSystemBindingComponent& Binding)
    { return Binding.BoundProfile==FOGContentId(TEXT("test:binding.existing")); }
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSystemBindingDeferredEntityChange,
    "OfflineGame.Content.SystemBinding.CommittedChangesDeferAndInvalidateStaging",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSystemBindingDeferredEntityChange::RunTest(const FString&)
{
    auto* Binding=NewObject<UOGSystemBindingComponent>();
    auto Feed=MakeShared<FOGWorldStoreCommittedEvents>();
    FOGSystemBindingEventTestAccess::Observe(*Binding,Feed);
    Feed->Broadcast({});
    TestFalse(TEXT("Empty batches do not schedule work"),FOGSystemBindingEventTestAccess::Queued(*Binding));
    TArray<FOGWorldEvent> Events; Events.AddDefaulted();
    Feed->Broadcast(Events); Feed->Broadcast(Events);
    TestTrue(TEXT("Committed notifications coalesce into deferred work"),FOGSystemBindingEventTestAccess::Queued(*Binding));
    TestTrue(TEXT("Dispatch preserves the currently installed profile"),FOGSystemBindingEventTestAccess::KeepsProfile(*Binding));
    const uint64 Before=FOGSystemBindingEventTestAccess::Generation(*Binding);
    FOGSystemBindingEventTestAccess::Preparing(*Binding,true); Feed->Broadcast(Events);
    TestTrue(TEXT("A commit during preparation invalidates the staged generation"),
        FOGSystemBindingEventTestAccess::Generation(*Binding)!=Before);
    FOGSystemBindingEventTestAccess::Preparing(*Binding,false);
    Binding->ReleaseBinding();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSystemBindingExplicitRelease,
    "OfflineGame.Content.SystemBinding.ExplicitReleaseDoesNotAutoRebind",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSystemBindingExplicitRelease::RunTest(const FString&)
{
    auto* Binding=NewObject<UOGSystemBindingComponent>();
    auto Feed=MakeShared<FOGWorldStoreCommittedEvents>();
    FOGSystemBindingEventTestAccess::Observe(*Binding,Feed);
    Binding->ReleaseBinding();
    TArray<FOGWorldEvent> Events; Events.AddDefaulted(); Feed->Broadcast(Events);
    FOGSystemBindingEventTestAccess::LibraryChanged(*Binding);
    TestFalse(TEXT("Release disables automatic binding"),FOGSystemBindingEventTestAccess::Enabled(*Binding));
    TestFalse(TEXT("Old feed and new definitions cannot resurrect a released binding"),
        FOGSystemBindingEventTestAccess::Queued(*Binding));
    return true;
}
#endif
