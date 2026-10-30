#include "UI/OGReactiveUiSubsystem.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Widgets/SWidget.h"
#include "UI/OGGate12GothicChromeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Rendering/DrawElements.h"

void UOGGate12GothicChromeWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree || WidgetTree->RootWidget) return;
    UBorder* Root = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),TEXT("EditableGothicChrome"));
    Root->SetBrushColor(FLinearColor(0.f,0.f,0.f,0.f));
    Root->SetVisibility(ESlateVisibility::HitTestInvisible);
    WidgetTree->RootWidget=Root;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UOGGate12GothicChromeWidget::NativeTick(const FGeometry& Geometry,float Delta)
{
    Super::NativeTick(Geometry,Delta);
    if (UOGReactiveUiSubsystem::Find(this))
    { auto Cached=GetCachedWidget(); if (Cached.IsValid()) Cached->Invalidate(EInvalidateWidgetReason::Paint); }
}
void UOGGate12GothicChromeWidget::Configure(
    const FLinearColor& InAccent, int32 InSigilIndex, bool bInSelected)
{
    Accent=InAccent;
    Sigil=InSigilIndex;
    bSelected=bInSelected;
    InvalidateLayoutAndVolatility();
}
int32 UOGGate12GothicChromeWidget::NativePaint(
    const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& Clip, FSlateWindowElementList& Elements,
    int32 Layer, const FWidgetStyle& Style, bool bEnabled) const
{
    const int32 Depth=Super::NativePaint(
        Args,Geometry,Clip,Elements,Layer,Style,bEnabled);
    const FVector2D S=Geometry.GetLocalSize();
    const float W=S.X, H=S.Y;
    if(W < 32.f || H < 24.f) return Depth;
    const auto* Theme=UOGReactiveUiSubsystem::Find(this);
    const auto LiveAccent=Theme?Theme->AccentFor(Accent):Accent;
    const float Glow=Theme?Theme->GetPalette().Glow:.12f;
    const float Pulse=Theme&&!Theme->IsMotionReduced()? .9f+.1f*FMath::Sin(Theme->GetEffectPhase()):1.f;
    bool Focused=bSelected,Pressed=false;
    if (const auto* Panel=Cast<UPanelWidget>(GetParent()))
        for (int32 I=0;I<Panel->GetChildrenCount();++I)
            if (const auto* Button=Cast<UButton>(Panel->GetChildAt(I)))
            { Focused|=Button->IsHovered()||Button->HasKeyboardFocus(); Pressed|=Button->IsPressed(); }
    FLinearColor Etch(.36f,.35f,.40f,.80f),Fine(.19f,.18f,.21f,.91f);
    if (Theme) { const auto Rim=Theme->GetPalette().Rim; Etch=FMath::Lerp(Etch,Rim,.65f); Etch.A=.8f; }
    const FLinearColor Highlight=Focused?LiveAccent*Pulse:FMath::Lerp(Etch,LiveAccent,.12f+Glow*.2f);
    // Low-opacity depth; the installed original artwork and hit targets remain separate.
    TArray<FSlateGradientStop> Stops;
    Stops.Emplace(FVector2D(0,0),FLinearColor(.03f,.03f,.05f,Pressed?.10f:.035f));
    Stops.Emplace(FVector2D(0,H),FLinearColor(0,0,0,.08f));
    FSlateDrawElement::MakeGradient(Elements,Depth+1,Geometry.ToPaintGeometry(),Stops,Orient_Vertical);
    TArray<FVector2D> Segment;
    Segment.Reserve(2);
    auto Stroke=[&](float Ax,float Ay,float Bx,float By,
                    FLinearColor Ink,float Width=1.f)
    {
        Segment.Reset();
        Segment.Add(FVector2D(Ax,Ay));Segment.Add(FVector2D(Bx,By));
        if (Focused&&Width>1.f)
        {
            auto Halo=Ink; Halo.A*=FMath::Clamp(Glow,0.f,1.f)*.13f;
            FSlateDrawElement::MakeLines(Elements,Depth+2,Geometry.ToPaintGeometry(),Segment,ESlateDrawEffect::None,Halo,true,Width+4.f);
        }
        FSlateDrawElement::MakeLines(Elements,Depth+3,
            Geometry.ToPaintGeometry(),Segment,
            ESlateDrawEffect::None,Ink,true,Pressed?Width+.35f:Width);
    };
    // Authentic editable bevel geometry: lines, cut angles and layered ribs.
    const float M=FMath::Min(10.f,FMath::Min(W,H)*.16f);
    Stroke(M,1.f,W-M,1.f,Highlight,1.35f);
    Stroke(W-M,1.f,W-1.f,M,Etch);
    Stroke(W-1.f,M,W-1.f,H-M,Etch);
    Stroke(W-1.f,H-M,W-M,H-1.f,Highlight);
    Stroke(W-M,H-1.f,M,H-1.f,Highlight,1.2f);
    Stroke(M,H-1.f,1.f,H-M,Etch);
    Stroke(1.f,H-M,1.f,M,Etch);
    Stroke(1.f,M,M,1.f,Etch);
    Stroke(M+5.f,5.f,W-M-5.f,5.f,Fine,.75f);
    Stroke(M+5.f,H-5.f,W-M-5.f,H-5.f,Fine,.75f);

    auto Diamond=[&](float Cx,float Cy,float Radius,
                     FLinearColor Color)
    {
        Stroke(Cx,Cy-Radius,Cx+Radius*.62f,Cy,Color);
        Stroke(Cx+Radius*.62f,Cy,Cx,Cy+Radius,Color);
        Stroke(Cx,Cy+Radius,Cx-Radius*.62f,Cy,Color);
        Stroke(Cx-Radius*.62f,Cy,Cx,Cy-Radius,Color);
    };
    // Four corner compass thorns and two small rivets.
    Diamond(M+1.f,M+1.f,5.f,Highlight);
    Diamond(W-M-1.f,M+1.f,5.f,Highlight);
    Diamond(M+1.f,H-M-1.f,5.f,Highlight);
    Diamond(W-M-1.f,H-M-1.f,5.f,Highlight);

    const float Mid=W*.5f;
    if (Sigil!=INDEX_NONE)
    {
        const float Y=FMath::Clamp(H*.29f,17.f,H*.52f);
        const float Radius=FMath::Min(H*.21f,W*.15f);
        // Vary insignia silhouette by fixed destination identity, but never
        // imply a fabricated character, location or faction emblem.
        switch(Sigil%5)
        {
        case 0: // Home: four-point astrolabe
            Diamond(Mid,Y,Radius,Highlight);
            Stroke(Mid,Y-Radius*1.35f,Mid,Y+Radius*1.35f,Highlight,1.3f);
            Stroke(Mid-Radius,Y,Mid+Radius,Y,Etch);
            break;
        case 1: // Characters: split crest
            Diamond(Mid,Y,Radius,Highlight);
            Diamond(Mid,Y,Radius*.52f,Highlight);
            Stroke(Mid,Y-Radius,Mid,Y+Radius,Highlight);
            break;
        case 2: // Gacha: radiating five-star
            Diamond(Mid,Y,Radius,Highlight);
            Stroke(Mid-Radius*.95f,Y-Radius*.80f,
                   Mid+Radius*.95f,Y+Radius*.80f,Highlight);
            Stroke(Mid+Radius*.95f,Y-Radius*.80f,
                   Mid-Radius*.95f,Y+Radius*.80f,Highlight);
            break;
        case 3: // Territory: sharpened tower geometry
            Stroke(Mid,Y-Radius,Mid,Y+Radius,Highlight,1.5f);
            Stroke(Mid-Radius*.65f,Y-Radius*.3f,
                   Mid,Y-Radius,Highlight);
            Stroke(Mid+Radius*.65f,Y-Radius*.3f,
                   Mid,Y-Radius,Highlight);
            Stroke(Mid-Radius*.6f,Y+Radius*.62f,
                   Mid+Radius*.6f,Y+Radius*.62f,Highlight);
            break;
        case 4: // Records: double archivist frame
            Diamond(Mid,Y,Radius,Highlight);
            Stroke(Mid-Radius*.38f,Y-Radius*.45f,
                   Mid+Radius*.38f,Y-Radius*.45f,Etch);
            Stroke(Mid-Radius*.38f,Y,
                   Mid+Radius*.38f,Y,Highlight);
            break;
        }
        Stroke(Mid,Y-Radius*1.65f,Mid,Y-Radius*1.22f,
            Highlight,.8f);
        Stroke(Mid,Y+Radius*1.22f,Mid,Y+Radius*1.65f,
            Highlight,.8f);
    }
    return Depth+3;
}
