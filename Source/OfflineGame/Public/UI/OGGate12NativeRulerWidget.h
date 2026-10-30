#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Progression/OGCharacterDevelopmentService.h"
#include "UI/OGUiViewModels.h"
#include "World/OGOwnedEquipmentService.h"
#include "Narrative/OGNarrativeService.h"
#include "Objects/OGObjectActionDefinition.h"
#include "Gacha/OGGachaService.h"
#include "OGGate12NativeRulerWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UProgressBar;
class USizeBox;
class UCanvasPanel;
class UMaterialInstanceDynamic;
class UScaleBox;
class UOGGate12ExploredMapWidget;
class UImage;
class UTexture2D;
class UHorizontalBox;
class UUniformGridPanel;
class UOGGate12CharacterVisualCatalog;
class AOGGate12CharacterPreviewStage;
class UOGGate12RulerSessionSubsystem;
class UOGGate12NativeRulerWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOGGate12WorldPresentationRequested);

/** Native UMG action element; no transparent hotspots over screenshots. */
UCLASS()
class OFFLINEGAME_API UOGGate12RulerActionButton : public UButton
{
    GENERATED_BODY()
public:
    UOGGate12RulerActionButton(const FObjectInitializer& ObjectInitializer);

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

    /** Authored asset manifest; missing 2D art is allowed. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Ruler|Visuals")
    TObjectPtr<UOGGate12CharacterVisualCatalog> VisualCatalog;

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Visuals")
    void SetVisualCatalog(UOGGate12CharacterVisualCatalog* InCatalog);

    UOGGate12CharacterVisualCatalog* GetVisualCatalog() const { return VisualCatalog; }

    void ShowWorldTransitionError(const FString& Error);
    /** Existing NPC/world interactions can open this same native transaction interface. */
    UFUNCTION(BlueprintCallable,Category="OfflineGame|Ruler|Objects")
    bool OpenObjectActionsForCounterparty(const FOGEntityId& Counterparty,FString& Error);

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
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Screen;

    UPROPERTY(Transient)
    TObjectPtr<UImage> FrozenSceneArtwork;

    // Figma-authored interface chrome only; interactions remain native UMG.
    UPROPERTY(Transient)
    TObjectPtr<UImage> Gate10FigmaGachaChrome;

    bool bGachaChromeWasVisible = false;

    // Own the live character UI cutout for the full scene capture lifetime.
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> LivePreviewCutoutMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UScaleBox> Gate10PortraitGachaScale;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> Gate10PortraitGachaCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> LoadedSceneTexture;

    FString LoadedSceneSource;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> Content;

    // The Characters destination fills the full available portrait area.
    // Updated only on a genuine geometry change, not every frame.
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> FullPageEmptyCharactersStage;
    float LastCharactersViewportHeight = 0.f;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> NavigationBar;

    UPROPERTY(Transient)
    TObjectPtr<UUniformGridPanel> RosterGrid;

    UPROPERTY(Transient)
    TObjectPtr<UOGGate12ExploredMapWidget> TerritoryMapWidget;

    UPROPERTY(Transient)
    TObjectPtr<AOGGate12CharacterPreviewStage> PreviewStage;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AOGGate12CharacterPreviewStage>> RosterPreviewStages;

    FString RosterSearchQuery;
    float LastRosterLayoutWidth=0.f;
    int32 RosterSortMode = 0; // 0 name, 1 selected level, 2 owned copies
    int32 OpenIdentityIndex = INDEX_NONE;
    // UI preference only: never changes wardrobe, ownership, or world state.
    bool bPrefer3DCharacterPresentation = false;
    // Diagnostic artwork viewer only; not a real owned Manifestation.
    bool bDevelopmentQaViewerOpen = false;
    // -1 is the four-destination Records hub; selecting a card opens a full page.
    int32 RecordsSection = -1;
    FOGEntityId GachaContextRulerId;
    FOGContentId GachaContextBannerId;
    TSharedPtr<FOGWorldStoreCommittedEvents> GachaEventFeed;
    FDelegateHandle GachaEventHandle;
    FOGGachaBatchPreview PendingGachaReview;
    FOGEntityId PendingGachaRequestId;
    TArray<FOGGachaBatchPreview> GachaQuotes;
    TArray<FOGGachaPullResult> GachaSavedResults;
    UPROPERTY(Transient) TArray<TObjectPtr<UOGGate12RulerActionButton>> GachaDrawButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> GachaCostLabels;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GachaWalletLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GachaPityLabel;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> GachaPityBar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GachaReviewStatus;
    UPROPERTY(Transient) TObjectPtr<UOGGate12RulerActionButton> GachaConfirmButton;
    FString GachaFeedback;
    int32 GachaRevealedResults=1;
    bool bGachaDirty=true,bGachaCommandInFlight=false;
    bool bGachaHistoryExpanded = false;
    bool bGachaRatesExpanded = false;
    bool bGachaResultsExpanded = false;
    FOGEntityId SelectedTerritoryId;
    int32 SelectedTerritoryOverlayIndex = 0;
    FString LastUiError;

    // In-memory review only: never reflected, serialized, or treated as truth.
    // Confirmation tokens and request IDs are discarded when context changes.
    FOGCharacterDevelopmentPreview PendingDevelopmentReview;
    FOGEntityId PendingDevelopmentRequestId;
    FOGEntityId DevelopmentContextRulerId;
    FOGContentId DevelopmentContextIdentityId;
    FOGEntityId DevelopmentContextManifestationId;
    FOGContentId DevelopmentSessionIdentityId;
    FOGEntityId DevelopmentSessionManifestationId;
    FString DevelopmentFeedback;
    FString DevelopmentRefreshWarning;
    bool bDevelopmentFeedbackError = false;
    bool bDevelopmentCommandInFlight = false;

    // Inventory utility page and review are transient presentation only.
    bool bInventoryOpen = false;
    bool bInventoryTargetPicker = false;
    FOGEntityId InventorySelectedItemId;
    FOGEntityId InventoryContextRulerId;
    FOGContentId InventoryContextIdentityId;
    FOGEntityId InventoryContextManifestationId;
    FOGObjectActionPreview PendingObjectReview;
    FOGEntityId PendingObjectRequestId;
    FOGEntityId ObjectActionCounterpartyId;
    FOGOwnedEquipmentPreview PendingEquipmentReview;
    FOGEntityId PendingEquipmentRequestId;
    FString InventoryFeedback;
    FString InventoryRefreshWarning;
    bool bInventoryFeedbackError = false;
    bool bInventoryCommandInFlight = false;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FOGInventoryQueuedConfirmationTest;
#endif

    bool bNarrativeJournalOpen = false;
    FOGContentId OpenNarrativeQuestId;
    FOGContentId OpenNarrativeDialogueId;
    FOGEntityId NarrativePageRulerId;
    FOGNarrativePreview PendingNarrativePreview;
    FOGEntityId PendingNarrativeRequestId;
    FString NarrativeFeedback;

    void BuildRoot();
    void RefreshFrozenScene(EOGRulerPrimaryDestination Destination);
    void BuildCurrentPage();
    void BuildHome();
    void BuildGacha();
    void BuildGachaPortrait();
    void HandleGachaAction(const FString& Action,UOGGate12RulerSessionSubsystem& Session);
    void ResetGachaUi();
    void ClearGachaReview();
    void ObserveGacha(UOGGate12RulerSessionSubsystem& Session);
    void SynchronizeGachaUi();
    void HandleGachaEvents(const TArray<FOGWorldEvent>& Events);
    void BuildRoster();
    void RebuildRosterGrid();
    void BuildDevelopmentQaCharacterViewer();

    UFUNCTION()
    void HandleRosterSearchChanged(const FText& NewText);
    void BuildCharacterDetail();
    void BuildCharacterDevelopment(const FOGRosterIdentityViewModel& Identity,
        const FOGRosterManifestationViewModel* Manifestation);
    void AddDevelopmentPreview(UVerticalBox* Parent,
        const FOGCharacterDevelopmentPreview& Preview);
    void HandleDevelopmentAction(const FString& Action,
        UOGGate12RulerSessionSubsystem& Session);
    void ClearDevelopmentReview();
    void ResetCharacterDevelopment();
    bool SynchronizeDevelopmentContext(const UOGGate12RulerSessionSubsystem* Session);
    void BuildInventory();
    void BuildObjectActions(UVerticalBox* Parent,UOGGate12RulerSessionSubsystem& Session);
    void BuildEquipmentReview(UVerticalBox* Parent, const FOGOwnedEquipmentPreview& Review);
    void HandleInventoryAction(const FString& Action, UOGGate12RulerSessionSubsystem& Session);
    void ClearInventoryReview();
    void ResetInventoryUi();
    bool SynchronizeInventoryContext(const UOGGate12RulerSessionSubsystem* Session);
    void AddCharacterVisual(UVerticalBox* Parent, const FOGRosterIdentityViewModel& Identity, bool bThreeDimensional);
    void ReleasePreviewStage();
    void ReleaseRosterPreviewStages();
    void LoadSelectedPresentationPreference(const UOGGate12RulerSessionSubsystem* Session, FString& OutError);
    void BuildRecords();
    void BuildNarrativeJournal();
    void HandleNarrativeAction(const FString& Action);
    void ClearNarrativeReview();
    void ResetNarrativePage();
    void BuildTerritory();
    void BuildNav();
    void AddMessage(UVerticalBox* Parent, const FString& Value, int32 FontSize = 22,
        FLinearColor Tint = FLinearColor(0.83f, 0.84f, 0.88f, 1.0f));
    void AddAction(UVerticalBox* Parent, const FString& Title, const FString& Action,
        bool bEnabled = true);
};
