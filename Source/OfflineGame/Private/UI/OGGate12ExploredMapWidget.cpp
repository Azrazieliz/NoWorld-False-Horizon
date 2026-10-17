#include "UI/OGGate12ExploredMapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

void UOGGate12ExploredMapWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree || WidgetTree->RootWidget) return;
    UBorder* Black = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("ExploredWorldTrueBlackMap"));
    Black->SetBrushColor(FLinearColor(.006f,.006f,.010f,1.f));
    WidgetTree->RootWidget = Black;
}

void UOGGate12ExploredMapWidget::SetKnownWorldTrail(
    const TArray<FVector2D>& WorldPoints,
    const FVector2D& ActualPlayerPosition)
{
    KnownTrail = WorldPoints;
    KnownPlayer = ActualPlayerPosition;
    bHasKnownPlayer = true;
    InvalidateLayoutAndVolatility();
}

void UOGGate12ExploredMapWidget::StepZoom(float ZoomFactor)
{
    Zoom = FMath::Clamp(Zoom * ZoomFactor, .40f, 8.f);
    InvalidateLayoutAndVolatility();
}

void UOGGate12ExploredMapWidget::RecenterView()
{
    ViewOffset = FVector2D::ZeroVector;
    Zoom = 1.f;
    InvalidateLayoutAndVolatility();
}

int32 UOGGate12ExploredMapWidget::NativePaint(
    const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& CullingRect, FSlateWindowElementList& DrawElements,
    int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
    const int32 Base = Super::NativePaint(
        Args, Geometry, CullingRect, DrawElements, LayerId, Style, bParentEnabled);
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.X < 32.f || Size.Y < 32.f) return Base;
    const FVector2D Center = Size * .5f + ViewOffset;
    // Geographic content is unknown until actually discovered. Render only
    // a nonsemantic projected coordinate grid, not fabricated continents,
    // locations, ownership or a baked concept-map screenshot.
    const float Spacing = FMath::Max(36.f, 106.f * Zoom);
    const FLinearColor GridInk(.42f,.40f,.49f,.67f);
    for (float X = FMath::Fmod(Center.X,Spacing); X <= Size.X; X += Spacing)
    {
        FSlateDrawElement::MakeLines(DrawElements,Base+1,
            Geometry.ToPaintGeometry(),
            {FVector2D(X,0.f),FVector2D(X,Size.Y)},
            ESlateDrawEffect::None,GridInk,true,.7f);
    }
    for (float Y = FMath::Fmod(Center.Y,Spacing); Y <= Size.Y; Y += Spacing)
    {
        FSlateDrawElement::MakeLines(DrawElements,Base+1,
            Geometry.ToPaintGeometry(),
            {FVector2D(0.f,Y),FVector2D(Size.X,Y)},
            ESlateDrawEffect::None,GridInk,true,.7f);
    }
    if (KnownTrail.IsEmpty() && !bHasKnownPlayer) return Base+1;
    const float WorldToPixel = FMath::Min(Size.X,Size.Y) * Zoom / 10000.f;
    TArray<FVector2D> Points;
    Points.Reserve(KnownTrail.Num());
    for (const FVector2D& Location : KnownTrail)
    {
        const FVector2D Screen(
            Center.X + (Location.X - KnownPlayer.X) * WorldToPixel,
            Center.Y - (Location.Y - KnownPlayer.Y) * WorldToPixel);
        // Don't draw speculative points outside actual map bounds.
        if (Screen.X > -64.f && Screen.X < Size.X + 64.f &&
            Screen.Y > -64.f && Screen.Y < Size.Y + 64.f)
            Points.Add(Screen);
    }
    if (Points.Num() >= 2)
        FSlateDrawElement::MakeLines(
            DrawElements, Base+1, Geometry.ToPaintGeometry(),
            Points, ESlateDrawEffect::None,
            FLinearColor(.73f,.69f,.81f,.90f),true,1.75f);
    // Only the actual current player position is used as a center marker.
    static const FSlateColorBrush CurrentMarker(
        FLinearColor(.87f,.78f,.54f,1.f));
    if (bHasKnownPlayer)
    {
        const FVector2D MarkerSize(8.f,8.f);
        FSlateDrawElement::MakeBox(
            DrawElements, Base+2,
            Geometry.ToPaintGeometry(
                FVector2f(MarkerSize),
                FSlateLayoutTransform(FVector2f(Center - MarkerSize*.5f))),
            &CurrentMarker, ESlateDrawEffect::None);
    }
    return Base+2;
}

FReply UOGGate12ExploredMapWidget::NativeOnMouseButtonDown(
    const FGeometry&, const FPointerEvent& Event)
{
    if (!Event.GetEffectingButton().IsMouseButton() ||
        Event.GetEffectingButton()!=EKeys::LeftMouseButton)
        return FReply::Unhandled();
    bDragging=true;
    LastPointer=Event.GetScreenSpacePosition();
    return FReply::Handled();
}
FReply UOGGate12ExploredMapWidget::NativeOnMouseMove(
    const FGeometry&, const FPointerEvent& Event)
{
    if (!bDragging) return FReply::Unhandled();
    const FVector2D Current=Event.GetScreenSpacePosition();
    ViewOffset+=Current-LastPointer;
    LastPointer=Current;
    InvalidateLayoutAndVolatility();
    return FReply::Handled();
}
FReply UOGGate12ExploredMapWidget::NativeOnMouseButtonUp(
    const FGeometry&,const FPointerEvent&)
{
    if (!bDragging) return FReply::Unhandled();
    bDragging=false;
    return FReply::Handled();
}
FReply UOGGate12ExploredMapWidget::NativeOnMouseWheel(
    const FGeometry&, const FPointerEvent& Event)
{
    StepZoom(Event.GetWheelDelta()>0.f?1.25f:.8f);
    return FReply::Handled();
}
FReply UOGGate12ExploredMapWidget::NativeOnTouchStarted(
    const FGeometry&,const FPointerEvent& Event)
{
    ActiveTouches.Add(Event.GetPointerIndex(),
                      Event.GetScreenSpacePosition());
    bDragging=ActiveTouches.Num()==1;
    LastPointer=Event.GetScreenSpacePosition();
    return FReply::Handled();
}
FReply UOGGate12ExploredMapWidget::NativeOnTouchMoved(
    const FGeometry&,const FPointerEvent& Event)
{
    FVector2D* Prior=ActiveTouches.Find(Event.GetPointerIndex());
    if (!Prior) return FReply::Unhandled();
    const FVector2D Current=Event.GetScreenSpacePosition();
    if (ActiveTouches.Num()==2)
    {
        for (const TPair<int32,FVector2D>& Other : ActiveTouches)
        {
            if (Other.Key==Event.GetPointerIndex()) continue;
            const float Before=FVector2D::Distance(*Prior,Other.Value);
            const float After=FVector2D::Distance(Current,Other.Value);
            if (Before>8.f && After>8.f)
                StepZoom(FMath::Clamp(After/Before,.66f,1.5f));
            break;
        }
        bDragging=false;
    }
    else if (ActiveTouches.Num()==1)
    {
        ViewOffset+=Current-*Prior;
        bDragging=true;
        InvalidateLayoutAndVolatility();
    }
    *Prior=Current;
    LastPointer=Current;
    return FReply::Handled();
}
FReply UOGGate12ExploredMapWidget::NativeOnTouchEnded(
    const FGeometry&,const FPointerEvent& Event)
{
    ActiveTouches.Remove(Event.GetPointerIndex());
    bDragging=ActiveTouches.Num()==1;
    if (bDragging)
        for (const TPair<int32,FVector2D>& Touch : ActiveTouches)
        {
            LastPointer=Touch.Value;
            break;
        }
    return FReply::Handled();
}
