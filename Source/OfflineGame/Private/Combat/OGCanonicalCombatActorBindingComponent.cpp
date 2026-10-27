#include "Combat/OGCanonicalCombatActorBindingComponent.h"

#include "Combat/OGTurnBattle.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Characters/OGContentCharacterActors.h"

bool UOGCanonicalCombatActorBindingComponent::BindActualNativeUnit(
    const FOGEntityId& UnitId, const FOGContentId& ApprovedIdentity,
    FString& OutError)
{
    OutError.Reset();
    UWorld* World=GetWorld();
    AActor* Owner=GetOwner();
    if (!World || !World->IsGameWorld() || !Owner ||
        !UnitId.IsValid() || !ApprovedIdentity.IsValid())
    {
        OutError=TEXT("Valid existing source character, World and unit ID required.");
        return false;
    }
    UGameInstance* GI=World->GetGameInstance();
    const UOGGameCoreSubsystem* Core=GI
        ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const FOGTurnBattle* Battle=Core
        ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!Battle || Battle->GetState().Status!=EOGTurnBattleStatus::Running)
    {
        OutError=TEXT("No active authoritative GameCore encounter for this visual.");
        return false;
    }
    const FOGTurnBattleState& State=Battle->GetState();
    const FOGCombatUnitState* Unit=State.Units.FindByPredicate(
        [&UnitId](const FOGCombatUnitState& Candidate)
        { return Candidate.UnitEntityId==UnitId; });
    if (!Unit || !Unit->CanAct() || Unit->IdentityId!=ApprovedIdentity)
    {
        OutError=TEXT("Actor Identity cannot be bound to a nonmatching reserve/defeated/unknown canonical unit.");
        return false;
    }
    // Reject mock actors: the physical participant must have actual native
    // source skeletal geometry. This creates no mesh, version or placeholder.
    bool bHasNativeBody=false;
    TArray<USkeletalMeshComponent*> Parts;
    Owner->GetComponents<USkeletalMeshComponent>(Parts);
    for (USkeletalMeshComponent* Part:Parts)
        if (Part && Part->GetSkeletalMeshAsset() &&
            Part->GetSkeletalMeshAsset()->GetSkeleton())
        { bHasNativeBody=true;break; }
    if (!bHasNativeBody)
    {
        OutError=TEXT("Source character has no valid native skeletal body.");
        return false;
    }
    // One actual world body per canonical unit; never duplicate actors while
    // streaming, and never confuse the player with an unrelated NPC.
    if (AActor* Other=FindActualWorldActor(World,State.BattleId,UnitId))
        if (Other!=Owner)
        {
            OutError=TEXT("This canonical battle unit is already embodied elsewhere.");
            return false;
        }
    // Existing native owned Luminous has stricter Registry/manifestation
    // checks. Do not bypass that through this actor visual binding.
    if (AOGLuminousV3PlayableCharacter* Luminous=
        Cast<AOGLuminousV3PlayableCharacter>(Owner))
    {
        // Owned Luminous cannot be rebound to a different unit merely by
        // passing a matching display Identity to this visual function.
        const UOGGate12RulerSessionSubsystem* Session=GI
            ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
        if (!Session || !Session->IsBoundToStoredRuler() ||
            !Luminous->IsBoundToOwnedLuminousCombatUnit(
                Session->BoundRulerId,UnitId,Unit->SkillSet.CurrentForm))
        {
            OutError=TEXT("Owned Luminous Registry/Manifestation binding differs from requested unit.");
            return false;
        }
    }
    if (const AOGContentPlayableCharacter* ContentCharacter = Cast<AOGContentPlayableCharacter>(Owner))
    {
        const UOGGate12RulerSessionSubsystem* Session = GI
            ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
        if (!Session || !Session->IsBoundToStoredRuler() ||
            !ContentCharacter->IsBoundToOwnedCombatUnit(Session->BoundRulerId, UnitId, Unit->SkillSet.CurrentForm))
        {
            OutError = TEXT("Content character Registry/Manifestation binding differs from requested unit.");
            return false;
        }
    }
    BoundBattleId=State.BattleId;
    BoundUnitId=UnitId;
    return true;
}

void UOGCanonicalCombatActorBindingComponent::UnbindVisual()
{
    BoundBattleId=FOGEntityId();
    BoundUnitId=FOGEntityId();
}

bool UOGCanonicalCombatActorBindingComponent::IsBoundTo(
    const FOGEntityId& BattleId, const FOGEntityId& UnitId) const
{
    return BattleId.IsValid() && UnitId.IsValid() &&
        BoundBattleId==BattleId && BoundUnitId==UnitId;
}

AActor* UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(
    UWorld* World, const FOGEntityId& BattleId, const FOGEntityId& UnitId)
{
    if (!World || !BattleId.IsValid() || !UnitId.IsValid())
        return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor=*It;
        if (!IsValid(Actor)) continue;
        const UOGCanonicalCombatActorBindingComponent* Binding=
            Actor->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
        if (Binding && Binding->IsBoundTo(BattleId,UnitId))
            return Actor;
    }
    return nullptr;
}

void UOGCanonicalCombatActorBindingComponent::EndPlay(
    const EEndPlayReason::Type Reason)
{
    UnbindVisual();
    Super::EndPlay(Reason);
}
