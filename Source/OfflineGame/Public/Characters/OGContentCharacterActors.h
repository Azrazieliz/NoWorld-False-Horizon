#pragma once

#include "CoreMinimal.h"
#include "World/OGStartingRegionPresentation.h"
#include "UI/OGGate12OwnedWorldCharacterBinding.h"
#include "UI/OGGate12CharacterPreviewConfiguration.h"
#include "OGContentCharacterActors.generated.h"

class UOGCharacterAppearanceComponent;
class UOGSharedMotionControllerComponent;
class UOGCanonicalCombatActorBindingComponent;
class UOGGameCoreSubsystem;
class UOGSystemBindingComponent;
struct FOGResolvedCombatAction;

/** Content Blueprint supplies appearance definitions and visual action maps.
 * The inherited World pawn continues to own movement, camera and touch input. */
UCLASS(Blueprintable)
class OFFLINEGAME_API AOGContentPlayableCharacter : public AOGWorldPrototypeCharacter,
    public IOGGate12OwnedWorldCharacterBinding
{
    GENERATED_BODY()
public:
    AOGContentPlayableCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character|Appearance")
    TObjectPtr<UOGCharacterAppearanceComponent> Appearance;

    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Character|Systems")
    TObjectPtr<UOGSystemBindingComponent> SystemBindings;

    /** Exact approved skill ID -> shared source motion semantic. Missing
     * presentation never executes a substitute skill or alters damage. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character|Combat")
    TMap<FOGContentId, FName> CombatActionSemantics;

    /** Zero disables Run selection until the authored locomotion threshold exists. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character|Motion", meta=(ClampMin="0"))
    float RunAnimationSpeedCmPerSecond = 0.f;

    virtual bool BindOwnedWorldCharacter_Implementation(const FOGEntityId& RulerId,
        const FOGContentId& IdentityId, const FOGContentId& VersionId,
        const FOGEntityId& ManifestationId, bool bSfwPresentation) override;
    bool IsBoundToOwnedCombatUnit(const FOGEntityId& Ruler,
        const FOGEntityId& Manifestation, const FOGContentId& Version) const;

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Character|Combat")
    TObjectPtr<UOGCanonicalCombatActorBindingComponent> CombatBody;
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    TWeakObjectPtr<UOGSharedMotionControllerComponent> LastMotionController;
    FOGEntityId BoundManifestation;
    FOGEntityId BoundRuler;
    FOGContentId BoundIdentity;
    FOGContentId BoundVersion;
    FName FailedTraversalAction = NAME_None;
    FOGEntityId LastPresentedActionId;
    void HandleResolvedCombatAction(const FOGResolvedCombatAction& Action);
};

/** Non-gameplay Actor for the existing live roster capture. Content only needs
 * a Blueprint subclass containing the same appearance definition as its pawn. */
UCLASS(Blueprintable)
class OFFLINEGAME_API AOGContentCharacterPreviewActor : public AActor,
    public IOGGate12CharacterPreviewConfiguration
{
    GENERATED_BODY()
public:
    AOGContentCharacterPreviewActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character|Appearance")
    TObjectPtr<UOGCharacterAppearanceComponent> Appearance;

    virtual bool ConfigureOwnedPreview_Implementation(const FOGEntityId& RulerId,
        const FOGContentId& IdentityId, const FOGContentId& VersionId,
        const FOGEntityId& ManifestationId, bool bSfwPresentation) override;
};
