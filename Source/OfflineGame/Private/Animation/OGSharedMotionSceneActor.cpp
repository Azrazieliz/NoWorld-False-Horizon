#include "Animation/OGSharedMotionSceneActor.h"
#include "Animation/OGSharedMotionSceneComponent.h"
#include "Components/SceneComponent.h"

AOGSharedMotionSceneActor::AOGSharedMotionSceneActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("NativeSceneRoot"));
    RootComponent = SceneRoot;
    Scene = CreateDefaultSubobject<UOGSharedMotionSceneComponent>(TEXT("NativeMotionScene"));
}
bool AOGSharedMotionSceneActor::PlayNativeScene(
    FName Action, FName Context, bool bAdultAuthorized)
{
    return Scene && Scene->StartSynchronizedScene(Action, Context, bAdultAuthorized);
}
void AOGSharedMotionSceneActor::CancelNativeScene()
{
    if (Scene) Scene->StopSynchronizedScene();
}
