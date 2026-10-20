#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "World/OGStartingRegionPresentation.h"
#include "UI/OGGate12OwnedWorldCharacterBinding.h"
#include "OGLuminousV3PlayableCharacter.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;
class UInputComponent;
class UOGLuminousV3WardrobeQAWidget;

/** Three clothing states of one canonical character; never Manifestations. */
UENUM(BlueprintType)
enum class EOGLuminousV3Wardrobe : uint8
{
    Clothed, Underwear, Nude
};

/**
 * Read-only appearance projection for the Gate10 character detail adapter.
 * ActualWardrobe is the authoritative clothing choice; RenderedWardrobe is
 * strictly derived and may be substituted for Privacy/SFW.
 */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGLuminousV3AppearanceProjection
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    EOGLuminousV3Wardrobe ActualWardrobe = EOGLuminousV3Wardrobe::Clothed;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    EOGLuminousV3Wardrobe EffectiveRenderedWardrobe = EOGLuminousV3Wardrobe::Clothed;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    bool bPrivacySfwEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    bool bExplicitContextAuthorized = false;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    bool bMaskedForPrivacyOrUnavailableAdultContext = false;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    bool bRenderable = false;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    bool bDefaultBaseAppearance = true;
};

/** Pure presentation policy: never touches world store, outfit JSON or items. */
struct OFFLINEGAME_API FOGLuminousV3AppearancePolicy
{
    static FOGLuminousV3AppearanceProjection Resolve(
        EOGLuminousV3Wardrobe Actual,
        bool bPrivacySfw,
        bool bExplicitContextAuthorized,
        bool bSafeClothedAssetAvailable);
};

UENUM(BlueprintType)
enum class EOGLuminousV3MotionPhase : uint8
{
    Idle, Walk, Run, Jump, Fall, Swimming, Flying
};
/** Four real Gate09 Base-CLOTHED native skeletal clips. */
UENUM(BlueprintType)
enum class EOGLuminousV3AuthoredEmote : uint8
{
    Thoughtful, Greeting, Expressive, Bow, DancePerformance
};

/**
 * Native 975-joint Luminous body on existing World Mode pawn.
 * Exactly one wardrobe's parts are assigned at a time. The Foundation movement,
 * combat, touch controls, camera and party systems remain inherited unchanged.
 */
UCLASS(Blueprintable)
class OFFLINEGAME_API AOGLuminousV3PlayableCharacter final : public AOGWorldPrototypeCharacter, public IOGGate12OwnedWorldCharacterBinding
{
    GENERATED_BODY()
public:
    AOGLuminousV3PlayableCharacter();

    // Actual acquired Manifestation, verified in the canonical SQLite store;
    // never a QA owner, auto-granted character or interchangeable Version.
    virtual bool BindOwnedWorldCharacter_Implementation(
        const FOGEntityId& RulerId, const FOGContentId& IdentityId,
        const FOGContentId& VersionId, const FOGEntityId& ManifestationId,
        bool bSfwPresentation) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|Native Assets")
    TArray<FSoftObjectPath> ClothedNativeParts;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|Native Assets")
    TArray<FSoftObjectPath> UnderwearNativeParts;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|Native Assets")
    TArray<FSoftObjectPath> NudeNativeParts;

    /** In order CLOTHED, UNDERWEAR, NUDE; each native clip has own exact 975 skeleton. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|MotionV3")
    TArray<FSoftObjectPath> NativeWalkByWardrobe;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|MotionV3")
    TArray<FSoftObjectPath> NativeRunByWardrobe;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|MotionV3")
    TArray<FSoftObjectPath> NativeJumpByWardrobe;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|MotionV3")
    EOGLuminousV3MotionPhase CurrentMotionPhase = EOGLuminousV3MotionPhase::Idle;

    /** Gate 11 canonical IdentityId/VersionId; blank blocks persistence. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|Canonical")
    FString CanonicalLuminousIdentityId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|Canonical")
    FString CanonicalLuminousVersionId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|Wardrobe")
    EOGLuminousV3Wardrobe InitialWardrobe = EOGLuminousV3Wardrobe::Clothed;

    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Wardrobe")
    EOGLuminousV3Wardrobe CurrentWardrobe = EOGLuminousV3Wardrobe::Clothed;

    /** Render-only state; does not become a new wardrobe or equipment record. */
    UPROPERTY(BlueprintReadOnly, Category="OfflineGame|Luminous|Presentation")
    FOGLuminousV3AppearanceProjection EffectiveAppearance;

    /** Only for isolated Gate12 QA world. Never persists, never enabled in production. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|QA")
    bool bRunNonPersistentWardrobeQACycle = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|QA")
    bool bRunNativeMotionQACycle = false;

    /** Mobile/gamepad wardrobe QA controls, disabled by default and never substitutes Gate10 UI. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|QA")
    bool bEnableNativeWardrobeQATouchPanel = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="OfflineGame|Luminous|QA")
    bool bEnterWorldModeOnIsolatedQAStartup = false;

    /**
     * If bPersist=false: local engineering/preview wardrobe switch.
     * If bPersist=true: must target the selected matching canonical Luminous
     * identity/version; edits only the existing outfit_state_json.
     */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|Wardrobe")
    bool SetLuminousWardrobe(EOGLuminousV3Wardrobe Desired, bool bPersist);

    UFUNCTION(BlueprintPure, Category="OfflineGame|Luminous|Wardrobe")
    int32 GetActiveLuminousPartCount() const { return ActivePartCount; }

    /** Explicit visual refresh; never writes the canonical store or alters actual clothing. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|Presentation")
    bool RefreshPrivacyRenderedAppearance();

    UFUNCTION(BlueprintPure, Category="OfflineGame|Luminous|Wardrobe")
    FString GetLastLuminousError() const { return LastLuminousError; }

    /**
     * Exact-skeleton animation binding; no foreign CK/CMU fixture animations
     * or rejected Motion V1/V2 are allowed.
     */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|MotionV3")
    bool SetLuminousNativeMotionClip(UAnimSequence* Clip, bool bLoop);

    UFUNCTION(BlueprintPure, Category="OfflineGame|Luminous|MotionV3")
    bool HasCompatibleLuminousMotion() const { return bCompatibleNativeMotion; }
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|Emotes")
    bool PlayGate09AuthoredEmote(EOGLuminousV3AuthoredEmote Emote);
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|Emotes")
    void InterruptGate09AuthoredEmote();
    UFUNCTION(BlueprintPure, Category="OfflineGame|Luminous|Emotes")
    bool IsGate09EmotePlaying() const { return bGate09EmotePlaying; }

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<USkeletalMeshComponent>> RuntimeParts;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> CurrentNativeClip;

    UPROPERTY(Transient)
    TObjectPtr<UOGLuminousV3WardrobeQAWidget> WardrobeQAWidget;

    void CycleLuminousWardrobeFromGamepad();

    int32 ActivePartCount = 0;
    FOGEntityId BoundOwnedRuler;
    FOGEntityId BoundOwnedManifestation;
    FOGContentId BoundOwnedIdentity;
    FOGContentId BoundOwnedVersion;
    bool bNativeOwnedCharacterBound = false;
    bool bOwnedPresentationSfw = true;
    bool bLastAppliedPrivacy = false;
    bool bLastAppliedExplicitContext = false;
    bool bCompatibleNativeMotion = false;
    // The motion driver is selected from the exact imported clip skeleton, not a fixed part index.
    int32 ActiveMotionLeaderIndex = INDEX_NONE;
    bool bLuminousReady = false;
    bool bGate09EmotePlaying = false;
    float Gate09EmoteEndTime = 0.f;
    int32 QACycleStep = 0;
    int32 QAMotionCycleStep = 0;
    float QANextCycleSeconds = 0.f;
    float QANextMotionSeconds = 0.f;
    FTimerHandle QAWorldModeTimer;
    FString LastLuminousError;

    void ActivateNativeQAWorldMode();
    void UpdateNativeMotionFromTraversal();
    UAnimSequence* ResolveAuthoredMotionClip(EOGLuminousV3MotionPhase Phase) const;

    const TArray<FSoftObjectPath>& PartsFor(EOGLuminousV3Wardrobe State) const;
    static int32 ExpectedPartsFor(EOGLuminousV3Wardrobe State);
    bool ValidateWardrobeAssets(EOGLuminousV3Wardrobe Desired, FString& Error) const;
    bool ApplyVisualWardrobe(EOGLuminousV3Wardrobe Desired, FString& Error);
    bool IsIsolatedWardrobeQAContext() const;
    void ConcealUnsafeVisualParts();
    bool ResolveCanonicalControlledContext(struct FOGFoundationCharacterContext& OutContext, FString& Error) const;
    bool RestoreCanonicalWardrobe();
    bool PersistCanonicalWardrobe(EOGLuminousV3Wardrobe Desired, FString& Error);
};
