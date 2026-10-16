#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/OGEntityId.h"
#include "Content/OGContentId.h"
#include "OGGate12CharacterPreviewStage.generated.h"

class USceneComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class USkeletalMeshComponent;
class UPointLightComponent;

/** Off-map isolated presentation stage. Its input must be an authored
 * non-Pawn actor containing the real imported modular skeletal meshes.
 * Stage and capture never acquire an owned Manifestation or alter saves.
 */
UCLASS(NotPlaceable)
class OFFLINEGAME_API AOGGate12CharacterPreviewStage : public AActor
{
    GENERATED_BODY()
public:
    AOGGate12CharacterPreviewStage();

    bool InitializeApprovedPreview(TSubclassOf<AActor> PreviewClass,
        const FOGEntityId& RulerId,
        const FOGContentId& IdentityId,
        const FOGContentId& VersionId,
        const FOGEntityId& ManifestationId,
        bool bSfwPresentation);

    /** Development-only visual inspection of the exact existing imported
     * Luminous QA Blueprint. It is NEVER an owned character, never a Registry
     * entry, never a production preview, and has no persistence side effects.
     * Disabled in shipping builds. */
    bool InitializeDevelopmentQaPreview(UClass* SourceQaClass);

    /** Only this existing QA class is permitted in the noncanonical viewer. */
    static bool IsDevelopmentQaClassAllowed(const UClass* SourceQaClass);

    UTextureRenderTarget2D* GetLiveRenderTarget() const { return RenderTarget; }
    /** Actual transient imported mesh components installed for QA; no synthetic actor. */
    int32 GetDevelopmentQaSourcePartCount() const { return DevelopmentQaMeshParts.Num(); }
    /** Freeze a roster thumbnail to cap Android scene-capture frame cost. */
    void SetLiveCapture(bool bLive);
    void ReleasePreview();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> StageRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneCaptureComponent2D> Capture;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

    UPROPERTY(Transient)
    TObjectPtr<AActor> PreviewActor;

    /** Passive imported geometry in development viewer; never a gameplay Pawn
     * or a new canonical Manifestation. Cleared when viewer closes. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> DevelopmentQaMeshParts;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UPointLightComponent>> DevelopmentQaLights;
};
