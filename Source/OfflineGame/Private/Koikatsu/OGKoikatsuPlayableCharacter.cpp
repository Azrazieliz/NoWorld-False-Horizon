#include "Koikatsu/OGKoikatsuPlayableCharacter.h"
#include "Animation/OGSharedMotionLibrary.h"
#include "Animation/OGSharedMotionControllerComponent.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
FName SharedActionForState(EOGKKVisualState State)
{
    switch (State)
    {
    case EOGKKVisualState::Idle: return TEXT("Traversal.Idle");
    case EOGKKVisualState::Walk: return TEXT("Traversal.Walk");
    case EOGKKVisualState::Run: return TEXT("Traversal.Run");
    case EOGKKVisualState::Jump: return TEXT("Traversal.Jump");
    case EOGKKVisualState::Fall: return TEXT("Traversal.Fall");
    case EOGKKVisualState::Land: return TEXT("Traversal.Land");
    case EOGKKVisualState::TurnLeft: return TEXT("Traversal.TurnLeft");
    case EOGKKVisualState::TurnRight: return TEXT("Traversal.TurnRight");
    case EOGKKVisualState::Dodge: return TEXT("Combat.Dodge");
    case EOGKKVisualState::Action: return TEXT("Interaction.Action");
    case EOGKKVisualState::SwimIdle: return TEXT("Traversal.SwimIdle");
    case EOGKKVisualState::Swim: return TEXT("Traversal.Swim");
    case EOGKKVisualState::FlyingIdle: return TEXT("Traversal.FlightHover");
    case EOGKKVisualState::Flying: return TEXT("Traversal.Fly");
    }
    return NAME_None;
}
FName SharedContextForAction(FName Action)
{
    const FString Name = Action.ToString();
    if (Name.StartsWith(TEXT("Combat."))) return TEXT("Combat");
    if (Name.StartsWith(TEXT("Interaction."))) return TEXT("Interaction");
    return TEXT("Traversal");
}
}

AOGKoikatsuPlayableCharacter::AOGKoikatsuPlayableCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    SharedMotionController = CreateDefaultSubobject<UOGSharedMotionControllerComponent>(TEXT("NativeSharedMotionController"));
    SharedMotionController->RigProfile = TEXT("koikatsu_fixture_21");
    SharedMotionCapabilities = {TEXT("grounded"), TEXT("swimming"),
        TEXT("flying"), TEXT("emote"), TEXT("combat")};
}

bool AOGKoikatsuPlayableCharacter::ParsePartId(
    const FString& Name, int32& Mesh, int32& Primitive)
{
    if (!Name.StartsWith(TEXT("KK_Full_M"))) return false;
    FString Left, Right, MeshString, PrimitiveString;
    if (!Name.Split(TEXT("_M"), &Left, &Right)) return false;
    if (!Right.Split(TEXT("P"), &MeshString, &PrimitiveString)) return false;
    // UE5 SCS component templates can retain a _GEN_VARIABLE suffix;
    // runtime instances and source receipts use the public primitive ID.
    PrimitiveString.RemoveFromEnd(TEXT("_GEN_VARIABLE"));
    if (!MeshString.IsNumeric() || !PrimitiveString.IsNumeric()) return false;
    Mesh = FCString::Atoi(*MeshString);
    Primitive = FCString::Atoi(*PrimitiveString);
    return Mesh >= 0 && Mesh <= 99 && Primitive >= 0 && Primitive <= 99;
}

void AOGKoikatsuPlayableCharacter::BeginPlay()
{
    Super::BeginPlay();
    TArray<USkeletalMeshComponent*> Existing;
    GetComponents<USkeletalMeshComponent>(Existing);
    for (USkeletalMeshComponent* Comp : Existing)
    {
        int32 MeshIndex=-1, PrimitiveIndex=-1;
        if (!Comp || !ParsePartId(Comp->GetName(), MeshIndex, PrimitiveIndex)) continue;
        if (!Comp->GetSkeletalMeshAsset() || !Comp->GetSkeletalMeshAsset()->GetSkeleton())
        {
            UE_LOG(LogTemp, Error, TEXT("Gate12 KK: missing imported skeleton for %s"), *Comp->GetName());
            continue;
        }
        NativeParts.Add(Comp);
        SourceMeshIds.Add(MeshIndex);
        SourcePrimitiveIds.Add(PrimitiveIndex);
        ActiveClips.Add(nullptr);
        Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Comp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    }
    // Avoid a second copy of each mesh: actual Blueprint already contains 19.
    if (NativeParts.Num() != 19)
    {
        UE_LOG(LogTemp, Error, TEXT("Gate12 KK: expected 19 exact imported Blueprint mesh parts, found %d"),
               NativeParts.Num());
        return;
    }
    // Register existing 19 mesh components for character-independent semantic
    // playback. Per-part legacy sequences remain the fallback for incompatible
    // skeleton derivatives and are not rebuilt or discarded.
    if (SharedMotionController)
    {
        TArray<USkeletalMeshComponent*> Native;
        for (USkeletalMeshComponent* Part : NativeParts) Native.Add(Part);
        SharedMotionController->RigProfile = SharedMotionRigProfile;
        SharedMotionController->AnatomyProfile = SharedMotionAnatomy;
        SharedMotionController->OutfitProfile = SharedOutfitProfile;
        SharedMotionController->Capabilities = SharedMotionCapabilities;
        SharedMotionController->SharedLibrary = SharedMotionLibrary;
        if (!SharedMotionController->RegisterNativeParts(Native))
            UE_LOG(LogTemp, Warning, TEXT("Shared motion rig registration unavailable for Koikatsu fixture"));
    }
    // Resolve an actual source-to-Unreal +Z(gltf) -> +Y(UE) face conversion:
    // the frozen Blueprint import has yaw 0 while UE capsule moves +X forward.
    // Apply the measured, fixture-specific -90 deg around UE Z to all 19
    // submeshes consistently; do not alter CharacterMovement input axes.
    int32 CalibratedParts=0;
    for(USkeletalMeshComponent* Comp : NativeParts)
    {
        if(!Comp) continue;
        const FRotator Before=Comp->GetRelativeRotation();
        if(!FMath::IsNearlyZero(Before.Yaw,0.25f))
        {
            UE_LOG(LogTemp, Error,TEXT("Gate12 MotionV3: existing component yaw nonzero %s: %f; refusing guessed correction"),
                   *Comp->GetName(),Before.Yaw);
            continue;
        }
        Comp->SetRelativeRotation(FRotator(Before.Pitch,
            Before.Yaw+FixtureVisualForwardYawCorrectionDegrees,Before.Roll));
        ++CalibratedParts;
    }
    UE_LOG(LogTemp, Display,TEXT("Gate12 MotionV3: calibrated fixture visual axis yaw=%f count=%d/%d; UE actor forward +X"),
           FixtureVisualForwardYawCorrectionDegrees,CalibratedParts,NativeParts.Num());
    if (GetMesh()) GetMesh()->SetHiddenInGame(true);
    LastYaw = GetActorRotation().Yaw;
    bAllPartsHaveNativeQualityV2=IsQualityV2Complete();
    SmoothedHorizontalSpeed=GetVelocity().Size2D();
    ApplyState(EOGKKVisualState::Idle);
    UE_LOG(LogTemp, Display, TEXT("Gate12 KK MotionV2: native=%d complete_quality_clips=%d synced=%d"),
           NativeParts.Num(),bAllPartsHaveNativeQualityV2,bSynchronizeNativePartPhases);
    UE_LOG(LogTemp, Display, TEXT("Gate12 KK: bound %d true skeletal mesh parts to native action controller"),
           NativeParts.Num());
}

UAnimSequence* AOGKoikatsuPlayableCharacter::LoadNativeClip(
    int32 Index, EOGKKVisualState State, bool bQualityV2)
{
    if(!NativeParts.IsValidIndex(Index)||!SourceMeshIds.IsValidIndex(Index)||
       !SourcePrimitiveIds.IsValidIndex(Index)) return nullptr;
    USkeletalMeshComponent* Comp=NativeParts[Index];
    if(!Comp || !Comp->GetSkeletalMeshAsset()) return nullptr;
    const TCHAR* Name=TEXT("Idle");
    switch(State)
    {
      case EOGKKVisualState::Walk: Name=TEXT("Walk"); break;
      case EOGKKVisualState::Run: Name=TEXT("Run"); break;
      case EOGKKVisualState::Jump: Name=TEXT("Jump"); break;
      case EOGKKVisualState::Fall: Name=TEXT("Fall"); break;
      case EOGKKVisualState::Land: Name=TEXT("Land"); break;
      case EOGKKVisualState::TurnLeft: Name=TEXT("TurnLeft"); break;
      case EOGKKVisualState::TurnRight: Name=TEXT("TurnRight"); break;
      case EOGKKVisualState::Dodge: Name=TEXT("Dodge"); break;
      case EOGKKVisualState::Action: Name=TEXT("Action"); break;
      case EOGKKVisualState::Swim: Name=TEXT("Swim"); break;
      case EOGKKVisualState::SwimIdle: Name=TEXT("SwimIdle"); break;
      case EOGKKVisualState::Flying: Name=TEXT("Flying"); break;
      case EOGKKVisualState::FlyingIdle: Name=TEXT("FlyingIdle"); break;
      default: break;
    }
    const FString Asset=bQualityV2
       ? FString::Printf(TEXT("AN_KK_V2_%s_M%02d_P%02d"),Name,SourceMeshIds[Index],SourcePrimitiveIds[Index])
       : State==EOGKKVisualState::Idle
         ? FString::Printf(TEXT("AN_KK_NativeRestSafe_M%02d_P%02d"),SourceMeshIds[Index],SourcePrimitiveIds[Index])
         : FString::Printf(TEXT("AN_KK_%s_M%02d_P%02d"),Name,SourceMeshIds[Index],SourcePrimitiveIds[Index]);
    const FString Folder=bQualityV2
       ? TEXT("/Game/Experimental/Gate12Koikatsu/Animation/NativeQualityV2")
       : State==EOGKKVisualState::Idle
         ? TEXT("/Game/Experimental/Gate12Koikatsu/Animation/NativeRestSafe")
         : TEXT("/Game/Experimental/Gate12Koikatsu/Animation/NativeActions");
    const FString Path=FString::Printf(TEXT("%s/%s.%s"),*Folder,*Asset,*Asset);
    UAnimSequence* Seq=LoadObject<UAnimSequence>(nullptr,*Path);
    if(Seq && Seq->GetSkeleton()!=Comp->GetSkeletalMeshAsset()->GetSkeleton())
    {
        UE_LOG(LogTemp, Error,TEXT("Gate12 KK MotionV2: exact-skeleton mismatch %s"),*Path);
        return nullptr;
    }
    return Seq;
}

bool AOGKoikatsuPlayableCharacter::IsQualityV2Complete() const
{
    if(NativeParts.Num()!=19) return false;
    for(int32 I=0;I<NativeParts.Num();++I)
        for(EOGKKVisualState S : {EOGKKVisualState::Walk,EOGKKVisualState::Run,
                                 EOGKKVisualState::Swim,EOGKKVisualState::SwimIdle})
            if(!const_cast<AOGKoikatsuPlayableCharacter*>(this)->LoadNativeClip(I,S,true))
                return false;
    return true;
}

UAnimSequence* AOGKoikatsuPlayableCharacter::ResolveNativeClip(int32 Index,EOGKKVisualState State)
{
    UAnimSequence* Clip=nullptr;
    // The actual rig component owns compatibility; never use a semantic name
    // alone to force foreign motion onto a different 19-part source skeleton.
    if (NativeParts.IsValidIndex(Index) && NativeParts[Index])
    {
        FName Action = NAME_None;
        FName Context(TEXT("Traversal"));
        switch(State)
        {
        case EOGKKVisualState::Idle: Action=TEXT("Traversal.Idle"); break;
        case EOGKKVisualState::Walk: Action=TEXT("Traversal.Walk"); break;
        case EOGKKVisualState::Run: Action=TEXT("Traversal.Run"); break;
        case EOGKKVisualState::Jump: Action=TEXT("Traversal.Jump"); break;
        case EOGKKVisualState::Fall: Action=TEXT("Traversal.Fall"); break;
        case EOGKKVisualState::Land: Action=TEXT("Traversal.Land"); break;
        case EOGKKVisualState::TurnLeft: Action=TEXT("Traversal.TurnLeft"); break;
        case EOGKKVisualState::TurnRight: Action=TEXT("Traversal.TurnRight"); break;
        case EOGKKVisualState::Dodge: Action=TEXT("Combat.Dodge"); Context=TEXT("Combat"); break;
        case EOGKKVisualState::Action: Action=TEXT("Interaction.Action"); Context=TEXT("Interaction"); break;
        case EOGKKVisualState::SwimIdle: Action=TEXT("Traversal.SwimIdle"); break;
        case EOGKKVisualState::Swim: Action=TEXT("Traversal.Swim"); break;
        case EOGKKVisualState::FlyingIdle: Action=TEXT("Traversal.FlightHover"); break;
        case EOGKKVisualState::Flying: Action=TEXT("Traversal.Fly"); break;
        }
        if(!Action.IsNone())
        {
            const UOGSharedMotionLibrary* Library=SharedMotionLibrary.IsNull()
                ? nullptr : SharedMotionLibrary.LoadSynchronous();
            if(!Library) Library=FOGSharedMotionResolver::GetInstalledLibrary();
            TArray<TObjectPtr<USkeletalMeshComponent>> ExactPart;
            ExactPart.Add(NativeParts[Index]);
            Clip=FOGSharedMotionResolver::Resolve(Library,Action,
                SharedMotionRigProfile,SharedMotionAnatomy,SharedOutfitProfile,
                Context,SharedMotionCapabilities,ExactPart,false,1);
        }
    }
    if(!Clip && bAllPartsHaveNativeQualityV2 &&
       (State==EOGKKVisualState::Walk||State==EOGKKVisualState::Run||
        State==EOGKKVisualState::Swim||State==EOGKKVisualState::SwimIdle))
        Clip=LoadNativeClip(Index,State,true);
    // Old fixture lacks a swim clip: a safe rest pose is preferable to
    // erroneously cycling through giant ground-running strides underwater.
    if(!Clip && (State==EOGKKVisualState::Flying||State==EOGKKVisualState::FlyingIdle))
        Clip=LoadNativeClip(Index,EOGKKVisualState::Idle,false);
    if(!Clip && (State==EOGKKVisualState::Swim||State==EOGKKVisualState::SwimIdle))
        Clip=LoadNativeClip(Index,EOGKKVisualState::Idle,false);
    if(!Clip) Clip=LoadNativeClip(Index,State,false);
    if(!Clip)
    {
        UE_LOG(LogTemp,Warning,TEXT("Gate12 KK MotionV2: missing exact clip part=%d state=%d"),
               Index,static_cast<int32>(State));
        return nullptr;
    }
    LoadedClips.AddUnique(Clip);
    return Clip;
}

bool AOGKoikatsuPlayableCharacter::Loops(EOGKKVisualState State)
{
    return State == EOGKKVisualState::Idle || State == EOGKKVisualState::Walk ||
           State == EOGKKVisualState::Run || State == EOGKKVisualState::Fall ||
           State == EOGKKVisualState::TurnLeft || State == EOGKKVisualState::TurnRight ||
           State == EOGKKVisualState::Swim || State == EOGKKVisualState::SwimIdle ||
           State == EOGKKVisualState::Flying || State == EOGKKVisualState::FlyingIdle;
}

void AOGKoikatsuPlayableCharacter::ApplyState(EOGKKVisualState Next)
{
    if (NativeParts.Num()!=19) return;
    const FName Action = SharedActionForState(Next);
    if (!Action.IsNone() && SharedMotionController &&
        SharedMotionController->PlayAction(Action,
            SharedContextForAction(Action), false, 1))
    {
        // Shared exact-source derivatives can animate all 19 parts in phase.
        // Individual fallback sequences are retained for incomplete imports.
        for (TObjectPtr<UAnimSequence>& Previous : ActiveClips) Previous = nullptr;
        CurrentState = Next;
        ActiveStateSeconds = 0.f;
        LastMotionSwitchTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        return;
    }
    if (SharedMotionController && SharedMotionController->IsPlayingAction())
        SharedMotionController->StopAction();
    int32 Replaced=0;
    for (int32 Index=0;Index<NativeParts.Num();++Index)
    {
        USkeletalMeshComponent* Comp=NativeParts[Index];
        if (!Comp) continue;
        UAnimSequence* Seq=ResolveNativeClip(Index,Next);
        if (!Seq) continue;
        if (ActiveClips[Index]==Seq) continue;
        Comp->PlayAnimation(Seq,Loops(Next));
        if(bSynchronizeNativePartPhases)
            if(UAnimSingleNodeInstance* Inst=Comp->GetSingleNodeInstance())
                Inst->SetPlaying(false);
        ActiveClips[Index]=Seq;
        ++Replaced;
    }
    CurrentState=Next;
    ActiveStateSeconds=0.f;
    LastMotionSwitchTime=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
    UE_LOG(LogTemp, Display, TEXT("Gate12 KK: state=%d clips_transitioned=%d/%d"),
           static_cast<int32>(Next),Replaced,NativeParts.Num());
}

EOGKKVisualState AOGKoikatsuPlayableCharacter::ComputeState() const
{
    if (bOverride) return ForcedState;
    const UCharacterMovementComponent* Movement=GetCharacterMovement();
    // Airborne flight is NOT ground gait, even with high horizontal velocity.
    // V3 provisional fallback uses neutral/rest pose until actual mocap flying
    // is authored; never replay twisted V1/V2 walk/run while flying.
    if(Movement && Movement->IsFlying())
        return SmoothedHorizontalSpeed<=WalkThresholdCmPerSecond
            ? EOGKKVisualState::FlyingIdle : EOGKKVisualState::Flying;
    if(Movement && Movement->IsSwimming())
        return SmoothedHorizontalSpeed<=WalkThresholdCmPerSecond
            ? EOGKKVisualState::SwimIdle : EOGKKVisualState::Swim;
    if (IsDodging()) return EOGKKVisualState::Dodge;
    const UWorld* World=GetWorld();
    const float Now=World ? World->GetTimeSeconds() : 0.f;
    if (Now<ActionUntilTime) return EOGKKVisualState::Action;
    if (Movement && Movement->IsFalling())
        return GetVelocity().Z>5.f ? EOGKKVisualState::Jump : EOGKKVisualState::Fall;
    if (Now<LandUntilTime) return EOGKKVisualState::Land;
    const float Speed=SmoothedHorizontalSpeed;
    if (Speed<=WalkThresholdCmPerSecond)
    {
        if (YawDegreesPerSecond>30.f) return EOGKKVisualState::TurnRight;
        if (YawDegreesPerSecond< -30.f) return EOGKKVisualState::TurnLeft;
        return EOGKKVisualState::Idle;
    }
    if (IsSprinting() || Speed>=RunThresholdCmPerSecond) return EOGKKVisualState::Run;
    return EOGKKVisualState::Walk;
}

void AOGKoikatsuPlayableCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Yaw=GetActorRotation().Yaw;
    YawDegreesPerSecond=DeltaSeconds>0.0001f
        ? FMath::FindDeltaAngleDegrees(LastYaw,Yaw)/DeltaSeconds : 0.f;
    LastYaw=Yaw;
    // A two-dimensional velocity in actor-local coordinates is the correct
    // driving signal for a directional Blend Space; world X/Y must NEVER be
    // treated as mesh-bone X/Y. The source mesh's -90 facing correction is
    // handled independently above.
    const FVector V=GetVelocity();
    const FVector Horizontal(V.X,V.Y,0.f);
    const FVector Local=GetActorTransform().InverseTransformVectorNoScale(Horizontal);
    ForwardVelocityCmS=Local.X;
    RightVelocityCmS=Local.Y;
    FacingRelativeVelocityAngleDegrees=FMath::RadiansToDegrees(
        FMath::Atan2(RightVelocityCmS,ForwardVelocityCmS));
    if(Horizontal.Size2D()<=WalkThresholdCmPerSecond)
        CurrentTravelDirection=EOGKKTravelDirection::Idle;
    else if(FMath::Abs(ForwardVelocityCmS)>=FMath::Abs(RightVelocityCmS))
        CurrentTravelDirection=ForwardVelocityCmS>=0.f
            ? EOGKKTravelDirection::Forward : EOGKKTravelDirection::Backward;
    else
        CurrentTravelDirection=RightVelocityCmS>=0.f
            ? EOGKKTravelDirection::Right : EOGKKTravelDirection::Left;

    SmoothedHorizontalSpeed=FMath::FInterpTo(SmoothedHorizontalSpeed,
        GetVelocity().Size2D(),DeltaSeconds,FMath::Max(0.5f,MotionSpeedSmoothing));
    // External gameplay/cinematic actions own their actual registered clip
    // until completion; locomotion must not replace them the next frame.
    const bool bControllerPlaying = SharedMotionController &&
        SharedMotionController->IsPlayingAction();
    const bool bSharedTraversal = bControllerPlaying &&
        SharedMotionController->CurrentAction.ToString().StartsWith(TEXT("Traversal."));
    if (bControllerPlaying && !bSharedTraversal)
    {
        bSharedControllerWasPlaying = true;
        UpdateExpressionPulses(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
        return;
    }
    if (bSharedControllerWasPlaying)
    {
        bSharedControllerWasPlaying = false;
        for (TObjectPtr<UAnimSequence>& OldClip : ActiveClips) OldClip = nullptr;
        ApplyState(ComputeState()); // Restore the original per-part locomotion driver.
    }
    const EOGKKVisualState Next=ComputeState();
    const bool bGaitCurrent=CurrentState==EOGKKVisualState::Walk||CurrentState==EOGKKVisualState::Run;
    const bool bGaitNext=Next==EOGKKVisualState::Walk||Next==EOGKKVisualState::Run;
    const float Now=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
    if(Next!=CurrentState && (!bGaitCurrent||!bGaitNext||
        Now-LastMotionSwitchTime>=MotionStateMinHoldSeconds))
        ApplyState(Next);
    if (!SharedMotionController || !SharedMotionController->IsPlayingAction())
        SyncPartPhases(DeltaSeconds);
    UpdateExpressionPulses(Now);
}

void AOGKoikatsuPlayableCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    if (UWorld* World=GetWorld()) LandUntilTime=World->GetTimeSeconds()+0.45f;
}

void AOGKoikatsuPlayableCharacter::TriggerVisualAction(float DurationSeconds)
{
    if (UWorld* World=GetWorld())
        ActionUntilTime=World->GetTimeSeconds()+FMath::Clamp(DurationSeconds,0.05f,5.f);
}

void AOGKoikatsuPlayableCharacter::OverrideVisualState(EOGKKVisualState State,bool bEnabled)
{
    ForcedState=State;
    bOverride=bEnabled;
    ApplyState(ComputeState());
}

void AOGKoikatsuPlayableCharacter::SetExpressionMorph(FName Name,float Weight)
{
    const float Clamped=FMath::Clamp(Weight,0.f,1.f);
    for (USkeletalMeshComponent* Comp : NativeParts)
    {
        if (Comp && Comp->GetSkeletalMeshAsset() &&
            Comp->GetSkeletalMeshAsset()->FindMorphTarget(Name))
            Comp->SetMorphTarget(Name,Clamped);
    }
}

void AOGKoikatsuPlayableCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if (SharedMotionController) SharedMotionController->StopAction();
    NativeParts.Reset();
    ActiveClips.Reset();
    LoadedClips.Reset();
    ExpressionPulses.Reset();
    Super::EndPlay(Reason);
}


void AOGKoikatsuPlayableCharacter::SyncPartPhases(float DeltaSeconds)
{
    if(!bSynchronizeNativePartPhases||DeltaSeconds<=0.f||
       NativeParts.Num()!=ActiveClips.Num()||NativeParts.Num()!=19) return;
    ActiveStateSeconds+=DeltaSeconds;
    const bool bGait=CurrentState==EOGKKVisualState::Walk||CurrentState==EOGKKVisualState::Run;
    if(bGait)
    {
        const float Cycle=CurrentState==EOGKKVisualState::Run?0.8f:1.0f;
        const float Scale=FMath::Clamp(
            SmoothedHorizontalSpeed/(CurrentState==EOGKKVisualState::Run?650.f:350.f),0.65f,1.35f);
        GaitPhaseCycles=FMath::Fmod(GaitPhaseCycles+DeltaSeconds*Scale/Cycle,1.f);
    }
    const bool bSwim=CurrentState==EOGKKVisualState::Swim||CurrentState==EOGKKVisualState::SwimIdle;
    const float SwimPhase=bSwim?FMath::Fmod(ActiveStateSeconds/
        (CurrentState==EOGKKVisualState::Swim?1.2f:1.8f),1.f):0.f;
    for(int32 I=0;I<NativeParts.Num();++I)
    {
        USkeletalMeshComponent* Comp=NativeParts[I];
        UAnimSequence* Seq=ActiveClips[I];
        if(!Comp||!Seq) continue;
        if(UAnimSingleNodeInstance* Inst=Comp->GetSingleNodeInstance())
            Inst->SetPlaying(false);
        const float Length=FMath::Max(Seq->GetPlayLength(),0.01f);
        const float Time=bGait?GaitPhaseCycles*Length:
            bSwim?SwimPhase*Length:
            Loops(CurrentState)?FMath::Fmod(ActiveStateSeconds,Length):
                FMath::Min(ActiveStateSeconds,Length-0.001f);
        Comp->SetPosition(Time,false);
    }
}

bool AOGKoikatsuPlayableCharacter::PulseExpressionMorph(
    FName Name,float PeakWeight,float RiseSeconds,float HoldSeconds,float FadeSeconds)
{
    if(Name.IsNone()||NativeParts.Num()!=19) return false;
    int32 Matches=0;
    for(USkeletalMeshComponent* Comp:NativeParts)
        if(Comp&&Comp->GetSkeletalMeshAsset()&&
           Comp->GetSkeletalMeshAsset()->FindMorphTarget(Name))
            ++Matches;
    if(Matches==0) return false;
    ExpressionPulses.RemoveAll([Name](const FExpressionPulse& P){return P.MorphName==Name;});
    FExpressionPulse P;
    P.MorphName=Name;
    P.Peak=FMath::Clamp(PeakWeight,0.f,1.f);
    P.Start=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
    P.Rise=FMath::Max(RiseSeconds,0.01f);
    P.Hold=FMath::Max(HoldSeconds,0.f);
    P.Fade=FMath::Max(FadeSeconds,0.01f);
    ExpressionPulses.Add(P);
    UE_LOG(LogTemp,Display,TEXT("Gate12 KK MotionV2: morph pulse %s real_native_matches=%d"),
           *Name.ToString(),Matches);
    return true;
}

void AOGKoikatsuPlayableCharacter::UpdateExpressionPulses(float Now)
{
    for(int32 I=ExpressionPulses.Num()-1;I>=0;--I)
    {
        const FExpressionPulse& P=ExpressionPulses[I];
        const float T=FMath::Max(0.f,Now-P.Start);
        const float Total=P.Rise+P.Hold+P.Fade;
        float Value=0.f;
        if(T<P.Rise) Value=P.Peak*FMath::SmoothStep(0.f,P.Rise,T);
        else if(T<P.Rise+P.Hold) Value=P.Peak;
        else if(T<Total)
            Value=P.Peak*(1.f-FMath::SmoothStep(P.Rise+P.Hold,Total,T));
        SetExpressionMorph(P.MorphName,Value);
        if(T>=Total) ExpressionPulses.RemoveAtSwap(I);
    }
}
