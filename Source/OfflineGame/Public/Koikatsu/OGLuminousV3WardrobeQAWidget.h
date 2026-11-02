#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OGLuminousV3WardrobeQAWidget.generated.h"

class AOGLuminousV3PlayableCharacter;
class UButton;
class UTextBlock;

/** Temporary real mobile-interactive engineering control only; Gate10 retains visual UI authority. */
UCLASS()
class OFFLINEGAME_API UOGLuminousV3WardrobeQAWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetTargetCharacter(AOGLuminousV3PlayableCharacter* InCharacter);

protected:
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<AOGLuminousV3PlayableCharacter> TargetCharacter;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NativePreviewLabel;

    TArray<FName> NativePreviewIds;
    TArray<FString> NativePreviewNames;
    int32 NativePreviewIndex = 0;

    void RefreshNativeMotionPreview();
    UFUNCTION() void OnPreviousNativeMotionClicked();
    UFUNCTION() void OnPlayNativeMotionClicked();
    UFUNCTION() void OnNextNativeMotionClicked();

    UFUNCTION() void OnClothedClicked();
    UFUNCTION() void OnUnderwearClicked();
    UFUNCTION() void OnNudeClicked();
    UFUNCTION() void OnThoughtfulClicked();
    UFUNCTION() void OnGreetingClicked();
    UFUNCTION() void OnExpressiveClicked();
    UFUNCTION() void OnBowClicked();

    void ChangeWardrobe(int32 StateIndex);
    void PlayAuthenticEmote(int32 EmoteIndex);
};
