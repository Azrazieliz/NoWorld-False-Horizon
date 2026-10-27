#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "OGNativeInteractionWidget.generated.h"

class UOGFoundationInteractionHost;
class UOGNativeInteractionWidget;
class UVerticalBox;
class UTextBlock;

UCLASS()
class OFFLINEGAME_API UOGInteractionActionButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) TObjectPtr<UOGNativeInteractionWidget> Screen;
    FName Action;
    UFUNCTION() void Invoke();
};

DECLARE_MULTICAST_DELEGATE(FOGNativeInteractionClosed);

/** Real responsive UMG controls over the canonical native interaction Host. */
UCLASS()
class OFFLINEGAME_API UOGNativeInteractionWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void BindHost(UOGFoundationInteractionHost* InHost);
    void Activate(FName Action);
    FOGNativeInteractionClosed OnClosed;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
private:
    void Refresh();
    void AddButton(FName Action, const FString& Label, bool Enabled);
    TWeakObjectPtr<UOGFoundationInteractionHost> Host;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Actions;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
    FString RowsSignature;
    float RefreshElapsed = 0;
};
