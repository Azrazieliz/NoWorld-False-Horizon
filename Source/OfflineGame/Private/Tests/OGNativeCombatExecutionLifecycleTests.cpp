#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGNativeTurnAgentComponent.h"
#include "Combat/OGWorldAbilityExecutionComponent.h"
#include "AIController.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// Local receiver state only; these tests admit no production character, ability or authority.
struct FOGNativeCombatExecutionTestAccess
{
    static void Seed(UOGWorldAbilityExecutionComponent& R,UOGCanonicalCombatActorBindingComponent& B)
    {
        R.ActiveCast=FOGEntityId::NewId(); R.BattleId=FOGEntityId::NewId();
        R.Root.SourceUnitId=FOGEntityId::NewId(); R.Specification.Steps.AddDefaulted();
        B.BoundBattleId=R.BattleId; B.BoundUnitId=R.Root.SourceUnitId;
    }
    static void LoseMotion(UOGWorldAbilityExecutionComponent& R)
    { R.bTracksSourceMotion=true; R.bSourceMotionFinished=false; R.SourceMotion.Reset(); }
    static bool BodyCurrent(const UOGWorldAbilityExecutionComponent& R) { return R.IsAcceptedBodyCurrent(); }
    static bool FinishWithRig(UOGWorldAbilityExecutionComponent& R,UOGSharedMotionControllerComponent& Motion,FString& Error)
    {
        R.ClearSourceMotionTracking();
        if (!R.CaptureSourceRig(Motion,Error)) return false;
        R.bSourceMotionFinished=true; return true; // Simulate the accepted receipt's natural-finish notification.
    }
    static bool RigReleased(const UOGWorldAbilityExecutionComponent& R)
    { return R.SourceParts.IsEmpty() && !R.SourceMotion.IsValid() && !R.bTracksSourceMotion && !R.bSourceMotionFinished; }
    static void AddProjectile(UOGWorldAbilityExecutionComponent& R,AOGNativeWorldProjectile* P) { R.Projectiles.Add(P); }
    static void Suppress(UOGNativeTurnAgentComponent& A)
    { A.FailedSequence=42; A.ObservedSequence=42; A.FailedAbilityRevision=7; A.ReadyAtSeconds=99; }
    static bool Suppressed(const UOGNativeTurnAgentComponent& A) { return A.FailedSequence==42; }
    static bool Reset(const UOGNativeTurnAgentComponent& A)
    { return A.FailedSequence==-1 && A.ObservedSequence==-1 && A.FailedAbilityRevision==-1 && A.ReadyAtSeconds<0; }
    static bool Human(UOGNativeTurnAgentComponent& A) { return A.ObserveHumanControl(); }
    static void ReleaseRuntime(UOGNativeTurnAgentComponent& A) { A.HandleRuntimeReleasing(); }
    static bool AutoEnabled(const UOGNativeTurnAgentComponent& A) { return A.bAutoBindingEnabled; }
};
namespace
{
USkeletalMesh* ReceiverRigMesh()
{
    auto* Mesh=NewObject<USkeletalMesh>(); auto* Skeleton=NewObject<USkeleton>();
    Mesh->SetSkeleton(Skeleton);
    FReferenceSkeletonModifier Modifier(const_cast<FReferenceSkeleton&>(Mesh->GetRefSkeleton()),Skeleton);
    Modifier.Add(FMeshBoneInfo(FName(TEXT("root")),TEXT("root"),INDEX_NONE),FTransform::Identity);
    return Mesh; // Transient native test geometry; never an admitted original asset.
}
struct FNativeReceiverFixture
{
    UWorld* World=nullptr;
    FNativeReceiverFixture()
    {
        UWorld::InitializationValues V;
        V.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
        World=UWorld::CreateWorld(EWorldType::Game,true,NAME_None,nullptr,true,ERHIFeatureLevel::SM5,&V,false);
    }
    ~FNativeReceiverFixture() { if (World) World->DestroyWorld(true); }
    template<typename T> T* Add(AActor* Actor)
    {
        auto* C=NewObject<T>(Actor); Actor->AddInstanceComponent(C); C->RegisterComponent(); return C;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeCastUnbindCleanupTest,
    "OfflineGame.Combat.NativeExecution.UnbindReleasesPhysicalCastWithoutLiveCore",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeCastUnbindCleanupTest::RunTest(const FString&)
{
    FNativeReceiverFixture F; if (!TestNotNull(TEXT("Native World"),F.World)) return false;
    auto* Actor=F.World->SpawnActor<AActor>();
    auto* Actions=F.Add<UOGWorldActionRuntimeComponent>(Actor);
    auto* Runner=F.Add<UOGWorldAbilityExecutionComponent>(Actor);
    auto* Body=F.Add<UOGCanonicalCombatActorBindingComponent>(Actor);
    FOGNativeCombatExecutionTestAccess::Seed(*Runner,*Body);
    auto* Projectile=F.World->SpawnActor<AOGNativeWorldProjectile>();
    FOGNativeCombatExecutionTestAccess::AddProjectile(*Runner,Projectile);
    FOGWorldActionState Action; Action.ActionId=TEXT("test:local_cast"); Actions->BeginAuthoredAction(Action);
    int32 Ends=0;
    Actions->OnAuthoredActionEnded.AddLambda([&](){++Ends; Runner->CancelActiveCast();});
    Body->UnbindVisual();
    TestFalse(TEXT("Unbind releases the accepted local receipt"),Runner->HasActiveCast());
    TestFalse(TEXT("Traversal/cancel action ends"),Actions->HasActiveAction());
    TestTrue(TEXT("Old physical projectile is destroyed"),Projectile->IsActorBeingDestroyed());
    TestEqual(TEXT("Action-ended reentry ends once"),Ends,1);
    FOGWorldCastPresentationCheckpoint Saved; Runner->CapturePresentationCheckpoint(Saved);
    TestFalse(TEXT("No cancelled receipt is captured"),Saved.CastId.IsValid());
    TestTrue(TEXT("No cancelled projectile is captured"),Saved.Projectiles.IsEmpty());
    Actions->BeginAuthoredAction(Action); Runner->CancelActiveCast();
    TestTrue(TEXT("Duplicate cancellation preserves a subsequent unrelated action"),Actions->HasActiveAction());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeCastStaleSourceMotionTest,
    "OfflineGame.Combat.NativeExecution.StaleSourceMotionCannotCommitPhysicalContact",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeCastStaleSourceMotionTest::RunTest(const FString&)
{
    FNativeReceiverFixture F; if (!TestNotNull(TEXT("Native World"),F.World)) return false;
    auto* Actor=F.World->SpawnActor<AActor>();
    auto* Actions=F.Add<UOGWorldActionRuntimeComponent>(Actor);
    auto* Runner=F.Add<UOGWorldAbilityExecutionComponent>(Actor);
    auto* Body=F.Add<UOGCanonicalCombatActorBindingComponent>(Actor);
    FOGNativeCombatExecutionTestAccess::Seed(*Runner,*Body);
    TestTrue(TEXT("Matching motionless staging retains ownership"),FOGNativeCombatExecutionTestAccess::BodyCurrent(*Runner));
    FOGNativeCombatExecutionTestAccess::LoseMotion(*Runner);
    TestFalse(TEXT("Lost original motion invalidates ownership"),FOGNativeCombatExecutionTestAccess::BodyCurrent(*Runner));
    FOGWorldActionState Action; Action.ActionId=TEXT("test:local_cast"); Actions->BeginAuthoredAction(Action);
    FString Error; const auto Cast=Runner->GetActiveCastId();
    TestFalse(TEXT("Physical callback cannot apply a lost source contact"),Runner->CommitProjectileContact(Cast,0,nullptr,Error));
    TestFalse(TEXT("Interrupted local receipt is released"),Runner->HasActiveCast());
    TestFalse(TEXT("Interrupted traversal state is released"),Actions->HasActiveAction());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeAgentHandoffTest,
    "OfflineGame.Combat.NativeExecution.HumanHandoffAndCanonicalReconnect",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeAgentHandoffTest::RunTest(const FString&)
{
    FNativeReceiverFixture F; if (!TestNotNull(TEXT("Native World"),F.World)) return false;
    auto* Pawn=F.World->SpawnActor<APawn>(); auto* Agent=F.Add<UOGNativeTurnAgentComponent>(Pawn);
    auto* Human=F.World->SpawnActor<APlayerController>(); auto* AI=F.World->SpawnActor<AAIController>();
    if (!TestNotNull(TEXT("Human controller"),Human) || !TestNotNull(TEXT("AI controller"),AI)) return false;
    FOGNativeCombatExecutionTestAccess::Suppress(*Agent); Human->Possess(Pawn);
    TestTrue(TEXT("Human possession prevents AI decisions"),FOGNativeCombatExecutionTestAccess::Human(*Agent));
    TestTrue(TEXT("Handoff clears stale failure/timing"),FOGNativeCombatExecutionTestAccess::Reset(*Agent));
    FOGNativeCombatExecutionTestAccess::Suppress(*Agent);
    TestTrue(TEXT("Same human remains authoritative"),FOGNativeCombatExecutionTestAccess::Human(*Agent));
    TestTrue(TEXT("Unchanged control does not churn failure state"),FOGNativeCombatExecutionTestAccess::Suppressed(*Agent));
    Human->UnPossess(); AI->Possess(Pawn);
    TestFalse(TEXT("Returning AI control is observed"),FOGNativeCombatExecutionTestAccess::Human(*Agent));
    TestTrue(TEXT("Returning AI can retry the same sequence"),FOGNativeCombatExecutionTestAccess::Reset(*Agent));
    FOGNativeCombatExecutionTestAccess::ReleaseRuntime(*Agent);
    TestTrue(TEXT("Automatic bodies reconnect after runtime release"),FOGNativeCombatExecutionTestAccess::AutoEnabled(*Agent));
    Agent->ReleaseBattleUnit(); FOGNativeCombatExecutionTestAccess::ReleaseRuntime(*Agent);
    TestFalse(TEXT("Explicitly released agents stay released"),FOGNativeCombatExecutionTestAccess::AutoEnabled(*Agent));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGNativeCastFinishedRigReplacementTest,
    "OfflineGame.Combat.NativeExecution.NaturalFinishRetainsControllerAndNativeRigIdentity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGNativeCastFinishedRigReplacementTest::RunTest(const FString&)
{
    for (int32 Replacement=0;Replacement<3;++Replacement)
    {
        const FString Case=Replacement==0?TEXT("controller"):Replacement==1?TEXT("native part"):TEXT("secondary mesh");
        FNativeReceiverFixture F; if (!TestNotNull(TEXT("Native World"),F.World)) return false;
        auto* Actor=F.World->SpawnActor<AActor>();
        auto* Actions=F.Add<UOGWorldActionRuntimeComponent>(Actor);
        auto* Runner=F.Add<UOGWorldAbilityExecutionComponent>(Actor);
        auto* Body=F.Add<UOGCanonicalCombatActorBindingComponent>(Actor);
        auto* Motion=F.Add<UOGSharedMotionControllerComponent>(Actor); Motion->RigProfile=TEXT("test:local_rig");
        auto* First=F.Add<USkeletalMeshComponent>(Actor); First->SetSkeletalMeshAsset(ReceiverRigMesh());
        auto* Second=F.Add<USkeletalMeshComponent>(Actor); Second->SetSkeletalMeshAsset(ReceiverRigMesh());
        if (!TestTrue(Case+TEXT(": connect both native parts"),Motion->RegisterNativeParts({First,Second}))) return false;
        FOGNativeCombatExecutionTestAccess::Seed(*Runner,*Body);
        FString Error;
        if (!TestTrue(Case+TEXT(": capture actual rig and natural finish"),
            FOGNativeCombatExecutionTestAccess::FinishWithRig(*Runner,*Motion,Error))) return false;
        const auto Cast=Runner->GetActiveCastId();
        TestTrue(Case+TEXT(": original rig remains valid after natural finish"),
            FOGNativeCombatExecutionTestAccess::BodyCurrent(*Runner));
        const auto Generation=Motion->GetPlaybackGeneration(); Motion->StopAction();
        TestTrue(Case+TEXT(": cosmetic playback changed generation"),Motion->GetPlaybackGeneration()!=Generation);
        TestTrue(Case+TEXT(": finished recovery allows a different cosmetic receipt"),Motion->GetActiveCombatReceipt()!=Cast);
        TestTrue(Case+TEXT(": cosmetic receipt/generation changes preserve recovery"),
            FOGNativeCombatExecutionTestAccess::BodyCurrent(*Runner));
        FOGWorldActionState Action; Action.ActionId=TEXT("test:local_recovery"); Actions->BeginAuthoredAction(Action);
        auto* Projectile=F.World->SpawnActor<AOGNativeWorldProjectile>();
        FOGNativeCombatExecutionTestAccess::AddProjectile(*Runner,Projectile);
        if (Replacement==0)
        {
            Motion->DestroyComponent();
            auto* NewMotion=F.Add<UOGSharedMotionControllerComponent>(Actor); NewMotion->RigProfile=TEXT("test:local_rig");
            if (!TestTrue(TEXT("Replacement controller connects the same native parts"),
                NewMotion->RegisterNativeParts({First,Second}))) return false;
        }
        else if (Replacement==1)
        {
            auto* NewPart=F.Add<USkeletalMeshComponent>(Actor); NewPart->SetSkeletalMeshAsset(First->GetSkeletalMeshAsset());
            if (!TestTrue(TEXT("Replacement part uses the same mesh asset"),
                Motion->RegisterNativeParts({NewPart,Second}))) return false;
        }
        else Second->SetSkeletalMeshAsset(ReceiverRigMesh());
        TestFalse(Case+TEXT(": replacement invalidates finished cast ownership"),
            FOGNativeCombatExecutionTestAccess::BodyCurrent(*Runner));
        TestFalse(Case+TEXT(": old physical callback cannot commit"),
            Runner->CommitProjectileContact(Cast,0,nullptr,Error));
        TestFalse(Case+TEXT(": old cast receipt is released"),Runner->HasActiveCast());
        TestFalse(Case+TEXT(": old recovery traversal ends"),Actions->HasActiveAction());
        TestTrue(Case+TEXT(": old projectile is destroyed"),Projectile->IsActorBeingDestroyed());
        TestTrue(Case+TEXT(": captured rig identities are cleared"),
            FOGNativeCombatExecutionTestAccess::RigReleased(*Runner));
    }
    return true;
}
#endif
