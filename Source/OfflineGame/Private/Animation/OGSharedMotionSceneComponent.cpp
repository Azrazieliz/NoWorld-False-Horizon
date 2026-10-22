#include "Animation/OGSharedMotionSceneComponent.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Engine/World.h"

UOGSharedMotionSceneComponent::UOGSharedMotionSceneComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UOGSharedMotionSceneComponent::StartSynchronizedScene(
    FName Action, FName Context, bool bAuthorizedAdultContext)
{
    const int32 Count = Participants.Num();
    if (!GetWorld() || Action.IsNone() || Context.IsNone() ||
        Count < 2 || Count > 13) return false;
    // A failed preflight must not interrupt a currently valid scene.
    TSet<UOGSharedMotionControllerComponent*> UniqueControllers;
    TSet<FName> UniqueRoles;
    TArray<UOGSharedMotionControllerComponent*> Ready;
    Ready.Reserve(Count);
    for (const FOGSharedMotionParticipant& Participant : Participants)
    {
        UOGSharedMotionControllerComponent* Controller = Participant.Controller;
        const FName Role = Participant.SourceRole;
        if (!IsValid(Controller) || !Controller->GetOwner() || Role.IsNone() ||
            Controller->GetWorld() != GetWorld() ||
            UniqueControllers.Contains(Controller) || UniqueRoles.Contains(Role))
            return false;
        UniqueControllers.Add(Controller);
        UniqueRoles.Add(Role);
        FName Source = NAME_None;
        // Each original participant animation may have a DISTINCT Unity
        // source clip ID. The canonical scene action, context and role form
        // their common scene identity; never demand one clip for all bodies.
        if (!Controller->PreviewSourceClipId(Action, Context,
                bAuthorizedAdultContext, Count, Role, Source) || Source.IsNone())
            return false;
        Ready.Add(Controller);
    }
    // Each participant resolves its own authentic source clip for the same
    // authored scene and its unique role; one clock removes startup drift.
    if (bScenePlaying) StopSynchronizedScene();
    const float StartTime = GetWorld()->GetTimeSeconds();
    for (int32 I = 0; I < Ready.Num(); ++I)
    {
        UOGSharedMotionControllerComponent* Controller = Ready[I];
        if (!Controller->PlayAction(Action, Context,
                bAuthorizedAdultContext, Count, Participants[I].SourceRole))
        {
            for (UOGSharedMotionControllerComponent* Started : Ready)
                if (IsValid(Started) && Started->IsPlayingAction())
                    Started->StopAction();
            return false;
        }
        Controller->SynchronizeToWorldTime(StartTime);
        ActiveControllers.Add(Controller);
    }
    ActiveSceneAction = Action;
    bScenePlaying = true;
    SetComponentTickEnabled(true);
    return true;
}

void UOGSharedMotionSceneComponent::StopSynchronizedScene()
{
    for (const TWeakObjectPtr<UOGSharedMotionControllerComponent>& Participant :
         ActiveControllers)
    {
        if (UOGSharedMotionControllerComponent* Controller = Participant.Get())
            if (Controller->CurrentAction == ActiveSceneAction)
                Controller->StopAction();
    }
    ActiveControllers.Reset();
    ActiveSceneAction = NAME_None;
    bScenePlaying = false;
    SetComponentTickEnabled(false);
}

void UOGSharedMotionSceneComponent::TickComponent(
    float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    if (!bScenePlaying) return;
    bool bAnyStopped = false;
    for (const TWeakObjectPtr<UOGSharedMotionControllerComponent>& Participant :
         ActiveControllers)
        if (!Participant.IsValid() || !Participant->IsPlayingAction())
        { bAnyStopped = true; break; }
    if (bAnyStopped) StopSynchronizedScene();
}

void UOGSharedMotionSceneComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    StopSynchronizedScene();
    Super::EndPlay(EndPlayReason);
}
