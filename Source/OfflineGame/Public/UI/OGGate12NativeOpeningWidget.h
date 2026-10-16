#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "OGGate12NativeOpeningWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UVerticalBox;
class USizeBox;
class UTextBlock;
class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOGGate12NativeOpeningAction, FName, Action);

/**
 * Source-native, data-driven opening controls. The frozen Gate10 direction 04
 * Vacant Throne requires separate authored scene/animation assets before the
 * opening can ever be visually accepted. Do NOT use the design-reference PNG
 * as clickable game UI or display fictional saved-world data.
 */
UCLASS()
class OFFLINEGAME_API UOGGate12NativeOpeningWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category="NoWorld|Opening")
    FOGGate12NativeOpeningAction OnOpeningAction;

    UFUNCTION(BlueprintCallable, Category="NoWorld|Opening")
    void RefreshOpening(bool bExistingOwnedRuler, bool bHasWorldSave,
                        bool bRecoverable, bool bImportAvailable,
                        bool bReducedMotion, FName CurrentPage);

    UFUNCTION()
    void DispatchNativeAction(FName Action);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildRoot();
    void RebuildMenu();
    void AddAction(const FString& Caption, FName Action,
                   bool bEnabled, bool bPrimary=false);
    bool bHasRuler = false;
    bool bHasSave = false;
    bool bHasRecovery = false;
    bool bHasImport = false;
    bool bReducedMotion = false;
    FName Page = FName(TEXT("main"));

    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Menu;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> MenuSlot;
    UPROPERTY(Transient) TObjectPtr<USizeBox> MenuWidthConstraint;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> TitleSlot;
    UPROPERTY(Transient) TObjectPtr<USizeBox> TitleWidthConstraint;
    UPROPERTY(Transient) TObjectPtr<UBorder> AtmosphericAccent;
};

UCLASS()
class OFFLINEGAME_API UOGGate12OpeningActionButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) TObjectPtr<UOGGate12NativeOpeningWidget> Opening;
    UPROPERTY() FName Action;
    UFUNCTION() void InvokeOpeningAction();
};
