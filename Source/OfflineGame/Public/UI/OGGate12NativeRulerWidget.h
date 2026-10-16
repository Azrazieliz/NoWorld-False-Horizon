#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "UI/OGUiViewModels.h"
#include "OGGate12NativeRulerWidget.generated.h"

class UVerticalBox;
class UHorizontalBox;
class UOGGate12NativeRulerWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOGGate12WorldPresentationRequested);

/** Native UMG action element; no transparent hotspots over screenshots. */
UCLASS()
class OFFLINEGAME_API UOGGate12RulerActionButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient)
    TObjectPtr<UOGGate12NativeRulerWidget> RulerWidget;

    UPROPERTY(Transient)
    FString ActionId;

    UFUNCTION()
    void InvokeAction();
};

/**
 * Gate12 native Ruler presentation foundation.
 *
 * Built from real UMG widgets, reads the canonical Gate12 session, and keeps
 * UI transient. Gate10 artwork/material integration can replace this class
 * without altering gameplay data or persistence.
 *
 * Three exactly uniform tall roster columns (width/height 0.445), one tile
 * per real owned Identity; repeat Manifestations remain inside the detail.
 * No synthetic Ruler, gacha banner, currency, chip or character.
 */
UCLASS(Blueprintable)
class OFFLINEGAME_API UOGGate12NativeRulerWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category="OfflineGame|Ruler|Native")
    FOGGate12WorldPresentationRequested OnWorldPresentationRequested;

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Native")
    void RefreshFromCanonical();

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Native")
    void InvokeAction(const FString& Action);

    UFUNCTION(BlueprintPure, Category="OfflineGame|Ruler|Native")
    static int32 GetApprovedRosterColumns() { return 3; }

    UFUNCTION(BlueprintPure, Category="OfflineGame|Ruler|Native")
    static float GetApprovedRosterCardRatio() { return 0.445f; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Screen;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Content;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> NavigationBar;

    int32 OpenIdentityIndex = INDEX_NONE;
    // UI preference only: never changes wardrobe, ownership, or world state.
    bool bPrefer3DCharacterPresentation = false;
    int32 RecordsSection = 0;
    bool bGachaHistoryExpanded = false;
    FOGEntityId SelectedTerritoryId;
    int32 SelectedTerritoryOverlayIndex = 0;
    FString LastUiError;

    void BuildRoot();
    void BuildCurrentPage();
    void BuildRoster();
    void BuildCharacterDetail();
    void BuildRecords();
    void BuildTerritory();
    void BuildNav();
    void AddMessage(UVerticalBox* Parent, const FString& Value, int32 FontSize = 22,
        FLinearColor Tint = FLinearColor(0.83f, 0.84f, 0.88f, 1.0f));
    void AddAction(UVerticalBox* Parent, const FString& Title, const FString& Action,
        bool bEnabled = true);
};
