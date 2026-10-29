#include "Combat/OGWorldAbilityExecutionComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGNativeBattleCommands.h"
#include "Combat/OGTurnBattle.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"

namespace
{
UOGSharedMotionControllerComponent* WorldMotion(AActor* Actor)
{
    if (!Actor) return nullptr;
    if (auto* Appearance=Actor->FindComponentByClass<UOGCharacterAppearanceComponent>())
        if (auto* Motion=Appearance->GetMotionController()) return Motion;
    return Actor->FindComponentByClass<UOGSharedMotionControllerComponent>();
}
FOGEntityId BodyId(AActor* Actor,const FOGEntityId& Battle)
{
    const auto* Binding=Actor?Actor->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>():nullptr;
    return Binding && Binding->GetBoundBattleId()==Battle?Binding->GetBoundUnitId():FOGEntityId();
}
bool VisibleBetween(UWorld* World,AActor* Source,AActor* Target,const FVector& From,const FVector& To,ECollisionChannel Channel)
{
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(NativeWorldAbilityVisibility),false,Source);
    return !World->LineTraceSingleByChannel(Hit,From,To,Channel,Query) || Hit.GetActor()==Target;
}
}
AOGNativeWorldProjectile::AOGNativeWorldProjectile()
{
    PrimaryActorTick.bCanEverTick=true;
    Collision=CreateDefaultSubobject<USphereComponent>(TEXT("SourceCollision")); RootComponent=Collision;
    Visual=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SourceVisual")); Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Movement=CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("SourceMovement")); Movement->SetUpdatedComponent(Collision);
    Movement->bRotationFollowsVelocity=true; Movement->bShouldBounce=false;
    Collision->OnComponentHit.AddDynamic(this,&ThisClass::OnPhysicalHit);
}
bool AOGNativeWorldProjectile::InitializeAcceptedStep(UOGWorldAbilityExecutionComponent* InSource,
    const FOGEntityId& Cast,int32 Step,const FOGWorldAbilityStep& Spec,const FVector& Direction,FString& Error)
{
    auto* Mesh=Spec.ProjectileMesh.LoadSynchronous();
    if (!InSource || !Cast.IsValid() || Step<0 || !Mesh || Direction.IsNearlyZero() || Spec.ProjectileSpeed<=0)
    { Error=TEXT("Projectile requires its accepted cast and original installed visual/kinematics."); return false; }
    Source=InSource; CastId=Cast; StepIndex=Step;
    LaunchOrigin=GetActorLocation(); MaximumTravelCentimeters=Spec.RangeCentimeters;
    Collision->SetSphereRadius(Spec.RadiusCentimeters); Collision->SetCollisionProfileName(Spec.ProjectileCollisionProfile);
    Collision->SetNotifyRigidBodyCollision(true); Collision->IgnoreActorWhenMoving(InSource->GetOwner(),true);
    Visual->SetStaticMesh(Mesh); Visual->SetRelativeTransform(Spec.ProjectileMeshTransform);
    Movement->InitialSpeed=Spec.ProjectileSpeed; Movement->MaxSpeed=Spec.ProjectileSpeed;
    Movement->ProjectileGravityScale=Spec.ProjectileGravityScale; Movement->Velocity=Direction.GetSafeNormal()*Spec.ProjectileSpeed;
    SetLifeSpan((Spec.ClosesAtMilliseconds-Spec.OpensAtMilliseconds)/1000.0); return true;
}
bool AOGNativeWorldProjectile::CaptureCheckpoint(FOGNativeProjectileCheckpoint& Out) const
{
    if (!Source.IsValid() || !CastId.IsValid() || StepIndex<0 || !Movement || IsActorBeingDestroyed()) return false;
    Out.StepIndex=StepIndex; Out.Transform=GetActorTransform(); Out.Velocity=Movement->Velocity;
    Out.LaunchOrigin=LaunchOrigin; Out.RemainingLifeSeconds=bRecoveryPaused?RecoveryRemainingLife:GetLifeSpan();
    return Out.RemainingLifeSeconds>0;
}
bool AOGNativeWorldProjectile::RestoreCheckpoint(const FOGNativeProjectileCheckpoint& State,FString& Error)
{
    if (State.StepIndex!=StepIndex || State.Transform.ContainsNaN() || State.Velocity.ContainsNaN() ||
        State.LaunchOrigin.ContainsNaN() || !FMath::IsFinite(State.RemainingLifeSeconds) || State.RemainingLifeSeconds<=0)
    { Error=TEXT("Saved projectile geometry or remaining lifetime is invalid."); return false; }
    SetActorTransform(State.Transform,false,nullptr,ETeleportType::TeleportPhysics);
    LaunchOrigin=State.LaunchOrigin; Movement->Velocity=State.Velocity;
    bRecoveryPaused=Source.IsValid() && Source->IsRecoveryPaused(); RecoveryRemainingLife=State.RemainingLifeSeconds;
    Movement->SetComponentTickEnabled(!bRecoveryPaused); SetLifeSpan(bRecoveryPaused?0:RecoveryRemainingLife);
    return true;
}
void AOGNativeWorldProjectile::OnPhysicalHit(UPrimitiveComponent*,AActor* Other,UPrimitiveComponent*,FVector,const FHitResult&)
{
    if (Other==GetOwner() || (Source.IsValid() && Source->IsRecoveryPaused())) return;
    if (FVector::DistSquared(LaunchOrigin,GetActorLocation())>FMath::Square(MaximumTravelCentimeters)) { Destroy(); return; }
    if (Source.IsValid()) { FString Error; Source->CommitProjectileContact(CastId,StepIndex,Other,Error); }
    Destroy();
}
void AOGNativeWorldProjectile::Tick(float Delta)
{
    Super::Tick(Delta);
    if (Source.IsValid() && Source->IsRecoveryPaused()) return;
    if (bRecoveryPaused)
    { bRecoveryPaused=false; Movement->SetComponentTickEnabled(true); SetLifeSpan(RecoveryRemainingLife); }
    if (!Source.IsValid() || Source->GetActiveCastId()!=CastId ||
        FVector::DistSquared(LaunchOrigin,GetActorLocation())>FMath::Square(MaximumTravelCentimeters)) Destroy();
}
UOGWorldAbilityExecutionComponent::UOGWorldAbilityExecutionComponent()
{
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void UOGWorldAbilityExecutionComponent::BeginPlay()
{
    Super::BeginPlay(); FString Error; ResolveRuntime(Error);
    if (auto* Motion=WorldMotion(GetOwner())) AddTickPrerequisiteComponent(Motion);
    if (auto* Actions=GetOwner()?GetOwner()->FindComponentByClass<UOGWorldActionRuntimeComponent>():nullptr)
        Actions->OnAuthoredActionEnded.AddUObject(this,&ThisClass::CancelActiveCast);
}
bool UOGWorldAbilityExecutionComponent::ResolveRuntime(FString& Error)
{
    auto* Game=GetWorld()?GetWorld()->GetGameInstance():nullptr;
    Core=Game?Game->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    const auto* Binding=GetOwner()?GetOwner()->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>():nullptr;
    if (!Core.IsValid() || !Core->IsCoreReady() || !Binding || !Binding->GetBoundUnitId().IsValid() ||
        !Core->GetOwnedLuminousTurnBattle() || Binding->GetBoundBattleId()!=Core->GetOwnedLuminousTurnBattle()->GetState().BattleId)
    { Error=TEXT("World ability runner requires its real bound native combat body."); return false; }
    return true;
}
USceneComponent* UOGWorldAbilityExecutionComponent::TracePart(const FOGWorldAbilityStep& Step,FName& Start,FName& End,FString& Error) const
{
    Start=Step.StartSocket; End=Step.EndSocket.IsNone()?Step.StartSocket:Step.EndSocket; auto* Motion=WorldMotion(GetOwner());
    if (Motion)
    {
        if (const auto* Alias=Motion->SharedSourceBoneAliases.Find(Start)) Start=*Alias;
        if (const auto* Alias=Motion->SharedSourceBoneAliases.Find(End)) End=*Alias;
    }
    TArray<USceneComponent*> Parts; if (GetOwner()) GetOwner()->GetComponents<USceneComponent>(Parts);
    if (!Step.TraceComponentName.IsNone())
    {
        auto Matches=Parts.FilterByPredicate([&](const auto* P){return P && P->GetFName()==Step.TraceComponentName;});
        if (Matches.Num()==1 && Matches[0]->DoesSocketExist(Start) && Matches[0]->DoesSocketExist(End)) return Matches[0];
    }
    else
    {
        if (Motion) for (const auto& Ref:Motion->GetNativeParts())
            if (auto* Part=Ref.Get(); Part && Part->DoesSocketExist(Start) && Part->DoesSocketExist(End)) return Part;
        if (auto* Character=Cast<ACharacter>(GetOwner()))
            if (Character->GetMesh() && Character->GetMesh()->DoesSocketExist(Start) && Character->GetMesh()->DoesSocketExist(End))
                return Character->GetMesh();
    }
    Error=TEXT("Authored physical trace sockets do not exist on the connected actual rig."); return nullptr;
}
bool UOGWorldAbilityExecutionComponent::BeginNativeAction(const FOGResolvedCombatAction& Request,bool FromAI,FString& Error)
{
    Error.Reset(); if (!ResolveRuntime(Error)) return false;
    const auto* Binding=GetOwner()->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
    const auto* Battle=Core->GetOwnedLuminousTurnBattle();
    const auto* Unit=Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Binding->GetBoundUnitId();});
    if (!Unit || Battle->GetState().ExecutionMode!=EOGCombatExecutionMode::World || Request.SourceUnitId!=Unit->UnitEntityId)
    { Error=TEXT("World command source differs from this actual connected actor."); return false; }
    const auto* Pawn=Cast<APawn>(GetOwner());
    const bool Human=Pawn && Cast<APlayerController>(Pawn->GetController());
    const auto* Session=GetWorld()->GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>();
    if ((!FromAI && (!Human || !Session || Core->GetOwnedBattleRulerId()!=Session->BoundRulerId ||
        (Unit->UnitEntityId!=Session->SelectedManifestationId && Unit->UnitEntityId!=Session->BoundRulerId))) || (FromAI && Human))
    { Error=TEXT("World command does not belong to this local player or canonical AI body."); return false; }
    if (HasActiveCast() && Battle->GetUnitWorldCast(Unit->UnitEntityId)!=ActiveCast) EndLocalCast();
    const auto* Spec=Battle->FindWorldAbility(*Unit,Request.SkillId);
    if (!Spec) { Error=TEXT("This skill's original World staging is not installed."); return false; }
    for (const auto& Step:Spec->Steps)
    {
        if (Step.Delivery==EOGWorldAbilityDelivery::MeleeSweep)
        { FName Start,End; if (!TracePart(Step,Start,End,Error)) return false; }
        if (Step.Delivery==EOGWorldAbilityDelivery::Projectile && !Step.ProjectileMesh.LoadSynchronous())
        { Error=TEXT("The original projectile mesh is unavailable."); return false; }
    }
    const FName Semantic=Battle->ResolveNativeAbilityMotion(*Unit,Request.SkillId);
    if (!Semantic.IsNone())
    {
        auto* Motion=WorldMotion(GetOwner());
        if (!Motion || !Motion->CanPlayAction(Semantic,TEXT("Combat"),false,1,NAME_None))
        { Error=TEXT("The original action motion is not connected to this actual rig."); return false; }
    }
    const auto FrozenSpec=*Spec;
    auto* Actions=GetOwner()->FindComponentByClass<UOGWorldActionRuntimeComponent>();
    if (Actions && Actions->HasActiveAction())
    {
        const auto Destination=Unit->SkillSet.IsUltimate(Request.SkillId)?EOGActionCancelDestination::Ultimate:EOGActionCancelDestination::Skill;
        if (!Actions->CanCancelTo(Destination)) { Error=TEXT("Current action has no authored cancel window for this command."); return false; }
        Actions->EndAuthoredAction();
        if (HasActiveCast()) { Error=TEXT("The previous World cast could not be cancelled."); return false; }
    }
    else if (HasActiveCast())
    { Error=TEXT("Finish the active World action before starting another."); return false; }
    Root=Request; if (!Root.ActionId.IsValid()) Root.ActionId=FOGEntityId::NewId();
    if (!Core->BeginOwnedWorldAbility(Root,Error)) return false;
    FOGWorldAbilityCastView View;
    if (!Core->GetOwnedLuminousTurnBattle()->ReadWorldCast(Root.ActionId,View)) return true;
    ActiveCast=Root.ActionId; BattleId=Battle->GetState().BattleId; Specification=FrozenSpec; StartedAt=View.StartedAtMilliseconds;
    Completed.Reset(); SpawnedProjectiles.Reset(); PreviousStarts.Reset(); PreviousEnds.Reset();
    if (Actions) { auto State=Specification.TraversalAndCancel; State.ActionId=FName(*Root.SkillId.ToString()); Actions->BeginAuthoredAction(State); }
    if (!TrackAcceptedMotion(Error))
    {
        // Already committed: cancel through canonical authority, never refund or apply packets locally.
        Status=Error; CancelActiveCast(); return true;
    }
    return true;
}
bool UOGWorldAbilityExecutionComponent::TrackAcceptedMotion(FString& Error)
{
    if (SourceMotion.IsValid()) SourceMotion->OnMotionFinished.RemoveAll(this);
    SourceMotion.Reset(); bSourceMotionFinished=false; bTracksSourceMotion=false;
    auto* Motion=WorldMotion(GetOwner());
    if (Motion && Motion->GetActiveCombatReceipt()==ActiveCast)
    {
        SourceMotion=Motion; bTracksSourceMotion=true;
        Motion->OnMotionFinished.AddUniqueDynamic(this,&ThisClass::HandleSourceMotionFinished);
        AddTickPrerequisiteComponent(Motion);
        // Physical sweeps observe the evaluated source rig, including independently animated parts.
        for (USkeletalMeshComponent* Part:Motion->GetNativeParts()) if (Part) AddTickPrerequisiteComponent(Part);
        return true;
    }
    const auto* Battle=Core.IsValid()?Core->GetOwnedLuminousTurnBattle():nullptr;
    const auto* Unit=Battle?Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Root.SourceUnitId;}):nullptr;
    if (Unit && !Battle->ResolveNativeAbilityMotion(*Unit,Root.SkillId).IsNone())
    { Error=TEXT("Accepted World action has no matching source-motion receipt on its actual body."); return false; }
    return true; // Motionless source staging remains valid.
}
void UOGWorldAbilityExecutionComponent::HandleSourceMotionFinished(FName,FName)
{
    if (SourceMotion.IsValid() && SourceMotion->GetFinishingCombatReceipt()==ActiveCast)
        bSourceMotionFinished=true; // Original finish is distinct from the authored cast recovery interval.
}
bool UOGWorldAbilityExecutionComponent::IsRecoveryPaused() const
{ return Core.IsValid() && Core->IsNativeEncounterAwaitingWorldRebind(); }
void UOGWorldAbilityExecutionComponent::CapturePresentationCheckpoint(FOGWorldCastPresentationCheckpoint& Out) const
{
    Out=FOGWorldCastPresentationCheckpoint(); Out.CastId=ActiveCast; Out.SpawnedSteps=SpawnedProjectiles;
    Out.PreviousStarts=PreviousStarts; Out.PreviousEnds=PreviousEnds;
    for (const auto& Ref:Projectiles) if (const auto* Projectile=Ref.Get())
    { FOGNativeProjectileCheckpoint Saved; if (Projectile->CaptureCheckpoint(Saved)) Out.Projectiles.Add(MoveTemp(Saved)); }
}
bool UOGWorldAbilityExecutionComponent::CanRecoverAcceptedCast(const FOGWorldCastPresentationCheckpoint& Saved,FString& Error)
{
    if (!ResolveRuntime(Error)) return false;
    if (ActiveCast.IsValid())
    { if (ActiveCast==Saved.CastId) return true; Error=TEXT("Another native cast already owns this body's playback."); return false; }
    const auto* Battle=Core->GetOwnedLuminousTurnBattle();
    FOGResolvedCombatAction Request; FOGWorldAbilitySpecification Spec; TSet<int32> Steps; int64 Start=0;
    if (!Battle->ReadWorldCastRecovery(Saved.CastId,Request,Spec,Steps,Start) ||
        BodyId(GetOwner(),Battle->GetState().BattleId)!=Request.SourceUnitId)
    { Error=TEXT("Recovered cast does not belong to this actual native body."); return false; }
    for (const auto& Step:Spec.Steps)
    {
        if (Step.Delivery==EOGWorldAbilityDelivery::MeleeSweep)
        { FName A,B; if (!TracePart(Step,A,B,Error)) return false; }
        if (Step.Delivery==EOGWorldAbilityDelivery::Projectile && !Step.ProjectileMesh.LoadSynchronous())
        { Error=TEXT("Recovered projectile requires its original installed mesh."); return false; }
    }
    TSet<int32> PhysicalSteps;
    for (const auto& Projectile:Saved.Projectiles)
    {
        if (!Spec.Steps.IsValidIndex(Projectile.StepIndex) ||
            Spec.Steps[Projectile.StepIndex].Delivery!=EOGWorldAbilityDelivery::Projectile ||
            !Saved.SpawnedSteps.Contains(Projectile.StepIndex) || PhysicalSteps.Contains(Projectile.StepIndex) ||
            Projectile.Transform.ContainsNaN() || !Projectile.Transform.GetRotation().IsNormalized() ||
            Projectile.Velocity.ContainsNaN() || Projectile.LaunchOrigin.ContainsNaN() ||
            !FMath::IsFinite(Projectile.RemainingLifeSeconds) || Projectile.RemainingLifeSeconds<=0)
        { Error=TEXT("Recovered projectile has an inconsistent original cast/step or physical state."); return false; }
        PhysicalSteps.Add(Projectile.StepIndex);
    }
    for (int32 Index:Saved.SpawnedSteps)
        if (!Spec.Steps.IsValidIndex(Index) || Spec.Steps[Index].Delivery!=EOGWorldAbilityDelivery::Projectile)
        { Error=TEXT("Recovered projectile step differs from the accepted original ability."); return false; }
    const auto* Unit=Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Request.SourceUnitId;});
    const FName Motion=Unit?Battle->ResolveNativeAbilityMotion(*Unit,Request.SkillId):NAME_None;
    auto* Controller=WorldMotion(GetOwner());
    if (!Motion.IsNone() && (!Controller || !Controller->CanPlayAction(Motion,TEXT("Combat"),false,1,NAME_None)))
    { Error=TEXT("Recovered cast waits for its original native motion and rig."); return false; }
    return true;
}
bool UOGWorldAbilityExecutionComponent::RecoverAcceptedCast(const FOGWorldCastPresentationCheckpoint& Saved,FString& Error)
{
    if (!CanRecoverAcceptedCast(Saved,Error)) return false;
    if (ActiveCast==Saved.CastId)
    {
        if (auto* Motion=WorldMotion(GetOwner())) Motion->SetExternalPacing(IsRecoveryPaused()?0.f:1.f);
        if (!IsRecoveryPaused()) if (auto* Actions=GetOwner()->FindComponentByClass<UOGWorldActionRuntimeComponent>())
        {
            auto State=Specification.TraversalAndCancel; State.ActionId=FName(*Root.SkillId.ToString());
            const int64 Now=Core->GetOwnedLuminousTurnBattle()->GetState().WorldElapsedMilliseconds;
            Actions->RestoreAuthoredAction(State,static_cast<float>((Now-StartedAt)/1000.0));
        }
        return true;
    }
    const auto* Battle=Core->GetOwnedLuminousTurnBattle();
    if (!Battle->ReadWorldCastRecovery(Saved.CastId,Root,Specification,Completed,StartedAt)) return false;
    BattleId=Battle->GetState().BattleId; ActiveCast=Saved.CastId; SpawnedProjectiles=Saved.SpawnedSteps;
    PreviousStarts=Saved.PreviousStarts; PreviousEnds=Saved.PreviousEnds;
    for (const auto& State:Saved.Projectiles)
    {
        if (Completed.Contains(State.StepIndex)) continue; // Contact already committed; never recreate its hit.
        const auto& Step=Specification.Steps[State.StepIndex];
        FActorSpawnParameters Params; Params.Owner=GetOwner(); Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Projectile=GetWorld()->SpawnActor<AOGNativeWorldProjectile>(AOGNativeWorldProjectile::StaticClass(),State.Transform,Params);
        FVector Direction=State.Velocity.GetSafeNormal(); if (Direction.IsNearlyZero()) Direction=State.Transform.GetRotation().GetForwardVector();
        if (!Projectile || !Projectile->InitializeAcceptedStep(this,ActiveCast,State.StepIndex,Step,Direction,Error) ||
            !Projectile->RestoreCheckpoint(State,Error))
        {
            if (Projectile) Projectile->Destroy(); EndLocalCast(); return false;
        }
        Projectiles.Add(Projectile);
    }
    if (auto* Actions=GetOwner()->FindComponentByClass<UOGWorldActionRuntimeComponent>())
    {
        auto State=Specification.TraversalAndCancel; State.ActionId=FName(*Root.SkillId.ToString());
        Actions->RestoreAuthoredAction(State,static_cast<float>((Battle->GetState().WorldElapsedMilliseconds-StartedAt)/1000.0));
    }
    const auto* Unit=Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Root.SourceUnitId;});
    const FName Semantic=Unit?Battle->ResolveNativeAbilityMotion(*Unit,Root.SkillId):NAME_None;
    if (!Semantic.IsNone()) if (auto* Motion=WorldMotion(GetOwner()))
    {
        if (!Motion->StartImmediateCombatAction(Semantic,BattleId,ActiveCast)) { EndLocalCast(); return false; }
        const float Elapsed=static_cast<float>((Battle->GetState().WorldElapsedMilliseconds-StartedAt)/1000.0);
        Motion->RestorePlaybackPhase(Elapsed);
        Motion->SetExternalPacing(IsRecoveryPaused()?0.f:1.f);
    }
    if (!TrackAcceptedMotion(Error)) { EndLocalCast(); return false; }
    Status=TEXT("Accepted World cast recovered without repeating costs or committed contacts.");
    return true;
}
void UOGWorldAbilityExecutionComponent::QueryContacts(const FOGWorldAbilityStep& Step,int32 Index,TArray<FOGEntityId>& Out)
{
    Out.Reset(); if (!Core.IsValid() || !GetWorld() || !GetOwner()) return;
    const auto* Battle=Core->GetOwnedLuminousTurnBattle(); if (!Battle) return;
    const FVector Origin=GetOwner()->GetActorLocation();
    if (Step.Delivery==EOGWorldAbilityDelivery::Direct)
    {
        for (const auto& Unit:Battle->GetState().Units)
        {
            auto* Actor=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),BattleId,Unit.UnitEntityId);
            if (!Actor || !Unit.CanAct()) continue;
            if (Actor!=GetOwner() && (Step.RangeCentimeters<=0 || FVector::DistSquared(Origin,Actor->GetActorLocation())>FMath::Square(Step.RangeCentimeters))) continue;
            if (Step.bRequireLineOfSight && Actor!=GetOwner() && !VisibleBetween(GetWorld(),GetOwner(),Actor,Origin,Actor->GetActorLocation(),Step.VisibilityChannel)) continue;
            Out.Add(Unit.UnitEntityId);
        }
    }
    else
    {
        TArray<FHitResult> Hits; FCollisionQueryParams Query(SCENE_QUERY_STAT(NativeWorldAbilityContact),false,GetOwner());
        if (Step.Delivery==EOGWorldAbilityDelivery::Sphere)
        {
            FVector Center=Origin;
            if (!Root.TargetUnitIds.IsEmpty())
                if (auto* Target=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),BattleId,Root.TargetUnitIds[0]))
                {
                    if (FVector::DistSquared(Origin,Target->GetActorLocation())>FMath::Square(Step.RangeCentimeters)) return;
                    Center=Target->GetActorLocation();
                }
            GetWorld()->SweepMultiByChannel(Hits,Center,Center,FQuat::Identity,Step.TraceChannel,FCollisionShape::MakeSphere(Step.RadiusCentimeters),Query);
        }
        else if (Step.Delivery==EOGWorldAbilityDelivery::MeleeSweep)
        {
            FString Error; FName A,B; auto* Part=TracePart(Step,A,B,Error); if (!Part) { Status=Error; return; }
            const FVector Start=Part->GetSocketLocation(A),End=Part->GetSocketLocation(B);
            GetWorld()->SweepMultiByChannel(Hits,Start,End,FQuat::Identity,Step.TraceChannel,FCollisionShape::MakeSphere(Step.RadiusCentimeters),Query);
            if (const auto* Previous=PreviousStarts.Find(Index))
            {
                TArray<FHitResult> Swept;
                GetWorld()->SweepMultiByChannel(Swept,*Previous,Start,FQuat::Identity,Step.TraceChannel,FCollisionShape::MakeSphere(Step.RadiusCentimeters),Query);
                Hits.Append(Swept);
            }
            if (const auto* Previous=PreviousEnds.Find(Index))
            {
                TArray<FHitResult> Swept;
                GetWorld()->SweepMultiByChannel(Swept,*Previous,End,FQuat::Identity,Step.TraceChannel,FCollisionShape::MakeSphere(Step.RadiusCentimeters),Query);
                Hits.Append(Swept);
            }
            PreviousStarts.Add(Index,Start); PreviousEnds.Add(Index,End);
        }
        Hits.Sort([&](const FHitResult& A,const FHitResult& B){return FVector::DistSquared(Origin,A.ImpactPoint)<FVector::DistSquared(Origin,B.ImpactPoint);});
        for (const auto& Hit:Hits)
        {
            auto* Actor=Hit.GetActor(); const auto Id=BodyId(Actor,BattleId); if (!Id.IsValid()) continue;
            if (Step.bRequireLineOfSight && !VisibleBetween(GetWorld(),GetOwner(),Actor,Origin,Hit.ImpactPoint,Step.VisibilityChannel)) continue;
            Out.AddUnique(Id);
        }
    }
    TArray<FOGEntityId> Eligible; FString Error;
    if (Battle->FilterWorldContacts(Root,Out,Eligible,Error)) Out=MoveTemp(Eligible); else { Out.Reset(); Status=Error; }
}
bool UOGWorldAbilityExecutionComponent::CommitContacts(int32 Step,const TArray<FOGEntityId>& Contacts,FString& Error)
{
    if (!Core.IsValid() || !ActiveCast.IsValid() || Completed.Contains(Step)) return false;
    if (!Core->ApplyOwnedWorldAbilityStep(ActiveCast,Step,Contacts,Error)) return false;
    Completed.Add(Step); return true;
}
bool UOGWorldAbilityExecutionComponent::CommitProjectileContact(const FOGEntityId& Cast,int32 Step,AActor* Contact,FString& Error)
{
    if (Cast!=ActiveCast || Completed.Contains(Step) || !Specification.Steps.IsValidIndex(Step)) return false;
    TArray<FOGEntityId> Targets; const auto Id=BodyId(Contact,BattleId); if (Id.IsValid()) Targets.Add(Id);
    const auto* Binding=GetOwner()?GetOwner()->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>():nullptr;
    if (!Binding || !Binding->IsBoundTo(BattleId,Root.SourceUnitId) ||
        (bTracksSourceMotion && !bSourceMotionFinished &&
            (!SourceMotion.IsValid() || SourceMotion->GetActiveCombatReceipt()!=ActiveCast)))
    { CancelActiveCast(); return false; }
    const bool Accepted=CommitContacts(Step,Targets,Error); if (!Accepted) Status=Error; return Accepted;
}
void UOGWorldAbilityExecutionComponent::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Tick,Function); if (!ActiveCast.IsValid() || !Core.IsValid()) return;
    FString Error;
    const auto* Binding=GetOwner()?GetOwner()->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>():nullptr;
    if (!Binding || !Binding->IsBoundTo(BattleId,Root.SourceUnitId) ||
        (bTracksSourceMotion && !bSourceMotionFinished &&
            (!SourceMotion.IsValid() || SourceMotion->GetActiveCombatReceipt()!=ActiveCast)))
    { CancelActiveCast(); return; }
    if (!Core->AdvanceOwnedWorldCombatTime(Error)) { Status=Error; return; }
    const auto* Battle=Core->GetOwnedLuminousTurnBattle(); FOGWorldAbilityCastView View;
    if (!Battle || Battle->GetState().BattleId!=BattleId || !Battle->ReadWorldCast(ActiveCast,View))
    {
        const auto* Unit=Battle?Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==Root.SourceUnitId;}):nullptr;
        EndLocalCast(Battle && Battle->GetState().BattleId==BattleId && Unit && Unit->CanAct() &&
            Completed.Num()==Specification.Steps.Num() &&
            Battle->GetState().WorldElapsedMilliseconds>=StartedAt+Specification.DurationMilliseconds);
        return;
    }
    const int64 Elapsed=Battle->GetState().WorldElapsedMilliseconds-StartedAt;
    for (int32 Index=0;Index<Specification.Steps.Num();++Index)
    {
        if (Completed.Contains(Index)) continue;
        const auto& Step=Specification.Steps[Index]; if (Elapsed<Step.OpensAtMilliseconds) continue;
        if ((Step.ClosesAtMilliseconds>Step.OpensAtMilliseconds && Elapsed>Step.ClosesAtMilliseconds) ||
            Elapsed>Specification.DurationMilliseconds)
        { if (!CommitContacts(Index,{},Error)) Status=Error; continue; }
        if (Step.Delivery==EOGWorldAbilityDelivery::Projectile)
        {
            if (SpawnedProjectiles.Contains(Index)) continue;
            FVector Direction=GetOwner()->GetActorForwardVector();
            if (!Root.TargetUnitIds.IsEmpty())
                if (auto* Target=UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(),BattleId,Root.TargetUnitIds[0]))
                    Direction=(Target->GetActorLocation()-GetOwner()->GetActorLocation()).GetSafeNormal();
            FActorSpawnParameters Params; Params.Owner=GetOwner(); Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            auto* Projectile=GetWorld()->SpawnActor<AOGNativeWorldProjectile>(AOGNativeWorldProjectile::StaticClass(),
                GetOwner()->GetActorLocation(),Direction.Rotation(),Params);
            if (!Projectile || !Projectile->InitializeAcceptedStep(this,ActiveCast,Index,Step,Direction,Error))
            { if (Projectile) Projectile->Destroy(); Status=Error; CommitContacts(Index,{},Error); }
            else
            {
                Projectile->SetLifeSpan(FMath::Max(.001,static_cast<double>(Step.ClosesAtMilliseconds-Elapsed)/1000.0));
                Projectiles.Add(Projectile);
            }
            SpawnedProjectiles.Add(Index); continue;
        }
        TArray<FOGEntityId> Targets; QueryContacts(Step,Index,Targets);
        if (!Targets.IsEmpty() || Step.Delivery==EOGWorldAbilityDelivery::Direct || Step.ClosesAtMilliseconds==Step.OpensAtMilliseconds)
            if (!CommitContacts(Index,Targets,Error)) Status=Error;
    }
}
void UOGWorldAbilityExecutionComponent::CancelActiveCast()
{
    if (bEndingLocal || !ActiveCast.IsValid()) return;
    FString Error;
    if (Core.IsValid() && !Core->CancelOwnedWorldAbility(ActiveCast,Error)) Status=Error;
    // A commit notification can forbid reentry. Release contact now; the World clock
    // cancels accepted casts whose exact runner has gone away on its next transaction.
    EndLocalCast();
}
void UOGWorldAbilityExecutionComponent::EndLocalCast(bool CompletedNormally)
{
    TGuardValue<bool> Ending(bEndingLocal,true);
    const auto EndingCast=ActiveCast,EndingBattle=BattleId;
    TWeakObjectPtr<UOGSharedMotionControllerComponent> EndingMotion=WorldMotion(GetOwner());
    const bool CompleteLoop=CompletedNormally && EndingMotion.IsValid() && EndingMotion->IsLoopingAction();
    if (SourceMotion.IsValid())
    {
        SourceMotion->OnMotionFinished.RemoveAll(this);
        RemoveTickPrerequisiteComponent(SourceMotion.Get());
        for (USkeletalMeshComponent* Part:SourceMotion->GetNativeParts()) if (Part) RemoveTickPrerequisiteComponent(Part);
    }
    SourceMotion.Reset(); bSourceMotionFinished=false; bTracksSourceMotion=false;
    ActiveCast=FOGEntityId(); Completed.Reset(); SpawnedProjectiles.Reset();
    for (auto& Projectile:Projectiles) if (Projectile.IsValid()) Projectile->Destroy();
    Projectiles.Reset(); PreviousStarts.Reset(); PreviousEnds.Reset();
    if (auto* Actions=GetOwner()?GetOwner()->FindComponentByClass<UOGWorldActionRuntimeComponent>():nullptr)
        if (Actions->HasActiveAction()) Actions->EndAuthoredAction();
    // Completion listeners may start a new cast. All old local state is released before publishing the final cue.
    if (EndingMotion.IsValid())
    {
        if (CompleteLoop) EndingMotion->CompleteCombatAction(EndingBattle,EndingCast);
        else if (!CompletedNormally) EndingMotion->CancelCombatAction(EndingBattle,EndingCast);
    }
}
void UOGWorldAbilityExecutionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelActiveCast();
    if (auto* Actions=GetOwner()?GetOwner()->FindComponentByClass<UOGWorldActionRuntimeComponent>():nullptr)
        Actions->OnAuthoredActionEnded.RemoveAll(this);
    Super::EndPlay(Reason);
}
