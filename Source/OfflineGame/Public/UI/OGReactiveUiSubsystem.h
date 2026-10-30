#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Content/OGDefinitionLibrary.h"
#include "Persistence/OGWorldStore.h"
#include "OGReactiveUiSubsystem.generated.h"

class AActor;
class UUserWidget;
class UWidget;
class ULightComponent;
class UMaterialInstanceDynamic;
class UOGGameCoreSubsystem;
struct FOGResolvedCombatAction;
struct FOGTurnBattleState;

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGReactiveUiPalette
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor Accent=FLinearColor(.72f,.62f,.44f,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor Rim=FLinearColor(.36f,.35f,.40f,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor Surface=FLinearColor(.009f,.009f,.016f,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor Text=FLinearColor(.92f,.91f,.94f,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor Muted=FLinearColor(.59f,.59f,.65f,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Glow=.16f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float MotionRate=.18f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float TransitionSeconds=.35f;
    bool IsFinite() const;
    bool NearlyEquals(const FOGReactiveUiPalette& Other) const;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGReactiveUiContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGEntityId RulerId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId SubjectId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId WorldBodyId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId LocationId;
    UPROPERTY(BlueprintReadOnly) FName LocationKind;
    UPROPERTY(BlueprintReadOnly) FName MovementContext;
    UPROPERTY(BlueprintReadOnly) FName NativeMovementContext;
    UPROPERTY(BlueprintReadOnly) FName RulerTitle;
    UPROPERTY(BlueprintReadOnly) TArray<FOGContentId> RulerClasses;
    UPROPERTY(BlueprintReadOnly) int32 OwnedLevel=0;
    UPROPERTY(BlueprintReadOnly) bool bWorldMode=false;
    UPROPERTY(BlueprintReadOnly) bool bCombat=false;
    UPROPERTY(BlueprintReadOnly) bool bCanonicalReady=false;
    FString RulerScopeJson=TEXT("{}");
    FString LocationStateJson=TEXT("{}");
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGReactiveUiRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Priority=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MinimumOwnedLevel=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName RequiredRulerTitle;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId RequiredRulerClass;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName RequiredLocationKind;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName RequiredMovementContext;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<FName,FString> RequiredRulerScope;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<FName,FString> RequiredLocationState;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bRequireCombat=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGReactiveUiPalette Palette;
    bool Matches(const FOGReactiveUiContext&) const;
};
/** Installed presentation rules only; these never grant levels, powers or gameplay permission. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGReactiveUiThemeDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGReactiveUiThemeDefinition();
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGReactiveUiRule> Rules;
    virtual bool ValidateDefinition_Implementation(FString&) const override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOGReactiveUiPaletteChanged,const FOGReactiveUiPalette&,Palette);

/** One read-only native presentation driver per World. No wallet/gameplay writes and no page rebuilding. */
UCLASS()
class OFFLINEGAME_API UOGReactiveUiSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase&) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UOGReactiveUiSubsystem,STATGROUP_Tickables); }
    virtual bool IsTickable() const override;
    static UOGReactiveUiSubsystem* Find(const UObject* Context);
    void TrackTree(UUserWidget* Owner);
    /** Explicit semantic ownership: rarity, warning, team and artwork colours are never themed. */
    void PreserveOriginal(UWidget* Widget);
    void UntrackTree(UUserWidget* Owner);
    const FOGReactiveUiPalette& GetPalette() const { return Current; }
    const FOGReactiveUiContext& GetContext() const { return Context; }
    float GetEffectPhase() const { return Phase; }
    bool IsMotionReduced() const { return bReducedMotion; }
    FLinearColor AccentFor(const FLinearColor& Original,float Strength=.65f) const;
    UFUNCTION(BlueprintCallable,Category="NoWorld|UI") void SetReducedMotion(bool Reduced);
    UFUNCTION(BlueprintCallable,Category="NoWorld|UI") void SelectInstalledTheme(FOGContentId ThemeId);
    UFUNCTION(BlueprintCallable,Category="NoWorld|UI") void SetEnvironmentLight(ULightComponent* Light);
    UFUNCTION(BlueprintCallable,Category="NoWorld|UI") bool BindMaterial(UMaterialInstanceDynamic* Material,FName AccentParameter,FName GlowParameter);
    UPROPERTY(BlueprintAssignable,Category="NoWorld|UI") FOGReactiveUiPaletteChanged OnPaletteChanged;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) FString Status;
    /** Pure visual tuning; never an experience/rank limit or source gameplay formula. */
    static FOGReactiveUiPalette ResolveDefault(const FOGReactiveUiContext&);
private:
    friend struct FOGReactiveUiTestAccess;
    enum class ERole : uint8 { Original,Primary,Secondary,Accent,Surface,Ornament };
    struct FBinding
    {
        TWeakObjectPtr<UUserWidget> Owner;
        TWeakObjectPtr<UWidget> Widget;
        FLinearColor Base,Last;
        ERole Role=ERole::Original;
    };
    struct FMaterialBinding
    {
        TWeakObjectPtr<UMaterialInstanceDynamic> Material;
        FName Accent,Glow;
        FLinearColor OriginalAccent;
        float OriginalGlow=0;
        FLinearColor LastAccent;
        float LastGlow=0;
    };
    TMap<TWeakObjectPtr<UWidget>,FBinding> Bindings;
    TArray<FMaterialBinding> Materials;
    TSet<TWeakObjectPtr<UWidget>> OriginalWidgets;
    TSet<TWeakObjectPtr<UUserWidget>> PendingTrees;
    TWeakObjectPtr<UOGGameCoreSubsystem> Core;
    TSharedPtr<FOGWorldStoreCommittedEvents> RulerFeed,SubjectFeed,WorldBodyFeed,LocationFeed;
    FDelegateHandle RulerHandle,SubjectHandle,BodyHandle,LocationHandle,ReleaseHandle,ActionHandle,BattleHandle,LibraryHandle,SpawnHandle,ProfileHandle;
    TWeakObjectPtr<ULightComponent> EnvironmentLight;
    FOGReactiveUiContext Context;
    FOGReactiveUiPalette Current,Target;
    FOGContentId ThemeId;
    UPROPERTY(Transient) TObjectPtr<UOGReactiveUiThemeDefinition> ActiveTheme;
    bool bThemeDirty=true,bStyleDirty=true,bHasAuthoredPalette=false;
    FOGReactiveUiPalette AuthoredPalette;
    bool bDirty=true,bApplying=false,bReducedMotion=false,bLightDirty=true;
    float Phase=0,ActionPulse=0,PropertyRefreshSeconds=0;
    void Disconnect();
    void Observe(UOGGameCoreSubsystem*,const FOGReactiveUiContext&);
    void ReadProjection();
    void HandleEvents(const TArray<FOGWorldEvent>&);
    void HandleRelease();
    void HandleAction(const FOGResolvedCombatAction&);
    void HandleBattle(const FOGTurnBattleState&,const FOGEntityId&);
    void HandleDefinitions(int64);
    void HandleActorSpawned(AActor*);
    void DiscoverLight();
    void Apply();
    void Track(UUserWidget*,UWidget*);
    FLinearColor Resolve(const FBinding&) const;
    ERole Classify(UWidget*,const FLinearColor&) const;
};
