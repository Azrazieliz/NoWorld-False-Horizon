#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OGSharedMotionSceneActor.generated.h"

class UOGSharedMotionSceneComponent;
class USceneComponent;

/** Gameplay-placeable entry point for genuine multi-participant native motion scenes. */
UCLASS(Blueprintable)
class OFFLINEGAME_API AOGSharedMotionSceneActor : public AActor
{
    GENERATED_BODY()
public:
    AOGSharedMotionSceneActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Motion|Scene")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Motion|Scene")
    TObjectPtr<UOGSharedMotionSceneComponent> Scene;

    UFUNCTION(BlueprintCallable, Category="Motion|Scene")
    bool PlayNativeScene(FName Action, FName Context, bool bAdultAuthorized);

    UFUNCTION(BlueprintCallable, Category="Motion|Scene")
    void CancelNativeScene();
};
