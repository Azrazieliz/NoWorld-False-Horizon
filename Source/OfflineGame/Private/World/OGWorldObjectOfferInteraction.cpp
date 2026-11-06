#include "World/OGStartingRegionPresentation.h"

#include "GameFramework/PlayerController.h"
#include "UI/OGGate12NativeRulerWidget.h"

bool AOGWorldPresentationHud::ShowNativeObjectOffers(const FOGEntityId& Counterparty, FString& OutError)
{
    OutError.Reset();
    if (!Counterparty.IsValid() || !AllowsWorldGameplayInput() || !bGate12NativeRulerUMGEnabled ||
        !PlayerOwner || !PlayerOwner->IsLocalController() || !PlayerOwner->GetLocalPlayer())
    {
        OutError = TEXT("World offers require the active local World player.");
        return false;
    }

    // Let the existing inventory surface review the exact canonical counterparty.
    // Keep the World surface active until the transaction UI accepts the context.
    SynchronizeGate12NativeRulerUMG(true);
    if (!IsValid(Gate12NativeRulerWidget) || !Gate12NativeRulerWidget->IsInViewport())
    {
        OutError = TEXT("The native offers screen could not be opened.");
        return false;
    }
    if (!Gate12NativeRulerWidget->OpenObjectActionsForCounterparty(Counterparty, OutError))
    {
        SynchronizeGate12NativeRulerUMG(false);
        return false;
    }

    SetFoundationSurface(TEXT("ruler"));
    return true;
}
