#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/OGGate12CharacterPreviewConfiguration.h"
#include "OGLuminousExactOriginalPreviewActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UOGSharedMotionControllerComponent;

/** Source-authentic 3D viewer; not a pawn or an owned gameplay identity.
 * It displays exactly the canonical Ruler's manifested approved card form.
 */
UCLASS()
class OFFLINEGAME_API AOGLuminousExactOriginalPreviewActor final
    : public AActor, public IOGGate12CharacterPreviewConfiguration
{
    GENERATED_BODY()
public:
    AOGLuminousExactOriginalPreviewActor();

    virtual bool ConfigureOwnedPreview_Implementation(
        const FOGEntityId& RulerId, const FOGContentId& IdentityId,
        const FOGContentId& VersionId, const FOGEntityId& ManifestationId,
        bool bSfwPresentation) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> NativeRoot;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UOGSharedMotionControllerComponent> Motion;
    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> Parts;
};
