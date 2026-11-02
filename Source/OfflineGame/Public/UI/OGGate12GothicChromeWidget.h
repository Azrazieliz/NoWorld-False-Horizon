#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OGGate12GothicChromeWidget.generated.h"

UENUM()
enum class EOGNativeSurface : uint8 { Automatic,Panel,Action,PrimaryAction,Hero,Navigation };
/** Native presentation geometry. Original art, hit targets and gameplay stay independent. */
UCLASS()
class OFFLINEGAME_API UOGGate12GothicChromeWidget final : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(const FLinearColor& InAccent,int32 InSigilIndex,bool bInSelected);
    void SetSurface(EOGNativeSurface InSurface,bool bInOpaque=true);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry&,float) override;
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,
        FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
private:
    FLinearColor Accent=FLinearColor(.72f,.62f,.44f,1);
    EOGNativeSurface Surface=EOGNativeSurface::Automatic;
    int32 Sigil=INDEX_NONE;
    bool bSelected=false,bOpaque=false;
};
