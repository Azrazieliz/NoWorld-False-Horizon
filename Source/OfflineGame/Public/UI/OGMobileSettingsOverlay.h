#pragma once

#include "Blueprint/UserWidget.h"
#include "OGMobileSettingsOverlay.generated.h"

class UButton;
class UBorder;
class UTextBlock;
struct FOGSemanticCaptionProjection;

/** Local Settings entry and authored captions above the native presentation. */
UCLASS()
class UOGMobileSettingsOverlay : public UUserWidget
{
    GENERATED_BODY()

public:
    void UpdatePresentation(bool bShowSettings, const FOGSemanticCaptionProjection& Caption);

protected:
    virtual void NativeOnInitialized() override;

private:
    UFUNCTION() void OpenSettings();

    UPROPERTY(Transient) TObjectPtr<UButton> SettingsButton;
    UPROPERTY(Transient) TObjectPtr<UBorder> CaptionPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CaptionLine;
    float CaptionFontSize = 0.f;
    bool bContrastCaption = false;
};
