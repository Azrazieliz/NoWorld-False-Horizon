#pragma once

#include "CoreMinimal.h"
#include "Core/OGEntityId.h"
#include "GameFramework/Actor.h"
#include "World/OGWorldModeInterfaces.h"
#include "OGNarrativeWorldActor.generated.h"

/** Authored placed/streamed interaction body for an EXISTING canonical subject.
 * Content supplies visuals and IDs; this actor never creates NPCs or story facts. */
UCLASS(Blueprintable)
class OFFLINEGAME_API AOGNarrativeWorldActor : public AActor, public IOGWorldInteractable
{
    GENERATED_BODY()
public:
    AOGNarrativeWorldActor();
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Narrative") FOGEntityId CanonicalSubjectId;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Narrative") FOGEntityId CanonicalLocationId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Narrative") FText InteractionPrompt;
    /** Content must author the real interaction distance; zero disables interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Narrative", meta=(ClampMin="0")) float InteractionRangeCm = 0.f;
    virtual bool CanInteract_Implementation(AActor* Interactor) const override;
    virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
    virtual void Interact_Implementation(AActor* Interactor) override;
};
