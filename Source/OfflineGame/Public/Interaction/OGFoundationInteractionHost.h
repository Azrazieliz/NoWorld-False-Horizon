#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/OGDiagnosticAnimationPresentation.h"
#include "UObject/StrongObjectPtr.h"
#include "Interaction/OGSystemicInteractionRuntime.h"
#include "Interaction/OGInteractionDefinition.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "OGFoundationInteractionHost.generated.h"

class UOGGameCoreSubsystem;
struct FOGTurnBattleState;
class UOGDiagnosticCombatComponent;
class UOGDiagnosticAnimationPresentation;
class UOGSharedMotionSceneComponent;
class USkeletalMeshComponent;
class UOGSharedMotionControllerComponent;

struct OFFLINEGAME_API FOGFoundationInteractionRow
{
    FName Id;
    FString Label;
    bool bEnabled = true;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOGInteractionCharacterPresentation, const FOGInteractionParticipantState&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOGInteractionConstraintPresentation, const FOGInteractionConstraint&, const TArray<FOGInteractionParticipantState>&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOGInteractionReactionPresentation, const FOGEntityId&, UObject*, UObject*);

/** Disposable neutral controls hosting the canonical, content-neutral graph.
 * Entry bindings and selected participants have no fixed arity limit. */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGFoundationInteractionHost : public UActorComponent,
    public IOGSystemicInteractionPresentation
{
    GENERATED_BODY()
public:
    UOGFoundationInteractionHost();
    virtual ~UOGFoundationInteractionHost() override;
    bool Initialize();
    /** Native catalog of admitted original interaction definitions. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Content")
    TSoftObjectPtr<UOGInteractionCatalog> Catalog;
    bool IsActive() const;
    FString GetStatus() const;
    TArray<FOGFoundationInteractionRow> GetRows() const;
    bool ActivateRow(FName Id);
    bool BeginContext(FName Context, const TArray<FOGInteractionParticipant>& Participants = {},
        const FOGEntityId& ContextEventId = FOGEntityId());
    bool EnterAuthored(const FOGInteractionEntry&, const FOGInteractionActionGraph&);
    void SetSelectedParticipants(const TArray<FOGInteractionParticipant>& Participants);
    void Cancel(FName Reason = FName(TEXT("player_exit")));

    // Ordinary character consumers receive the complete projection, never a
    // synthetic body/state replacement. Rig-specific IK/expression code extends
    // these native callbacks without gaining authority over factual state.
    FOGInteractionCharacterPresentation OnCharacterPresentation;
    FOGInteractionConstraintPresentation OnConstraintPresentation;
    FOGInteractionReactionPresentation OnReactionPresentation;

    virtual void ApplyCharacter(const FOGInteractionParticipantState&) override;
    virtual void SetSignificance(const FOGEntityId&, bool, bool) override;
    virtual void ApplyConstraint(const FOGInteractionConstraint&, const TArray<FOGInteractionParticipantState>&) override;
    virtual void ApplyAnimation(const FOGEntityId&, UObject*) override;
    virtual bool PlaySharedAction(const FOGEntityId&, FName, FName) override;
    virtual bool PlaySharedScene(const FOGInteractionActionNode&,
        const TArray<FOGInteractionParticipantState>&) override;
    virtual void StopSharedScene() override;
    virtual void EndAction() override;
    virtual void ApplyReaction(const FOGEntityId&, UObject*, UObject*) override;
    virtual void SetPrivacy(bool) override;
    virtual void RestoreGameplay() override;
protected:
    virtual void TickComponent(float, ELevelTick, FActorComponentTickFunction*) override;
    virtual void EndPlay(const EEndPlayReason::Type) override;
private:
    void HandleCanonicalRuntimeReleasing();
    void HandleApplicationDeactivated();
    void HandleBattleCommitted(const FOGTurnBattleState& State,const FOGEntityId& Receipt);
    bool EnsureRuntime();
    void BuildDiagnosticSelection(bool bAll);
    void RefreshProductionSelection();
    FOGEntityId GetAcceptedContextEvent(bool bCompletedBattle) const;
    UOGSharedMotionControllerComponent* FindMotionController(AActor*) const;
    void OwnMotion(UOGSharedMotionControllerComponent*);
    void UpdateAnimationPacing();
    AActor* FindBoundActor(const FOGEntityId&) const;
    struct FMeshRestore
    {
        TWeakObjectPtr<USkeletalMeshComponent> Mesh;
        TWeakObjectPtr<UOGDiagnosticAnimationPresentation> Animation;
        uint8 TickPolicy = 0;
        bool bUpdateRateOptimization = false;
        bool bHadClip = false;
        bool bOwnsClip = false;
        FOGDiagnosticAnimationClip PreviousClip;
        TStrongObjectPtr<UAnimSequence> PreviousSequence;
    };
    TWeakObjectPtr<UOGGameCoreSubsystem> Core;
    TWeakObjectPtr<UOGDiagnosticCombatComponent> Combat;
    TWeakObjectPtr<UOGPresentationModeSubsystem> PresentationMode;
    EOGPresentationMode SavedPresentationMode = EOGPresentationMode::World;
    bool bOwnsPresentationMode = false;
    TWeakObjectPtr<class AOGWorldPresentationHud> InteractionHud;
    FName SavedHudSurface;
    IOGWorldStore* BoundStore = nullptr;
    FOGEntityId BoundRulerId;
    TUniquePtr<FOGSystemicInteractionRuntime> Runtime;
    FDelegateHandle ReleaseHandle;
    FDelegateHandle DeactivateHandle;
    FDelegateHandle BackgroundHandle;
    TArray<FOGInteractionParticipant> SelectedParticipants;
    TArray<FOGInteractionParticipant> AvailableParticipants;
    UPROPERTY(Transient) TArray<TObjectPtr<UOGInteractionDefinition>> AvailableDefinitions;
    UPROPERTY(Transient) TObjectPtr<UOGInteractionDefinition> SelectedDefinition;
    TMap<FOGEntityId, TWeakObjectPtr<AActor>> BoundActors;
    TMap<FOGEntityId, FMeshRestore> MeshRestores;
    UPROPERTY(Transient)
    TObjectPtr<UOGSharedMotionSceneComponent> ActiveSharedScene = nullptr;
    struct FMotionRestore
    {
        TWeakObjectPtr<UOGSharedMotionControllerComponent> Motion;
        bool bRootMotion = false;
        float Pacing = 1.f;
    };
    TArray<FMotionRestore> OwnedMotions;
    FOGInteractionActionGraph ActiveGraph;
    FString Status = TEXT("Choose participants and an entry context.");
    int64 LastTick = -1;
    double CameraAngle = 0;
    int32 SelectionIndex = 0;
    bool bPrivacy = false;
    bool bLastProfilePrivacy = false;
};
