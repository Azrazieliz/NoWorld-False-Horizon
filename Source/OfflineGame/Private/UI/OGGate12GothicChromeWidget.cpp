#include "UI/OGGate12GothicChromeWidget.h"
#include "UI/OGReactiveUiSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SWidget.h"

void UOGGate12GothicChromeWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree||WidgetTree->RootWidget) return;
    auto* Root=WidgetTree->ConstructWidget<UBorder>();
    Root->SetBrushColor(FLinearColor::Transparent); WidgetTree->RootWidget=Root;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UOGGate12GothicChromeWidget::NativeTick(const FGeometry& Geometry,float Delta)
{
    Super::NativeTick(Geometry,Delta);
    // Paint-only changes preserve every real widget, hitbox, focus and scroll position.
    if (auto Cached=GetCachedWidget();Cached.IsValid()) Cached->Invalidate(EInvalidateWidgetReason::Paint);
}
void UOGGate12GothicChromeWidget::Configure(const FLinearColor& Color,int32 Index,bool Selected)
{
    Accent=Color; Sigil=Index; bSelected=Selected; InvalidateLayoutAndVolatility();
}
void UOGGate12GothicChromeWidget::SetSurface(EOGNativeSurface Kind,bool Opaque)
{
    Surface=Kind; bOpaque=Opaque; InvalidateLayoutAndVolatility();
}
int32 UOGGate12GothicChromeWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,
    const FSlateRect& Clip,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
    const int32 Base=Super::NativePaint(Args,Geometry,Clip,Elements,Layer,Style,Enabled);
    const FVector2D Size=Geometry.GetLocalSize(); const float W=Size.X,H=Size.Y;
    if (W<16||H<16) return Base;
    const auto* Driver=UOGReactiveUiSubsystem::Find(this);
    const FOGReactiveUiPalette Palette=Driver?Driver->GetPalette():FOGReactiveUiPalette();
    auto Metal=Driver?Driver->AccentFor(Accent,.85f):Accent;
    bool Hover=false,Pressed=false,Active=bSelected;
    if (auto* Parent=Cast<UPanelWidget>(GetParent()))
        for (int32 I=0;I<Parent->GetChildrenCount();++I)
            if (auto* Button=Cast<UButton>(Parent->GetChildAt(I)))
            { Enabled&=Button->GetIsEnabled(); Hover|=Button->IsHovered()||Button->HasKeyboardFocus(); Pressed|=Button->IsPressed(); }
    Active|=Hover; const float Opacity=Enabled?1.f:.38f;
    EOGNativeSurface Kind=Surface;
    if (Kind==EOGNativeSurface::Automatic)
        Kind=Sigil!=INDEX_NONE?EOGNativeSurface::Navigation:H<W*.42f?EOGNativeSurface::Action:EOGNativeSurface::Hero;
    const bool Primary=Kind==EOGNativeSurface::PrimaryAction;
    const bool Action=Primary||Kind==EOGNativeSurface::Action;
    const bool Navigation=Kind==EOGNativeSurface::Navigation;
    const float Breath=Driver&&!Driver->IsMotionReduced()? .96f+.04f*FMath::Sin(Driver->GetEffectPhase()):1.f;
    const auto SurfaceInk=FMath::Lerp(Palette.Surface,Metal,Pressed?.13f:Primary?.085f:Hover?.065f:.015f);
    auto Gradient=[&](FLinearColor Top,FLinearColor Bottom)
    {
        TArray<FSlateGradientStop> Stops;
        Stops.Emplace(FVector2D(0,0),Top); Stops.Emplace(FVector2D(0,H),Bottom);
        FSlateDrawElement::MakeGradient(Elements,Base+1,Geometry.ToPaintGeometry(),Stops,Orient_Vertical);
    };
    if (bOpaque)
    {
        auto Top=SurfaceInk; Top.A=(Action?.97f:.94f)*Opacity;
        auto Bottom=FMath::Lerp(SurfaceInk,FLinearColor::Black,.36f); Bottom.A=Top.A;
        Gradient(Top,Bottom);
    }
    else Gradient(FLinearColor(.07f,.07f,.09f,.045f),FLinearColor(0,0,0,.12f));
    auto Line=[&](float X,float Y,float X2,float Y2,FLinearColor Color,float Width=1.f)
    {
        Color.A*=Opacity;
        TArray<FVector2D,TInlineAllocator<2>> Vertices; Vertices.Add(FVector2D(X,Y));Vertices.Add(FVector2D(X2,Y2));
        // MakeLines takes a standard TArray; two points, no custom Slate resource or texture.
        TArray<FVector2D> Points; Points.Append(Vertices.GetData(),2);
        FSlateDrawElement::MakeLines(Elements,Base+3,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Width);
    };
    auto Rim=FMath::Lerp(Palette.Rim,Metal,Active?.72f:Primary?.5f:.12f); Rim.A=Active?.85f:.48f;
    auto Bright=Metal; Bright.A=(Active?.9f:Primary?.72f:.4f)*Breath;
    const float Cut=FMath::Min(9.f,H*.12f);
    if (Navigation)
    {
        Line(8,1,W-8,1,FLinearColor(Palette.Rim.R,Palette.Rim.G,Palette.Rim.B,.28f));
        if (bSelected) { Line(W*.28f,2,W*.72f,2,Bright,2.f); Line(W*.38f,H-3,W*.62f,H-3,Bright,2.f); }
    }
    else
    {
        Line(Cut,1,W-1,1,Rim); Line(W-1,1,W-1,H-Cut,Rim);
        Line(W-1,H-Cut,W-Cut,H-1,Rim); Line(W-Cut,H-1,1,H-1,Rim);
        Line(1,H-1,1,Cut,Rim); Line(1,Cut,Cut,1,Rim);
        // Sparse machined highlights replace repeated filigree on every control.
        Line(Cut+3,3,FMath::Min(W*.27f,110.f),3,Bright);
        if (Kind==EOGNativeSurface::Hero) Line(W-3,H-Cut-3,W-3,FMath::Max(H*.74f,H-140.f),Bright);
    }
    if (Action&&Primary)
    {
        auto Halo=Metal; Halo.A=.10f*Palette.Glow*Opacity;
        Line(12,H-3,W-12,H-3,Halo,6.f); Line(12,H-2,W-12,H-2,Bright,1.5f);
    }
    if (Hover)
    {
        auto Focus=Metal; Focus.A=.8f;
        Line(5,6,5,H-6,Focus,2.f); Line(W-5,6,W-5,H-6,Focus,2.f);
    }
    if (Navigation&&Sigil!=INDEX_NONE)
    {
        const float X=W*.5f,Y=H*.29f,R=FMath::Min(W*.12f,H*.15f);
        auto Icon=Active?Metal:FMath::Lerp(Palette.Muted,Metal,.12f); Icon.A=Active?1.f:.7f;
        auto Diamond=[&](float Radius)
        { Line(X,Y-Radius,X+Radius*.7f,Y,Icon,1.2f); Line(X+Radius*.7f,Y,X,Y+Radius,Icon,1.2f);
          Line(X,Y+Radius,X-Radius*.7f,Y,Icon,1.2f); Line(X-Radius*.7f,Y,X,Y-Radius,Icon,1.2f); };
        switch (Sigil%5)
        {
        case 0: Diamond(R);Line(X-R,Y,X+R,Y,Icon);Line(X,Y-R*1.3f,X,Y+R*1.3f,Icon);break;
        case 1: Diamond(R);Diamond(R*.48f);break;
        case 2: Diamond(R);Line(X-R,Y-R*.7f,X+R,Y+R*.7f,Icon);Line(X+R,Y-R*.7f,X-R,Y+R*.7f,Icon);break;
        case 3: Line(X,Y-R,X,Y+R,Icon,1.5f);Line(X-R*.7f,Y-R*.3f,X,Y-R,Icon);Line(X+R*.7f,Y-R*.3f,X,Y-R,Icon);Line(X-R*.6f,Y+R*.6f,X+R*.6f,Y+R*.6f,Icon);break;
        case 4: Diamond(R);Line(X-R*.35f,Y-R*.3f,X+R*.35f,Y-R*.3f,Icon);Line(X-R*.35f,Y+R*.15f,X+R*.35f,Y+R*.15f,Icon);break;
        }
    }
    return Base+3;
}
