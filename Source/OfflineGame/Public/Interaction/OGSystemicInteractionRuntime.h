#pragma once
#include "CoreMinimal.h"
#include "Core/OGEntityId.h"
#include "Runtime/OGFoundationCharacterRuntime.h"
#include "Runtime/OGOptionalAssetRuntime.h"
#include "World/OGWorldStateRecords.h"
#include "World/OGItemKnowledgeCharacterRecords.h"

class AActor;
class APlayerController;
class UWorld;
class IOGWorldStore;

/** Context labels are extensible; they are provenance, not access gates. */
struct OFFLINEGAME_API FOGInteractionParticipant
{
    FOGFoundationCharacterContext Character;
    FName Role = NAME_None;
    /** Distinguishes an explicit source role from the legacy generic participant selector. */
    bool bExplicitSourceRole=false;
    FOGContentId RigFamilyId;
    TWeakObjectPtr<AActor> Actor;
};
struct OFFLINEGAME_API FOGInteractionConstraint
{
    FName ParticipantRole;
    FName TargetRole;
    FName Effector;
    FVector TargetOffset = FVector::ZeroVector;
};
struct OFFLINEGAME_API FOGInteractionAnimationBinding
{
    FName Role;
    FOGContentId RigFamilyId;
    // Empty means shared; a nonempty ID is an exact current Manifestation override.
    FOGEntityId ManifestationId;
    FOGContentId VersionId;
    FOGOptionalAssetReference Animation;
    // Game-wide native semantic motion, resolved by actual rig and shared
    // SourceClipId, never by character Version or wardrobe display name.
    // Optional package clips remain valid when these fields are empty.
    FName SharedGameplayAction = NAME_None;
    FName SharedGameplayContext = NAME_None;
    // Only populated by an authored 2+ participant scene role. This is
    // a source-track role, distinct from the canonical world participant role.
    FName SharedSceneRole = NAME_None;
    FOGOptionalAssetReference Expression;
    FOGOptionalAssetReference Reaction;
};
/** Sparse completion-time changes. Role must resolve to exactly one canonical
 * participant. Objects merge recursively; other JSON values replace, including
 * null (no deletion). An unset optional ID preserves the current selection. */
struct OFFLINEGAME_API FOGInteractionPresentationPatch
{
    FName Role = NAME_None;
    TOptional<FOGContentId> SelectedSkinId;
    FString OutfitMergeJson = TEXT("{}");
    FString VariantMergeJson = TEXT("{}");
};
struct OFFLINEGAME_API FOGInteractionAdultContextPatch
{
    FName Role = NAME_None;
    TOptional<FOGContentId> CurrentProfileVariantId;
    FString MutableContextMergeJson = TEXT("{}");
};
/** Original source role offset/orientation; actor body scale stays canonical. */
struct OFFLINEGAME_API FOGInteractionRolePlacement
{
    FName Role = NAME_None;
    FTransform RelativeToAnchor = FTransform::Identity;
};
struct OFFLINEGAME_API FOGInteractionAuthoredConsequences
{
    TArray<FOGKnowledgeFactRecord> Knowledge;
    TArray<FOGSemanticMemoryRecord> Memories;
    TArray<FOGManifestationPresentationStateRecord> Presentation;
    TArray<FOGWorldPresenceRecord> Presence;
    TArray<FOGInteractionPresentationPatch> PresentationPatches;
    TArray<FOGInteractionAdultContextPatch> AdultContextPatches;
};
struct FOGInteractionParticipantState;
struct OFFLINEGAME_API FOGInteractionActionNode
{
    FName NodeId;
    FOGContentId ActionId;
    double DurationSeconds = 1.0;
    TArray<FName> RequiredRoles;
    TArray<FName> NextNodes;
    TArray<FOGInteractionAnimationBinding> AnimationBindings;
    // Original authored shared scene; the participant's individual rig role
    // is provided by AnimationBindings[].SharedSceneRole.
    // Empty means ordinary single-actor interaction playback.
    FName SharedSceneAction = NAME_None;
    FName SharedSceneContext = NAME_None;
    TArray<FOGInteractionConstraint> Constraints;
    FOGInteractionAuthoredConsequences Consequences;
    FName EnvironmentAnchorTag = NAME_None;
    FString DisplayName;
    TArray<FOGInteractionRolePlacement> RolePlacements;
    /** One source selector shared by solo playback, synchronized playback and compatibility reporting. */
    bool ResolveAnimationBinding(const FOGInteractionParticipantState& Participant,
        const FOGInteractionAnimationBinding*& Out,FString& Error) const;
};
struct OFFLINEGAME_API FOGInteractionActionGraph
{
    FOGContentId GraphId;
    FName StartNode;
    TArray<FOGInteractionActionNode> Nodes;
    FString DisplayName;
    FString Description;
    // Source classification/privacy hint only, including nonsexual mature
    // categories. Never a mutable permission or participant eligibility gate.
    bool bMaturePresentation = false;
};
struct OFFLINEGAME_API FOGInteractionEntry
{
    FName Context = FName(TEXT("world")); // roster/ruler/world/event/npc/post_combat/extension
    FOGEntityId ContextEventId;
    FOGEntityId LocationId;
    TArray<FOGInteractionParticipant> Participants;
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<APlayerController> ReturnController;
    FVector PreferredLocation = FVector::ZeroVector;
    bool bPrivacyPresentation = false;
};
struct OFFLINEGAME_API FOGInteractionParticipantState
{
    FOGInteractionParticipant Binding;
    FOGFoundationCharacterProjection Character;
    FTransform StagingTransform;
    bool bActionRelevant = false;
    bool bCameraRelevant = false;
    bool bAnimationCompatible = false;
    bool bHasOptionalPresentation = false;
    FString PresentationReason;
};

/** Presentation consumers may implement actual rig-specific IK/expression hooks.
 * Ordinary character significance is fed through this same bridge, without
 * removing logical participants or changing their current Manifestations. */
class OFFLINEGAME_API IOGSystemicInteractionPresentation
{
public:
    virtual ~IOGSystemicInteractionPresentation() = default;
    virtual void ApplyCharacter(const FOGInteractionParticipantState&) = 0;
    virtual void SetSignificance(const FOGEntityId&, bool bActionRelevant, bool bCameraRelevant) = 0;
    virtual void ApplyConstraint(const FOGInteractionConstraint&, const TArray<FOGInteractionParticipantState>&) = 0;
    virtual void ApplyAnimation(const FOGEntityId&, UObject* Animation) = 0;
    // Native semantic playback is presentation-only: the authored action graph
    // and persisted world consequences remain the sole gameplay authority.
    virtual bool PlaySharedAction(const FOGEntityId&, FName, FName) { return false; }
    virtual bool PlaySharedScene(const FOGInteractionActionNode&,
        const TArray<FOGInteractionParticipantState>&) { return false; }
    virtual void StopSharedScene() {}
    // Release action-scoped solo clips, reactions and constraints while keeping
    // session/character presentation ownership. Default preserves old bridges.
    virtual void EndAction() {}
    virtual void ApplyReaction(const FOGEntityId&, UObject* Expression, UObject* Reaction) = 0;
    virtual void SetPrivacy(bool bPrivacy) = 0;
    virtual void RestoreGameplay() = 0;
};

/** Content-neutral native runtime. All consequences use the existing world Store.
 * Host advances the canonical clock normally, even while Pacing is zero.
 * UWorld and actors are optional for logical Ruler/event execution. */
class OFFLINEGAME_API FOGSystemicInteractionRuntime
{
public:
    FOGSystemicInteractionRuntime(IOGWorldStore& Store, const FOGEntityId& CanonicalRulerId,
        FOGOptionalPackageHost* OptionalPackageHost = nullptr);
    ~FOGSystemicInteractionRuntime();
    bool SetOptionalPackageHost(FOGOptionalPackageHost* Host, FString& Error)
    {
        if (bActive) { Error = TEXT("Exit interaction before replacing its optional package host."); return false; }
        return OptionalAssets.SetContainerHost(Host, Error);
    }
    bool Enter(const FOGInteractionEntry&, const FOGInteractionActionGraph&, int64 WorldTick, FString&);
    bool SelectAction(FName NodeId, FString&);
    bool SetSequence(const TArray<FName>& NodeIds, FString&);
    bool SetPacing(double Rate, FString&);
    bool Advance(double DeltaSeconds, int64 WorldTick, FString&);
    bool Restage(const FVector& PreferredLocation, FString&);
    bool SetCamera(const FTransform& CameraTransform, FString&);
    void SetPrivacyPresentation(bool bPrivacy);
    void SetPresentationBridge(IOGSystemicInteractionPresentation* Bridge) { Presentation = Bridge; }
    bool Exit(FName Reason, int64 WorldTick, FString&);
    bool IsActive() const { return bActive; }
    bool IsAwaitingAction() const { return bActive && CurrentNode.IsNone(); }
    FName GetCurrentNode() const { return CurrentNode; }
    TArray<FName> GetAvailableActions() const;
    const TArray<FName>& GetSequence() const { return Sequence; }
    const TArray<FName>& GetQueuedActions() const { return Sequence; }
    double GetPacing() const { return Pacing; }
    const TArray<FOGInteractionParticipantState>& GetParticipants() const { return Participants; }
    static FOGInteractionActionGraph MakeNeutralDiagnosticGraph();
private:
    struct FActorRestore
    {
        TWeakObjectPtr<AActor> Actor;
        FTransform Transform;
        FVector Velocity = FVector::ZeroVector;
        uint8 MovementMode = 0;
        uint8 CustomMovementMode = 0;
        bool bHidden = false;
        bool bHasPresence = false;
        FOGWorldPresenceRecord Presence;
    };
    const FOGInteractionActionNode* FindNode(FName Id) const;
    bool RefreshCharacters(FString&);
    bool SolveStaging(const FVector&, const FOGInteractionActionNode*, FString&);
    bool StartNode(FName, FString&);
    bool CommitEvent(FName Type, FName Reason, int64 Tick,
        const FOGInteractionAuthoredConsequences* Consequences, FString&);
    bool ApplyStatePatches(const FOGInteractionAuthoredConsequences&, int64 Tick, FString&);
    void StopActionPresentation();
    void RestoreGameplay();
    void UpdatePresentation(const FOGInteractionActionNode&);
    bool LoadOptional(const FOGOptionalAssetReference&, UObject*&, FString&);
    void UpdateSignificance(const FOGInteractionActionNode&);
    IOGWorldStore& Store;
    FOGEntityId RulerId;
    FOGEntityId SessionId;
    FOGFoundationCharacterRuntime Characters;
    FOGOptionalAssetRuntime OptionalAssets;
    TMap<FString, TWeakObjectPtr<UObject>> SessionAssets;
    FOGInteractionEntry Entry;
    FOGInteractionActionGraph Graph;
    TArray<FOGInteractionParticipantState> Participants;
    TArray<FActorRestore> Restore;
    TArray<FName> Sequence;
    FName CurrentNode;
    FName LastCompletedNode;
    bool bCompletionRefreshPending = false;
    double Elapsed = 0;
    double Pacing = 1;
    bool bActive = false;
    int64 LastWorldTick = -1;
    TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UObject>> PlayedMontages;
    bool bInputSuppressed = false;
    bool bMoveSuppressed = false;
    bool bLookSuppressed = false;
    TWeakObjectPtr<AActor> ReturnViewTarget;
    TWeakObjectPtr<AActor> CameraActor;
    FRotator ReturnControlRotation;
    IOGSystemicInteractionPresentation* Presentation = nullptr;
};
