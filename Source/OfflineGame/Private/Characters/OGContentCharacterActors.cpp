#include "Characters/OGContentCharacterActors.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGTurnBattle.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"

AOGContentPlayableCharacter::AOGContentPlayableCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Appearance = CreateDefaultSubobject<UOGCharacterAppearanceComponent>(TEXT("ContentAppearance"));
    CombatBody = CreateDefaultSubobject<UOGCanonicalCombatActorBindingComponent>(TEXT("CanonicalCombatBody"));
    if (GetMesh()) Appearance->BodyComponentNamesToReplace.Add(GetMesh()->GetFName());
}

bool AOGContentPlayableCharacter::BindOwnedWorldCharacter_Implementation(
    const FOGEntityId& Ruler, const FOGContentId& Identity, const FOGContentId& Version,
    const FOGEntityId& Manifestation, bool bSfw)
{
    FString Error;
    if (!Appearance || !Appearance->BindOwnedManifestation(Ruler, Identity, Version, Manifestation, bSfw, Error))
    {
        UE_LOG(LogTemp, Warning, TEXT("Character appearance binding failed: %s"), *Error);
        return false;
    }
    if (BoundCore.IsValid()) BoundCore->OnLuminousCombatActionResolved.RemoveAll(this);
    UOGGameCoreSubsystem* Core = GetGameInstance() ? GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    BoundCore = Core;
    BoundManifestation = Manifestation;
    BoundRuler = Ruler;
    BoundIdentity = Identity;
    BoundVersion = Version;
    FailedTraversalAction = NAME_None;
    LastMotionController.Reset();
    if (Core) Core->OnLuminousCombatActionResolved.AddUObject(this, &ThisClass::HandleResolvedCombatAction);
    return true;
}

bool AOGContentPlayableCharacter::IsBoundToOwnedCombatUnit(const FOGEntityId& Ruler,
    const FOGEntityId& Manifestation, const FOGContentId& Version) const
{
    return Appearance && Appearance->MatchesOwnedManifestation(Ruler, Manifestation) &&
        Ruler == BoundRuler && Manifestation == BoundManifestation && Version == BoundVersion;
}

void AOGContentPlayableCharacter::HandleResolvedCombatAction(const FOGEntityId& Source,
    const FOGContentId& Skill)
{
    if (Source != BoundManifestation || !Appearance ||
        !Appearance->MatchesOwnedManifestation(BoundRuler, BoundManifestation)) return;
    const FName* Semantic = CombatActionSemantics.Find(Skill);
    if (!Semantic || Semantic->IsNone()) return;
    FString Error;
    if (!Appearance->PlaySemanticAction(*Semantic, TEXT("Combat"), false, 1, NAME_None, Error))
        UE_LOG(LogTemp, Warning, TEXT("Source combat motion %s is unavailable: %s"), *Semantic->ToString(), *Error);
}

void AOGContentPlayableCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Appearance || !Appearance->MatchesOwnedManifestation(BoundRuler, BoundManifestation)) return;
    const FOGTurnBattle* Battle = BoundCore.IsValid() ? BoundCore->GetOwnedLuminousTurnBattle() : nullptr;
    if (Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running && CombatBody)
    {
        const FOGCombatUnitState* Unit = Battle->GetState().Units.FindByPredicate(
            [this](const FOGCombatUnitState& U) { return U.UnitEntityId == BoundManifestation; });
        if (Unit && Unit->CanAct() && !CombatBody->IsBoundTo(Battle->GetState().BattleId, BoundManifestation))
        {
            FString Error;
            CombatBody->BindActualNativeUnit(BoundManifestation, BoundIdentity, Error);
        }
        // Turn animation is driven by accepted combat events rather than
        // ground locomotion overwriting a visible attack or defeat.
        UOGSharedMotionControllerComponent* Motion = Appearance->GetMotionController();
        if (Motion && Motion->IsPlayingAction() && Motion->CurrentAction.ToString().StartsWith(TEXT("Traversal.")))
            Motion->StopAction();
        return;
    }
    UOGSharedMotionControllerComponent* Controller = Appearance->GetMotionController();
    if (!Controller) return;
    if (LastMotionController.Get() != Controller)
    {
        LastMotionController = Controller;
        FailedTraversalAction = NAME_None;
    }
    if (Controller->IsPlayingAction() &&
        !Controller->CurrentAction.ToString().StartsWith(TEXT("Traversal."))) return;
    const UCharacterMovementComponent* Movement = GetCharacterMovement();
    const float Speed = GetVelocity().Size2D();
    FName Action(TEXT("Traversal.Idle"));
    if (Movement && Movement->IsSwimming()) Action = Speed > 1.f ? TEXT("Traversal.Swim") : TEXT("Traversal.SwimIdle");
    else if (Movement && Movement->IsFlying()) Action = GetVelocity().Size() > 1.f ? TEXT("Traversal.Fly") : TEXT("Traversal.FlightHover");
    else if (Movement && Movement->IsFalling()) Action = GetVelocity().Z > 0.f ? TEXT("Traversal.Jump") : TEXT("Traversal.Fall");
    else if (Speed > 1.f) Action = RunAnimationSpeedCmPerSecond > 0.f && Speed >= RunAnimationSpeedCmPerSecond
        ? TEXT("Traversal.Run") : TEXT("Traversal.Walk");
    if ((!Controller->IsPlayingAction() || Controller->CurrentAction != Action) && FailedTraversalAction != Action)
    {
        FString Error;
        if (Appearance->PlaySemanticAction(Action, TEXT("Traversal"), false, 1, NAME_None, Error))
            FailedTraversalAction = NAME_None;
        else
        {
            FailedTraversalAction = Action;
            UE_LOG(LogTemp, Warning, TEXT("Source traversal motion %s is unavailable: %s"), *Action.ToString(), *Error);
        }
    }
}

void AOGContentPlayableCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if (BoundCore.IsValid()) BoundCore->OnLuminousCombatActionResolved.RemoveAll(this);
    BoundCore.Reset();
    Super::EndPlay(Reason);
}

AOGContentCharacterPreviewActor::AOGContentCharacterPreviewActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AppearanceRoot"));
    Appearance = CreateDefaultSubobject<UOGCharacterAppearanceComponent>(TEXT("ContentAppearance"));
    SetActorEnableCollision(false);
}

bool AOGContentCharacterPreviewActor::ConfigureOwnedPreview_Implementation(
    const FOGEntityId& Ruler, const FOGContentId& Identity, const FOGContentId& Version,
    const FOGEntityId& Manifestation, bool bSfw)
{
    FString Error;
    const bool Result = Appearance && Appearance->BindOwnedManifestation(
        Ruler, Identity, Version, Manifestation, bSfw, Error);
    if (!Result) UE_LOG(LogTemp, Warning, TEXT("Character preview binding failed: %s"), *Error);
    return Result;
}
