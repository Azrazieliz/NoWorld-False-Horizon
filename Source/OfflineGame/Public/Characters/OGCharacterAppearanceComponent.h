#pragma once

#include "CoreMinimal.h"
#include "Characters/OGCharacterAppearanceDefinition.h"
#include "Components/ActorComponent.h"
#include "Core/OGEntityId.h"
#include "OGCharacterAppearanceComponent.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UOGGameCoreSubsystem;
class UOGSharedMotionControllerComponent;

/** Read-only native presentation adapter; can be mounted on any authored Character/Actor. */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGCharacterAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UOGCharacterAppearanceComponent();

    /** Cook-reachable definitions; duplicate exact Identity/Version entries fail closed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    TArray<TSoftObjectPtr<UOGCharacterAppearanceDefinition>> Definitions;

    /** None uses the owner's root. Otherwise exactly one owned scene component with this name. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    FName AttachmentComponentName = NAME_None;

    /** Explicit existing skeletal body components hidden only after a successful candidate is ready. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
    TArray<FName> BodyComponentNamesToReplace;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Appearance")
    FName RequestedWardrobe = NAME_None;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Appearance")
    FName RenderedWardrobe = NAME_None;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Appearance")
    bool bSfwPresentation = true;

    /** Exact live Core Store owner/Identity/Version/lifecycle and saved wardrobe; never writes. */
    UFUNCTION(BlueprintCallable, Category="Appearance")
    bool BindOwnedManifestation(const FOGEntityId& Ruler, const FOGContentId& Identity,
        const FOGContentId& Version, const FOGEntityId& Manifestation, bool bSfw, FString& Error);

    /** Local real-asset preview only. Cannot replace an owned binding or manufacture ownership. */
    UFUNCTION(BlueprintCallable, Category="Appearance|Preview")
    bool InitializePreviewDefinition(UOGCharacterAppearanceDefinition* Definition,
        FName Wardrobe, bool bSfw, FString& Error);

    /** Render only. An owned requested wardrobe must already match persisted UI selection. */
    UFUNCTION(BlueprintCallable, Category="Appearance")
    bool ApplyWardrobe(FName Wardrobe, bool bSfw, FString& Error);

    /** Re-read UI-persisted selection and update rendering; no-op when already current. */
    UFUNCTION(BlueprintCallable, Category="Appearance")
    bool RefreshCanonicalAppearance(FString& Error);

    /** Requires a complete native appearance matching live ownership/Version/lifecycle and saved wardrobe. */
    UFUNCTION(BlueprintPure, Category="Appearance")
    bool MatchesOwnedManifestation(const FOGEntityId& Ruler, const FOGEntityId& Manifestation) const;

    /** Weight in [0,1]. Only mapped exact (part,morph) targets are written. */
    UFUNCTION(BlueprintCallable, Category="Appearance|Expression")
    bool SetSemanticExpression(FName Semantic, float Weight, FString& Error);

    /** Presentation playback only; caller retains gameplay/adult-context authority. */
    UFUNCTION(BlueprintCallable, Category="Appearance|Motion")
    bool PlaySemanticAction(FName Action, FName Context, bool bAdultContextAuthorized,
        int32 ParticipantCount, FName ParticipantRole, FString& Error);

    UFUNCTION(BlueprintPure, Category="Appearance")
    bool IsOwnedManifestationBound() const { return bOwnedBound; }

    UFUNCTION(BlueprintPure, Category="Appearance")
    UOGCharacterAppearanceDefinition* GetActiveDefinition() const { return ActiveDefinition.Get(); }

    UFUNCTION(BlueprintPure, Category="Appearance|Motion")
    UOGSharedMotionControllerComponent* GetMotionController() const { return ActiveMotionController.Get(); }

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
    struct FOwnedBinding
    {
        FOGEntityId Ruler;
        FOGContentId Identity;
        FOGContentId Version;
        FOGEntityId Manifestation;
    };

    struct FBodyVisibility
    {
        TWeakObjectPtr<USceneComponent> Component;
        bool bVisible = false;
        bool bHiddenInGame = true;
    };

    UPROPERTY(Transient)
    TObjectPtr<UOGCharacterAppearanceDefinition> ActiveDefinition;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> ActiveParts;

    UPROPERTY(Transient)
    TObjectPtr<UOGSharedMotionControllerComponent> ActiveMotionController;

    UPROPERTY(Transient)
    FOGCharacterAppearanceWardrobe ActiveWardrobeContract;

    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    FOwnedBinding OwnedBinding;
    bool bOwnedBound = false;
    bool bPreviewInitialized = false;
    bool bInstalling = false;
    float CanonicalRefreshElapsedSeconds = 0.f;
    TArray<FBodyVisibility> ReplacedBodyVisibility;
    TMap<FName, float> SemanticWeights;

    UOGCharacterAppearanceDefinition* FindDefinition(
        const FOGContentId& Identity, const FOGContentId& Version, FString& Error) const;
    bool ReadCanonicalWardrobe(const FOwnedBinding& Binding,
        const UOGCharacterAppearanceDefinition* Definition, FName& Wardrobe,
        UOGGameCoreSubsystem*& Core, FString& Error) const;
    bool HasCompleteNativeAppearance() const;
    bool ValidateCurrentBinding(FString& Error) const;
    bool InstallAppearance(UOGCharacterAppearanceDefinition* Definition, FName Wardrobe,
        bool bSfw, const FOwnedBinding* Binding, FString& Error);
    void ApplyExpressionWeights();
    void HandleCanonicalRuntimeReleasing();
    void ReleaseAppearance(bool bRestoreAuthoredBody);
};
