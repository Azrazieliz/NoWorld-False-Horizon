#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OGGate12ExploredMapWidget.generated.h"

/**
 * Live spatial projection of locations ACTUALLY traversed by the World Pawn.
 * No illustrative map pixels, synthetic discovered sites or terrain claims.
 * Zoom/pan is viewport-only; it does not mutate saved gameplay state.
 */
UCLASS()
class OFFLINEGAME_API UOGGate12ExploredMapWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetKnownWorldTrail(
        const TArray<FVector2D>& WorldPoints,
        const FVector2D& ActualPlayerPosition);

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Territory")
    void StepZoom(float ZoomFactor);

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Territory")
    void RecenterView();

    UFUNCTION(BlueprintPure, Category="OfflineGame|Ruler|Territory")
    bool HasKnownTrail() const { return !KnownTrail.IsEmpty(); }

protected:
    virtual void NativeOnInitialized() override;
    virtual int32 NativePaint(
        const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(
        const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseWheel(
        const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnTouchStarted(
        const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnTouchMoved(
        const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnTouchEnded(
        const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
    UPROPERTY(Transient)
    TArray<FVector2D> KnownTrail;
    FVector2D KnownPlayer = FVector2D::ZeroVector;
    FVector2D ViewOffset = FVector2D::ZeroVector;
    FVector2D LastPointer = FVector2D::ZeroVector;
    float Zoom = 1.f;
    bool bDragging = false;
    TMap<int32, FVector2D> ActiveTouches;
};
