#include "UI/OGNativeUiStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
void FOGNativeUiStyle::SkinButton(UButton& Button)
{
    auto Style=Button.GetStyle();
    FSlateBrush Empty; Empty.DrawAs=ESlateBrushDrawType::NoDrawType;
    Style.SetNormal(Empty);Style.SetHovered(Empty);Style.SetPressed(Empty);Style.SetDisabled(Empty);
    Button.SetStyle(Style); Button.SetBackgroundColor(FLinearColor::Transparent);
}
UOverlay* FOGNativeUiStyle::Surface(UWidgetTree& Tree,UWidget* Content,EOGNativeSurface Kind,bool Selected,int32 Sigil)
{
    auto* Layers=Tree.ConstructWidget<UOverlay>();
    auto* Back=Tree.ConstructWidget<UOGGate12GothicChromeWidget>();
    Back->Configure(FLinearColor(.72f,.62f,.44f,1),Sigil,Selected); Back->SetSurface(Kind,true);
    // Back searches its actual sibling control for pressed, focus, disabled and hover state.
    auto* Art=Layers->AddChildToOverlay(Back);Art->SetHorizontalAlignment(HAlign_Fill);Art->SetVerticalAlignment(VAlign_Fill);
    if (Content)
    { auto* Foreground=Layers->AddChildToOverlay(Content); Foreground->SetHorizontalAlignment(HAlign_Fill);Foreground->SetVerticalAlignment(VAlign_Fill); }
    Layers->SetClipping(EWidgetClipping::ClipToBounds);
    return Layers;
}
