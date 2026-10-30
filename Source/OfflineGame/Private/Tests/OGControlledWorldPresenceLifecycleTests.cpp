#if WITH_DEV_AUTOMATION_TESTS
#include "World/OGWorldSimulationReceiver.h"
#include "Components/SceneComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

struct FOGControlledPresenceTestAccess
{
    static void AcceptForBoundary(UOGWorldSimulationReceiver& Body, UOGWorldSimulationDefinition* Definition,
        const FOGEntityId& Entity, AController* Controller, UOGWorldSimulationLocationComponent* Frame)
    {
        Body.Prepared = Definition; Body.BoundContext.EntityId = Entity;
        Body.bActive = true; Body.bHasAccepted = true; Body.bRestoreRequired = false;
        Body.ControlledController = Controller; Body.LocationFrame = Frame;
        Body.AcceptedFrameTransform = Frame ? Frame->GetComponentTransform() : FTransform::Identity;
    }
    static void ControllerChanged(UOGWorldSimulationReceiver& Body, APawn* Pawn, AController* Previous)
    { Body.HandleControllerChanged(Pawn, Previous, Pawn->GetController()); }
    static void ForeignMoveCompleted(UOGWorldSimulationReceiver& Body)
    { Body.HandleControlledMoveCompleted(FAIRequestID(9), FPathFollowingResult(EPathFollowingResult::Success)); }
    static bool HasAccepted(const UOGWorldSimulationReceiver& Body) { return Body.bHasAccepted; }
    static bool NeedsRestore(const UOGWorldSimulationReceiver& Body) { return Body.bRestoreRequired; }
    static bool HasController(const UOGWorldSimulationReceiver& Body) { return Body.ControlledController.IsValid(); }
    static bool HasFrame(const UOGWorldSimulationReceiver& Body) { return Body.LocationFrame.IsValid(); }
    static void Refresh(UOGWorldSimulationReceiver& Body) { Body.RefreshControlled(); }
    static void MarkWaiting(UOGWorldSimulationReceiver& Body) { Body.bWaitingForNativePrerequisites = true; }
};

namespace
{
struct FControlledBodyWorld
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ~FControlledBodyWorld() { if (World) World->DestroyWorld(false); }
    APawn* MakeBody()
    {
        if (!World) return nullptr;
        auto* Pawn = World->SpawnActor<APawn>();
        if (!Pawn) return nullptr;
        auto* Root = NewObject<USceneComponent>(Pawn);
        Pawn->SetRootComponent(Root); Pawn->AddInstanceComponent(Root); Root->RegisterComponent();
        return Pawn;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGControlledPresenceSingleWriterTest,
    "OfflineGame.World.NativeBody.ControlledPresence.SingleWriterHandoff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGControlledPresenceSingleWriterTest::RunTest(const FString&)
{
    FControlledBodyWorld F; auto* First = F.MakeBody(); auto* Second = F.MakeBody();
    if (!First || !Second) return false;
    auto* Registry = NewObject<UOGWorldSimulationBodyRegistry>(F.World);
    auto* Old = NewObject<UOGWorldSimulationReceiver>(First);
    auto* Staged = NewObject<UOGWorldSimulationReceiver>(First);
    auto* Foreign = NewObject<UOGWorldSimulationReceiver>(Second);
    const auto Entity = FOGEntityId::NewId();
    TestTrue(TEXT("First physical receiver claims entity"), Registry->Claim(Entity, Old));
    TestTrue(TEXT("Same-body replacement can prepare"), Registry->CanClaim(Entity, Staged));
    TestFalse(TEXT("Preparation cannot become a second physical writer"), Registry->Claim(Entity, Staged));
    TestFalse(TEXT("Another streamed body cannot prepare a duplicate"), Registry->CanClaim(Entity, Foreign));
    Registry->Release(Entity, Foreign);
    TestTrue(TEXT("Foreign release cannot steal old claim"), Registry->Owns(Entity, Old));
    Registry->Release(Entity, Old);
    TestTrue(TEXT("Replacement can claim only after old writer releases"), Registry->Claim(Entity, Staged));
    Registry->Release(Entity, Old);
    TestTrue(TEXT("Late old release cannot remove replacement"), Registry->Owns(Entity, Staged));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGControlledPresenceBoundaryInvalidationTest,
    "OfflineGame.World.NativeBody.ControlledPresence.ControllerAndFrameBoundariesRetainStagedPose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGControlledPresenceBoundaryInvalidationTest::RunTest(const FString&)
{
    FControlledBodyWorld F; auto* Pawn = F.MakeBody();
    if (!Pawn) return false;
    auto* Registry = NewObject<UOGWorldSimulationBodyRegistry>(F.World);
    auto* Body = NewObject<UOGWorldSimulationReceiver>(Pawn);
    auto* Previous = F.World->SpawnActor<APlayerController>();
    auto* FrameActor = F.World->SpawnActor<AActor>();
    if (!Previous || !FrameActor) return false;
    auto* Frame = NewObject<UOGWorldSimulationLocationComponent>(FrameActor);
    FrameActor->SetRootComponent(Frame); FrameActor->AddInstanceComponent(Frame); Frame->RegisterComponent();
    TStrongObjectPtr<UOGWorldSimulationDefinition> Definition(NewObject<UOGWorldSimulationDefinition>());
    Definition->bControlledPresence = true;
    const auto Entity = FOGEntityId::NewId();
    const FVector StagedPose(17,29,43);
    Pawn->SetActorLocation(StagedPose);
    FOGControlledPresenceTestAccess::AcceptForBoundary(*Body, Definition.Get(), Entity, Previous, Frame);
    FOGControlledPresenceTestAccess::ControllerChanged(*Body, Pawn, Previous);
    TestFalse(TEXT("Departing controller invalidates accepted writer even when Core is unavailable"),
        FOGControlledPresenceTestAccess::HasAccepted(*Body));
    TestFalse(TEXT("Old physical controller fence is removed"), FOGControlledPresenceTestAccess::HasController(*Body));
    TestTrue(TEXT("Next local writer must restore canonical state"), FOGControlledPresenceTestAccess::NeedsRestore(*Body));
    TestEqual(TEXT("Controller boundary never teleports a staged pose"), Pawn->GetActorLocation(), StagedPose);
    FOGControlledPresenceTestAccess::AcceptForBoundary(*Body, Definition.Get(), Entity, Previous, Frame);
    if (!Registry->Claim(Entity, Body)) return false;
    Registry->HandleLocationUnregistering(Frame);
    TestFalse(TEXT("Frame removal invalidates accepted physical sample"), FOGControlledPresenceTestAccess::HasAccepted(*Body));
    TestFalse(TEXT("Old streamed frame is not retained"), FOGControlledPresenceTestAccess::HasFrame(*Body));
    TestTrue(TEXT("Streaming requires canonical rehydration"), FOGControlledPresenceTestAccess::NeedsRestore(*Body));
    FOGControlledPresenceTestAccess::Refresh(*Body);
    TestEqual(TEXT("Unavailable canonical runtime cannot teleport staged pose"), Pawn->GetActorLocation(), StagedPose);
    FOGControlledPresenceTestAccess::AcceptForBoundary(*Body, Definition.Get(), Entity, Previous, Frame);
    FOGControlledPresenceTestAccess::ForeignMoveCompleted(*Body);
    TestFalse(TEXT("A completed foreign path cannot donate its final pose between refreshes"),
        FOGControlledPresenceTestAccess::HasAccepted(*Body));
    TestEqual(TEXT("Foreign completion does not teleport the physical pose"), Pawn->GetActorLocation(), StagedPose);
    Registry->Release(Entity, Body);
    Body->ReleaseDefinition();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGControlledPresenceReadinessResetTest,
    "OfflineGame.World.NativeBody.ControlledPresence.NativeReadinessDoesNotMaskInvalidSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGControlledPresenceReadinessResetTest::RunTest(const FString&)
{
    FControlledBodyWorld F; auto* Pawn = F.MakeBody();
    if (!Pawn) return false;
    auto* Body = NewObject<UOGWorldSimulationReceiver>(Pawn);
    TStrongObjectPtr<UOGWorldSimulationDefinition> InvalidSource(NewObject<UOGWorldSimulationDefinition>());
    FOGDefinitionBindingContext Context; Context.EntityId = FOGEntityId::NewId();
    FOGControlledPresenceTestAccess::MarkWaiting(*Body);
    FString Error;
    TestFalse(TEXT("Malformed source cannot prepare"), Body->PrepareDefinition(InvalidSource.Get(), Context, Error));
    TestFalse(TEXT("Fresh prepare clears old native readiness on source failure"), Body->IsWaitingForNativePrerequisites());
    FOGControlledPresenceTestAccess::MarkWaiting(*Body);
    Body->ReleaseDefinition();
    TestFalse(TEXT("Cleanup resets transient readiness after parent captured it"), Body->IsWaitingForNativePrerequisites());
    return true;
}
#endif
