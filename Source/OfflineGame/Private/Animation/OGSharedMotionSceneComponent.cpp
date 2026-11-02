#include "Animation/OGSharedMotionSceneComponent.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Engine/World.h"

UOGSharedMotionSceneComponent::UOGSharedMotionSceneComponent()
{
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.bStartWithTickEnabled=false;
}
bool UOGSharedMotionSceneComponent::PrepareScene(FName Action,FName Context,bool Authorized,
    TArray<UOGSharedMotionControllerComponent*>& Ready,FString& Error,bool Staged) const
{
    Ready.Reset(); Error.Reset(); const int32 Count=Participants.Num();
    if (!GetWorld() || Action.IsNone() || Context.IsNone() || Count<2)
    { Error=TEXT("A shared scene requires its source action/context and actual participant tracks."); return false; }
    TSet<UOGSharedMotionControllerComponent*> Controllers; TSet<FName> Roles; FName SceneId;
    for (const auto& Participant:Participants)
    {
        auto* Controller=Participant.Controller.Get(); const FName Role=Participant.SourceRole;
        if (!IsValid(Controller) || !Controller->GetOwner() || Role.IsNone() || Controller->GetWorld()!=GetWorld() ||
            Controllers.Contains(Controller) || Roles.Contains(Role))
        { Error=TEXT("Shared scene roles require distinct actual controllers and source roles in this World."); return false; }
        Controllers.Add(Controller); Roles.Add(Role); FName Source,Scene;
        const bool Compatible=Staged
            ? Controller->PreviewStagedSource(Action,Context,Authorized,Count,Role,Source,Scene)
            : (Controller->CanPlayAction(Action,Context,Authorized,Count,Role) &&
                Controller->PreviewSourceClipId(Action,Context,Authorized,Count,Role,Source) &&
                Controller->PreviewAuthoredSceneId(Action,Context,Authorized,Count,Role,Scene));
        if (!Compatible || Source.IsNone() || Scene.IsNone() || (!SceneId.IsNone() && SceneId!=Scene))
        { Error=TEXT("An original scene track is missing or incompatible with the actual role/rig."); return false; }
        SceneId=Scene; Ready.Add(Controller);
    }
    return true;
}
bool UOGSharedMotionSceneComponent::CanStartSynchronizedScene(FName Action,FName Context,bool Authorized,FString& Error,bool Staged) const
{ TArray<UOGSharedMotionControllerComponent*> Ready; return PrepareScene(Action,Context,Authorized,Ready,Error,Staged); }
bool UOGSharedMotionSceneComponent::StartSynchronizedScene(FName Action,FName Context,bool Authorized)
{
    TArray<UOGSharedMotionControllerComponent*> Ready;
    if (!PrepareScene(Action,Context,Authorized,Ready,Status)) return false;
    StopSynchronizedScene(); ActiveSceneAction=Action; const float Start=GetWorld()->GetTimeSeconds();
    for (int32 I=0;I<Ready.Num();++I)
    {
        auto* Controller=Ready[I];
        if (!Controller->PlayAction(Action,Context,Authorized,Ready.Num(),Participants[I].SourceRole))
        {
            // Release only tracks this attempt actually started, never untouched participants' actions.
            Status=TEXT("A prepared source scene track could not start."); StopSynchronizedScene(); return false;
        }
        Controller->SynchronizeToWorldTime(Start);
        ActiveControllers.Add(Controller); ActiveGenerations.Add(Controller,Controller->GetPlaybackGeneration());
        ActiveSources.Add(Controller,Controller->CurrentMotionId);
        Controller->OnNativeMotionFinished.AddUObject(this,&ThisClass::HandleSourceFinished);
    }
    Status.Reset(); bScenePlaying=true; SetComponentTickEnabled(true); return true;
}
void UOGSharedMotionSceneComponent::StopSynchronizedScene()
{
    for (const auto& Ref:ActiveControllers)
        if (auto* Controller=Ref.Get())
        {
            Controller->OnNativeMotionFinished.RemoveAll(this);
            if (const auto* Generation=ActiveGenerations.Find(Ref); Generation &&
                Controller->GetPlaybackGeneration()==*Generation && Controller->IsPlayingAction())
                Controller->StopAction();
        }
    ActiveControllers.Reset(); ActiveGenerations.Reset(); ActiveSources.Reset(); FinishedControllers.Reset();
    ActiveSceneAction=NAME_None; bScenePlaying=false; SetComponentTickEnabled(false);
}
void UOGSharedMotionSceneComponent::HandleSourceFinished(UOGSharedMotionControllerComponent* Controller,FName Action,FName Source)
{
    if (!bScenePlaying || !Controller || Action!=ActiveSceneAction) return;
    const auto* Generation=ActiveGenerations.Find(Controller); const auto* ExpectedSource=ActiveSources.Find(Controller);
    if (!Generation || !ExpectedSource || *ExpectedSource!=Source || Controller->IsPlayingAction() ||
        Controller->GetPlaybackGeneration()!=*Generation+1) return;
    // Genuine shorter source tracks may finish while the other roles continue.
    FinishedControllers.Add(Controller); ActiveGenerations.Add(Controller,Controller->GetPlaybackGeneration());
}
void UOGSharedMotionSceneComponent::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Tick,Function); if (!bScenePlaying) return;
    for (const auto& Ref:ActiveControllers)
    {
        auto* Controller=Ref.Get(); const auto* Generation=ActiveGenerations.Find(Ref);
        if (!Controller || !Generation || Controller->GetPlaybackGeneration()!=*Generation ||
            (FinishedControllers.Contains(Ref)?Controller->IsPlayingAction():!Controller->IsPlayingAction()))
        { StopSynchronizedScene(); return; }
    }
    if (FinishedControllers.Num()==ActiveControllers.Num()) StopSynchronizedScene();
}
void UOGSharedMotionSceneComponent::EndPlay(const EEndPlayReason::Type Reason)
{ StopSynchronizedScene(); Super::EndPlay(Reason); }
