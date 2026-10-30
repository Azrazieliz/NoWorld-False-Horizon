#if WITH_DEV_AUTOMATION_TESTS
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Animation/OGSharedMotionSceneComponent.h"
#include "Interaction/OGSystemicInteractionRuntime.h"
#include "Misc/AutomationTest.h"

struct FOGSharedSceneLifecycleTestAccess
{
    static void Start(UOGSharedMotionSceneComponent& Scene,
        UOGSharedMotionControllerComponent& A,UOGSharedMotionControllerComponent& B)
    {
        Scene.ActiveSceneAction=FName(TEXT("test.scene"));
        Scene.bScenePlaying=true;
        for (auto* Controller:{&A,&B})
        {
            Controller->bPlaying=true;
            Controller->CurrentAction=Scene.ActiveSceneAction;
            Controller->CurrentMotionId=Controller==&A?FName(TEXT("test.short")):FName(TEXT("test.long"));
            Scene.ActiveControllers.Add(Controller);
            Scene.ActiveGenerations.Add(Controller,Controller->PlaybackGeneration);
            Scene.ActiveSources.Add(Controller,Controller->CurrentMotionId);
            Controller->OnNativeMotionFinished.AddUObject(&Scene,&UOGSharedMotionSceneComponent::HandleSourceFinished);
        }
    }
    static void Finish(UOGSharedMotionControllerComponent& Controller)
    {
        const FName Action=Controller.CurrentAction,Source=Controller.CurrentMotionId;
        Controller.StopAction();
        Controller.OnNativeMotionFinished.Broadcast(&Controller,Action,Source);
    }
    static void Replace(UOGSharedMotionControllerComponent& Controller)
    {
        Controller.StopAction(); Controller.bPlaying=true;
        Controller.CurrentAction=FName(TEXT("test.foreign")); ++Controller.PlaybackGeneration;
    }
    static void Tick(UOGSharedMotionSceneComponent& Scene)
    { Scene.TickComponent(.016f,LEVELTICK_All,nullptr); }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedSceneDifferentDurations,
    "OfflineGame.Animation.SharedScene.DifferentOriginalDurations",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedSceneDifferentDurations::RunTest(const FString&)
{
    // Lifecycle-only fixtures exercise ownership; they do not stand in for source assets.
    auto* Scene=NewObject<UOGSharedMotionSceneComponent>();
    auto* A=NewObject<UOGSharedMotionControllerComponent>();
    auto* B=NewObject<UOGSharedMotionControllerComponent>();
    FOGSharedSceneLifecycleTestAccess::Start(*Scene,*A,*B);
    const uint64 BeforeB=B->GetPlaybackGeneration();
    FOGSharedSceneLifecycleTestAccess::Finish(*A);
    const uint64 FinishedA=A->GetPlaybackGeneration();
    FOGSharedSceneLifecycleTestAccess::Tick(*Scene);
    TestTrue(TEXT("Longer source role continues after shorter natural finish"),Scene->IsScenePlaying() && B->IsPlayingAction());
    TestTrue(TEXT("Longer role is untouched"),BeforeB==B->GetPlaybackGeneration());
    FOGSharedSceneLifecycleTestAccess::Finish(*B);
    const uint64 FinishedB=B->GetPlaybackGeneration();
    FOGSharedSceneLifecycleTestAccess::Tick(*Scene);
    TestFalse(TEXT("Scene releases after every original role finishes"),Scene->IsScenePlaying());
    TestTrue(TEXT("Natural finish does not receive a second stop generation"),
        FinishedA==A->GetPlaybackGeneration() && FinishedB==B->GetPlaybackGeneration());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedSceneInterruptionOwnership,
    "OfflineGame.Animation.SharedScene.InterruptionOwnership",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedSceneInterruptionOwnership::RunTest(const FString&)
{
    auto* Scene=NewObject<UOGSharedMotionSceneComponent>();
    auto* A=NewObject<UOGSharedMotionControllerComponent>();
    auto* B=NewObject<UOGSharedMotionControllerComponent>();
    FOGSharedSceneLifecycleTestAccess::Start(*Scene,*A,*B);
    A->StopAction(); // An interruption has no natural-finish signal.
    FOGSharedSceneLifecycleTestAccess::Tick(*Scene);
    TestFalse(TEXT("Interrupted scene ends"),Scene->IsScenePlaying());
    TestFalse(TEXT("Remaining owned role stops"),B->IsPlayingAction());
    FOGSharedSceneLifecycleTestAccess::Start(*Scene,*A,*B);
    FOGSharedSceneLifecycleTestAccess::Finish(*A);
    FOGSharedSceneLifecycleTestAccess::Replace(*A);
    const uint64 Foreign=A->GetPlaybackGeneration();
    FOGSharedSceneLifecycleTestAccess::Tick(*Scene);
    TestFalse(TEXT("A new action invalidates even a naturally finished scene role"),Scene->IsScenePlaying());
    TestTrue(TEXT("A later foreign action survives scene release"),
        A->IsPlayingAction() && A->GetPlaybackGeneration()==Foreign && A->CurrentAction==FName(TEXT("test.foreign")));
    A->StopAction();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedExpressionOnlySelector,
    "OfflineGame.Interaction.Animation.ExpressionOnlySharedSelector",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedExpressionOnlySelector::RunTest(const FString&)
{
    FOGInteractionActionNode Node;
    FOGInteractionAnimationBinding Binding; Binding.Role=FName(TEXT("test.role"));
    FOGSharedMorphTrack Track; Track.Layer=FName(TEXT("test.expression")); Track.Morph=FName(TEXT("test.smile"));
    Binding.ExpressionTracks.Add(Track); Node.AnimationBindings.Add(Binding);
    FOGInteractionParticipantState Participant; Participant.Binding.Role=Binding.Role;
    Participant.Binding.RigFamilyId=FOGContentId(TEXT("test:rig.actual"));
    const FOGInteractionAnimationBinding* Resolved=nullptr; FString Error;
    TestTrue(TEXT("Expression-only shared selector resolves"),Node.ResolveAnimationBinding(Participant,Resolved,Error));
    TestTrue(TEXT("Expression track remains selected without a body or scene motion"),Resolved==&Node.AnimationBindings[0]);
    Participant.Binding.Role=FName(TEXT("test.other"));
    TestTrue(TEXT("Other role has no matching selector"),Node.ResolveAnimationBinding(Participant,Resolved,Error) && Resolved==nullptr);
    return true;
}
#endif
