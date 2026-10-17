#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OGGate12GothicChromeWidget.generated.h"

/** Reusable native geometry for the frozen angular gothic UI chrome.
 *  Decorative only: no hit targets, character/art pixels, data or save state.
 */
UCLASS()
class OFFLINEGAME_API UOGGate12GothicChromeWidget final : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(const FLinearColor& InAccent, int32 InSigilIndex,
                   bool bInSelected);
protected:
    virtual void NativeOnInitialized() override;
    virtual int32 NativePaint(
        const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId,
        const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
private:
    FLinearColor Accent = FLinearColor(.69f,.66f,.72f,1.f);
    int32 Sigil = INDEX_NONE;
    bool bSelected = false;
};
