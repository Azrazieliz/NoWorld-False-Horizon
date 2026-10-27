#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/OGSystemicInteractionRuntime.h"
#include "OGInteractionParticipantComponent.generated.h"

class UOGCanonicalCharacterPresentation;
class UOGCharacterAppearanceComponent;
class UOGGameCoreSubsystem;
class UOGSharedMotionControllerComponent;
class USkeletalMeshComponent;

/** Native-authored source receipt. Selectors are exact, never wildcards.
 * CanonicalMaturity copies original lore/Identity data; AuthoredAge is descriptive,
 * never a numerical/visual age heuristic. General mature presentation is separate. */
struct OFFLINEGAME_API FOGInteractionParticipantSourceProfile
{
    FOGEntityId EntityId;
    FOGContentId IdentityId;
    FOGContentId VersionId;
    FOGContentId SourceProfileId;
    FString SourceRevision;
    FString AuthoredAge;
    EOGCanonicalMaturity CanonicalMaturity = EOGCanonicalMaturity::Unknown;

    bool Matches(const FOGFoundationCharacterProjection& Projection) const;
    /** Copies the real definition; mismatched Identity/Version cannot be relabeled. */
    static bool FromDefinitions(const FOGFoundationCharacterProjection& Projection,
        const FOGCharacterIdentityDefinition& Identity, const FOGCharacterVersionDefinition& Version,
        const FOGContentId& SourceProfileId, const FString& SourceRevision,
        const FString& AuthoredAge, FOGInteractionParticipantSourceProfile& Out, FString& Error);
};

/** Supply from native authored definitions or a verified install receipt, never
 * imported world JSON, appearance flags, display names or legacy AdultRestricted. */
using FOGInteractionParticipantSourceResolver = TFunction<bool(
    const FOGFoundationCharacterProjection&, FOGInteractionParticipantSourceProfile&, FString&)>;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOGInteractionParticipantSignificanceChanged, bool, bool);

/** Binds an existing canonical entity to this actual actor. No Store writes,
 * identity generation, ownership assignment, asset substitution or eligibility subsystem. */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGInteractionParticipantComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGInteractionParticipantComponent();

    /** Authored source label for presentation only; never canonical authority. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="OfflineGame|Interaction")
    FText DisplayName;

    /** Native factory supplies its real entity and authored rig family. Owner is
     * always derived by FOGFoundationCharacterRuntime::Project from the Store. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Interaction")
    bool BindActualNativeParticipant(const FOGEntityId& EntityId,
        const FOGContentId& RigFamilyId, FString& Error);
    /** Same production path for a native factory with an explicit Store lifetime.
     * Store/Host must outlive the binding; Unbind before destroying either when
     * LifecycleCore is null. No diagnostic selection or fixture creation occurs. */
    bool BindFromCanonicalStore(const FOGEntityId& EntityId, const FOGContentId& RigFamilyId,
        IOGWorldStore& Store, FOGOptionalPackageHost* Host, FString& Error,
        UOGGameCoreSubsystem* LifecycleCore = nullptr);
    void Unbind();

    /** Fresh read-only snapshots. Failure always resets output; no cached fallback. */
    bool BuildParticipant(FName Role, FOGInteractionParticipant& Out, FString& Error) const;
    bool GetCurrentProjection(FOGFoundationCharacterProjection& Out, FString& Error) const;
    /** Refreshes the ordinary canonical/appearance adapter, then rediscovers parts. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Interaction")
    bool RefreshFromCanonical(FString& Error);
    UOGSharedMotionControllerComponent* GetMotionController() const;
    bool GetNativeParts(TArray<USkeletalMeshComponent*>& Out, FString& Error) const;

    void SetSourceProfileResolver(FOGInteractionParticipantSourceResolver Resolver);
    bool ResolveSourceProfile(FOGInteractionParticipantSourceProfile& Out, FString& Error) const;
    /** Only Host's explicitly adult/sexual source category should consume this
     * metadata check. Never apply it to general mature/injury/violence graphs. */
    bool HasCanonicalAdultSource(FString& Error) const;

    /** Applies ordinary mesh update policy to every native part; restores exact
     * prior settings on release, unbind, destruction, and part replacement. */
    void SetInteractionSignificance(bool bActionRelevant, bool bCameraRelevant);
    void RestoreSignificance();
    bool IsInInteractionPresentation() const { return bHasSignificance; }
    FOGInteractionParticipantSignificanceChanged OnSignificanceChanged;

    /** Read-only discovery. No body/controller creation, registration or playback. */
    static bool DiscoverNativeBinding(AActor* Actor, TArray<USkeletalMeshComponent*>& Parts,
        UOGSharedMotionControllerComponent*& Motion, FString& Error);
    /** Finds a UNIQUE real body whose canonical projection is the Ruler. Does
     * not use possession, diagnostic selection, actor names, or fallback relabeling.
     * No matching projected physical body is a normal fail-closed result. */
    static bool ResolveActualRulerParticipant(UWorld* World, IOGWorldStore& Store,
        const FOGEntityId& RulerId, FOGInteractionParticipant& Out, FString& Error);

protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
    bool ValidateEmbodiment(const FOGFoundationCharacterProjection&, FString& Error) const;
    struct FPartSignificanceRestore
    {
        TWeakObjectPtr<USkeletalMeshComponent> Part;
        bool bUpdateRateOptimizations = false;
        uint8 VisibilityTickOption = 0;
    };
    FOGEntityId BoundEntityId;
    FOGContentId BoundRigFamilyId;
    IOGWorldStore* BoundStore = nullptr;
    FOGOptionalPackageHost* BoundHost = nullptr;
    TWeakObjectPtr<UOGGameCoreSubsystem> BoundCore;
    FDelegateHandle ReleaseHandle;
    TWeakObjectPtr<UOGCanonicalCharacterPresentation> CanonicalPresentation;
    TWeakObjectPtr<USkeletalMeshComponent> CanonicalRigPart;
    bool bConfiguredCanonicalRig = false;
    FOGInteractionParticipantSourceResolver SourceResolver;
    TArray<FPartSignificanceRestore> SignificanceRestore;
    bool bHasSignificance = false;
    bool bActionSignificant = false;
    bool bCameraSignificant = false;
    bool bRefreshing = false;
    bool bOwnsCanonicalPresentation = false;
    bool bUsesCoreLifetime = false;
    bool bUnbinding = false;
};
