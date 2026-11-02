#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OGGate12VacantThroneSceneWidget.generated.h"

/**
 * Authoritative opening 04: animated native scene beneath real UMG controls.
 * All architectural linework is vector Slate, not a design reference PNG,
 * fictional character art, or a pre-rendered interaction surface.
 */
UCLASS()
class OFFLINEGAME_API UOGGate12VacantThroneSceneWidget final : public UUserWidget
{
    GENERATED_BODY()
public:
    void UpdateAtmosphere(float TimeSeconds, bool bInReducedMotion);
protected:
    virtual void NativeOnInitialized() override;
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,
        const FSlateRect&,FSlateWindowElementList&,int32,
        const FWidgetStyle&,bool) const override;
private:
    float AtmosphereSeconds=0.f;
    bool bReducedMotion=false;
};
