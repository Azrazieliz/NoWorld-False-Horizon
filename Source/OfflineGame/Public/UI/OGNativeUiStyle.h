#pragma once
#include "CoreMinimal.h"
#include "UI/OGGate12GothicChromeWidget.h"
class UWidgetTree; class UWidget; class UOverlay; class UButton;
/** Shared native surface construction. No database, input dispatch or art substitution. */
struct OFFLINEGAME_API FOGNativeUiStyle
{
    static UOverlay* Surface(UWidgetTree& Tree,UWidget* Content,EOGNativeSurface Kind=EOGNativeSurface::Panel,
        bool Selected=false,int32 Sigil=INDEX_NONE);
    static void SkinButton(UButton& Button);
};
