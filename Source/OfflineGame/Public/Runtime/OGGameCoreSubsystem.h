#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Persistence/OGWorldStore.h"
#include "Combat/OGLuminousCanonicalAuthorityBinding.h"
#include "Combat/OGCombatRewardDefinition.h"
#include "Combat/OGWorldAbilityExecutionComponent.h"
#include "Combat/OGCombatRecoveryProvider.h"
#include "Runtime/OGPerformanceTelemetry.h"
#include "Runtime/OGCanonicalClockRuntime.h"
#include "Runtime/OGFoundationStrategicRuntime.h"
#include "Runtime/OGOptionalPackageHost.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OGGameCoreSubsystem.generated.h"

class FOGLocallyInstalledPackageProvider;
class UOGNativeEncounterDefinition;
struct FOGResolvedEncounterParticipant;
struct FOGTrustedOptionalPackageInstall;

DECLARE_MULTICAST_DELEGATE(FOGCanonicalRuntimeReleasing);
// Presentation-only signal AFTER an authentic, successful native turn action.
// No gameplay executor may infer a hit or award a resource from this event.
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FOGLuminousCombatActionResolvedEvent,const FOGEntityId&,const FOGContentId&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOGNativeCombatActionResolvedEvent, const FOGResolvedCombatAction&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOGNativeBattleStateCommittedEvent,
    const FOGTurnBattleState&,const FOGEntityId&);

/** Read-only provenance of the last successfully journaled native combat action.
 * EventId is the actual canonical ledger ID, not the action or battle ID.
 * BattleStatus is the executor's status AFTER that recorded action. */
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGNativeCombatEventProvenance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FOGEntityId EventId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId BattleId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId OwningRulerId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId ActionId;
    UPROPERTY(BlueprintReadOnly) FOGEntityId SourceUnitId;
    UPROPERTY(BlueprintReadOnly) TArray<FOGEntityId> TargetUnitIds;
    UPROPERTY(BlueprintReadOnly) FOGContentId SkillId;
    UPROPERTY(BlueprintReadOnly) int64 WorldTick = -1;
    UPROPERTY(BlueprintReadOnly) EOGTurnBattleStatus BattleStatus = EOGTurnBattleStatus::NotStarted;
};

/**
 * Application-level owner of the authoritative game-core lifetime.
 *
 * Presentation systems may query/command the core through services exposed from
 * here, but World Mode actors and Ruler Mode widgets must not become sources of
 * authoritative state.
 */
UCLASS()
class OFFLINEGAME_API UOGGameCoreSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    FOGCanonicalRuntimeReleasing OnCanonicalRuntimeReleasing;
    FOGWorldStoreCommittedEvents OnCanonicalWorldEventsCommitted;
    /** Entity-scoped delivery avoids broadcasting every World event to every actor. */
    TSharedPtr<FOGWorldStoreCommittedEvents> GetEntityEventFeed(const FOGEntityId& Entity);
    FOGLuminousCombatActionResolvedEvent OnLuminousCombatActionResolved;
    // Full accepted branch/target context for generic native character presentation.
    FOGNativeCombatActionResolvedEvent OnNativeCombatActionResolved;
    /** Actual battle state, once per successful durable root transaction/opening. */
    FOGNativeBattleStateCommittedEvent OnNativeBattleStateCommitted;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "OfflineGame|Core")
    bool IsCoreReady() const { return bCoreReady; }

    UFUNCTION(BlueprintCallable, Category = "OfflineGame|Diagnostics")
    bool GenerateDiagnosticsBundle(
        FString& OutBundlePath,
        FString& OutError);

    bool CreateManualRecoverySnapshot(
        FString& OutSnapshotPath,
        FString& OutError);

    bool ExportCanonicalWorldBackup(
        const FString& DestinationBackupPath,
        FString& OutBackupId,
        FString& OutError);

    bool ImportCanonicalWorldBackup(
        const FString& SourceBackupPath,
        FString& OutPreservedWorldPath,
        FString& OutError);

    bool ClearCanonicalWorld(
        bool bDeleteBackups,
        FString& OutError);

    UFUNCTION(BlueprintPure, Category = "OfflineGame|Performance")
    FOGPerformanceTelemetrySnapshot GetPerformanceTelemetry() const
    {
        return PerformanceTelemetry.Snapshot();
    }

    UFUNCTION(BlueprintCallable, Category = "OfflineGame|Performance")
    void SetPerformanceProfile(EOGPerformanceProfile Profile)
    {
        PerformanceTelemetry.SetProfile(Profile);
    }

    IOGWorldStore* GetWorldStore() const { return WorldStore.Get(); }

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|Combat")
    bool StartApprovedLuminousEncounter(const FOGTurnBattleState& State,
        const FOGEntityId& RulerId,FString& OutError);

    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|Combat")
    bool ApplyApprovedLuminousResolvedAction(
        const FOGResolvedCombatAction& Action,FString& OutError);

    UFUNCTION(BlueprintPure, Category="OfflineGame|Luminous|Combat")
    FOGTurnBattleState GetApprovedLuminousEncounterState() const;

    /** Resolve the exact version's full 3A/6P kit from owned WorldStore data
     * without creating a new character, changing equipment or inventing Rank. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Luminous|Combat")
    bool ReadOwnedApprovedLuminousSkills(
        const FOGEntityId& ManifestationId,const FOGEntityId& RulerId,
        FOGResolvedSkillSet& OutSkills,FString& OutError) const;


    /** Resolve any installed shared kit for an existing active, actually owned Manifestation. */
    UFUNCTION(BlueprintCallable,Category="OfflineGame|Characters")
    bool ReadOwnedCharacterSkills(const FOGEntityId& ManifestationId,const FOGEntityId& RulerId,
        FOGResolvedSkillSet& OutSkills,FString& Error) const;

    /** Production encounter integration: bind the EXISTING FOGTurnBattle to
     * real Registry, World and soul/blood providers. No fabricated defaults. */
    bool BindOwnedLuminousCombatAuthority(
        FOGTurnBattle& Battle,
        const FOGEntityId& RulerId,
        FOGLuminousCanonicalSources TrustedProviders,
        FString& OutError);

    /** Real GameCore-owned turn session. Requires registered formations,
     * triggers, installed effect providers and native World truth. */
    bool StartOwnedLuminousTurnBattle(
        const FOGTurnBattleState& InitialState,
        TArray<FOGCombatTriggerBinding> RegisteredTriggers,
        FOGCombatConditionEvaluator NativeConditions,
        FOGIdentityExclusivityContext IdentityContext,
        FOGRankSuppressionResolver RankResolver,
        const FOGEntityId& RulerId,
        FOGLuminousCanonicalSources TrustedProviders,
        FString& OutError,FName RecoveryProvider=NAME_None);
    /** Admit only installed authored assets for one existing participant, atomically. No command JSON can register effects. */
    bool ConfigureOwnedNativeAbilities(const FOGEntityId& UnitId,
        const TArray<UOGNativeCombatAbility*>& Abilities,const TArray<FOGContentId>& RequiredSkills,FString& Error);
    bool ApplyOwnedLuminousTurnAction(const FOGResolvedCombatAction& Action,
        FString& OutError);
    bool ResolveOwnedLuminousRegisteredContest(
        const FOGLuminousRegisteredEffectContest& Contest,
        const FOGLuminousRegisteredEffectRemoval& RemoveExactRegisteredInstance,
        bool& bOutWon, FString& OutError);
    /** Native extensions register their actual callback factory once; assets/saves cannot install code. */
    bool RegisterCombatRecoveryProvider(FOGCombatRecoveryProvider Provider,FString& Error);
    bool UnregisterCombatRecoveryProvider(FName Provider,FString& Error);
    /** Restores an existing committed encounter; never reruns costs/opening/rewards. */
    bool RestoreOwnedNativeEncounter(const FOGEntityId& Ruler,bool& Restored,FString& Error);
    bool CheckpointOwnedNativeEncounter(FString& Error);
    bool IsNativeEncounterAwaitingWorldRebind() const { return bNativeRecoveryAwaitingWorld; }
    bool IsRestoredNativeEncounter() const { return bNativeEncounterRestored; }
    /** Rebind notification is presentation-only and releases no gameplay event/reward. */
    bool CompleteNativeWorldRecovery(FString& Error);
    UPROPERTY(BlueprintReadOnly,Category="OfflineGame|Combat|Recovery") FString NativeEncounterRecoveryStatus;
    const FOGTurnBattle* GetOwnedLuminousTurnBattle() const { return OwnedLuminousTurnBattle.Get(); }
    /** Generic definition-based entry; resolved participants retain their existing canonical identities. */
    bool StartOwnedNativeEncounter(const UOGNativeEncounterDefinition& Definition,
        const TArray<FOGResolvedEncounterParticipant>& Participants,const FOGEntityId& Ruler,
        FOGIdentityExclusivityContext IdentityContext,FString& Error,FName RecoveryProvider=NAME_None);
    bool SetOwnedCombatExecutionMode(EOGCombatExecutionMode Mode,FString& Error);
    bool CanSetOwnedCombatExecutionMode(EOGCombatExecutionMode Mode,FString& Error) const;
    bool BeginOwnedWorldAbility(const FOGResolvedCombatAction& Root,FString& Error);
    bool ApplyOwnedWorldAbilityStep(const FOGEntityId& CastId,int32 Step,const TArray<FOGEntityId>& Contacts,FString& Error);
    bool CancelOwnedWorldAbility(const FOGEntityId& CastId,FString& Error);
    bool AdvanceOwnedWorldCombatTime(FString& Error);
    const FOGNativeBattleOutcome& GetLastNativeBattleOutcome() const { return LastNativeBattleOutcome; }
    FOGEntityId GetLastNativeBattleReceipt() const { return LastNativeBattleReceipt; }
    bool IsNativeOpeningPresentationCurrent() const { return LastNativeOpeningReceipt.IsValid() && LastNativeOpeningReceipt==LastNativeBattleReceipt; }
    FOGEntityId GetOwnedBattleRulerId() const { return OwnedLuminousTurnBattle ? ActiveCombatRulerId : FOGEntityId(); }

    /** Available before the existing resolved-action notifications fire. Empty
     * until an actual journal append succeeds; cleared on canonical release
     * and successful encounter replacement. Failed actions preserve the last
     * successfully recorded event. Shared by World and Ruler combat entry paths. */
    UFUNCTION(BlueprintPure, Category="OfflineGame|Combat|Provenance")
    FOGEntityId GetLastNativeCombatEventId() const { return LastNativeCombatEvent.EventId; }

    UFUNCTION(BlueprintPure, Category="OfflineGame|Combat|Provenance")
    FOGNativeCombatEventProvenance GetLastNativeCombatEventProvenance() const { return LastNativeCombatEvent; }

    UFUNCTION(BlueprintPure, Category = "OfflineGame|Core")
    int64 GetCanonicalWorldTick() const
    {
        return (bCoreReady || bReleasingCanonicalRuntime) && CanonicalClock ? CanonicalClock->GetCanonicalWorldTick() : -1;
    }

    FOGCanonicalClockRuntime* GetCanonicalClockRuntime() const
    {
        return (bCoreReady || bReleasingCanonicalRuntime) ? CanonicalClock.Get() : nullptr;
    }

    FOGFoundationStrategicRuntime* GetFoundationStrategicRuntime() const
    {
        return bCoreReady ? StrategicRuntime.Get() : nullptr;
    }

    // The installation host supplies byte/provenance and external-residency authority.
    // Imported world/package metadata cannot authorize physical mounting.
    bool ConfigureOptionalPackageHost(FOGOptionalPackageHostCallbacks Callbacks, FString& OutError);
    // Native installer boundary: Expected comes from trusted authored/authenticated
    // installation data, never from an imported world/package record.
    bool InstallVerifiedOptionalPackage(const FOGTrustedOptionalPackageInstall& Expected,
        const FString& SourcePak, FString& OutInstallUri, FString& OutError);
    FOGOptionalPackageHost* GetOptionalPackageHost() const
    {
        return (bCoreReady || bReleasingCanonicalRuntime) ? OptionalPackageHost.Get() : nullptr;
    }

private:
    TMap<FName,FOGCombatRecoveryProvider> CombatRecoveryProviders;
    FName PendingNativeRecoveryProvider;
    FName ActiveNativeRecoveryProvider;
    FOGEntityId PendingNativeRecoveryRuler;
    bool CaptureCombatRecoveryProvider(FName Provider,const FOGTurnBattle& Battle,const FOGEntityId& Ruler,
        int32& Version,FString& Payload,FString& Error) const;
    bool RestoreCombatRecoveryProvider(FName Provider,int32 Version,const FString& Payload,const FOGEntityId& Ruler,
        FOGCombatRecoveryBindings& Bindings,FString& Error);
    bool bPendingNativeRecovery=false;
    bool bActiveNativeRecovery=false;
    bool bNativeRecoveryAwaitingWorld=false;
    bool bNativeRecoveryBodiesRestored=false;
    bool bNativeEncounterRestored=false;
    FOGWorldEncounterPresentationCheckpoint NativeWorldRecoveryPresentation;
    FString PendingNativeRankRecovery;
    FString ActiveNativeRankRecovery;
    bool PersistNativeEncounter(const FOGTurnBattle& Candidate,const FOGEntityId& Ruler,
        const FOGWorldEvent& Receipt,const FOGResolvedCombatAction* Root,bool Opening,FString& Error);
    bool bSubmittingNativeAction=false;
    TSet<FOGEntityId> CommittedNativeActionIds;
    void PublishNativeAction(const FOGResolvedCombatAction& Action,const FOGWorldEvent& Event,const FOGTurnBattleState& Accepted);
#if WITH_DEV_AUTOMATION_TESTS
    friend class FOGNativeCombatEventProvenanceTest;
#endif
    bool InitializeCanonicalClock(FString& OutError);
    bool PumpCanonicalClock(FString& OutError);
    bool FlushCanonicalClockBoundary(FString& OutError);
    bool TickCanonicalClock(float DeltaSeconds);
    bool ReleaseCanonicalRuntime(FString* OutError = nullptr);

    UFUNCTION()
    void HandlePresentationModeChanged(EOGPresentationMode NewMode);

    bool TickPerformanceTelemetry(float DeltaSeconds);
    void HandleApplicationWillDeactivate();
    void HandleApplicationHasReactivated();
    void HandleApplicationWillEnterBackground();
    void HandleApplicationHasEnteredForeground();
    void CheckpointForLifecycleBoundary(const TCHAR* BoundaryName);
    void VerifyCoreAfterResume(const TCHAR* BoundaryName);

    TUniquePtr<IOGWorldStore> WorldStore;
    // Owned solely by canonical GameCore; torn down before replacing Store.
    TUniquePtr<FOGTurnBattle> OwnedLuminousTurnBattle;
    FOGEntityId ActiveCombatRulerId;
    FOGNativeCombatEventProvenance LastNativeCombatEvent;
    FOGEntityId LastNativeBattleReceipt;
    FOGEntityId LastNativeOpeningReceipt;
    FOGNativeBattleOutcome LastNativeBattleOutcome;
    bool CommitWorldCombatCandidate(FOGTurnBattle&& Candidate,FName Operation,const FOGResolvedCombatAction* Cause,
        int32 StepIndex,bool PublishRootMotion,FString& Error);
    bool ValidateOwnedWorldParticipants(const FOGTurnBattleState& State,const FOGEntityId& Ruler,FString& Error) const;
    bool TickWorldCombat(float DeltaSeconds);
    FString LastWorldCombatClockError;
    TSharedPtr<FOGWorldStoreCommittedEvents> ObservedWorldEventFeed;
    TMap<FOGEntityId,TSharedPtr<FOGWorldStoreCommittedEvents>> EntityEventFeeds;
    FDelegateHandle WorldEventFeedHandle;
    uint64 WorldEventFeedGeneration=0;
    TArray<FOGWorldEvent> QueuedCommittedWorldEvents;
    bool bFlushingCommittedWorldEvents=false;
    void BindWorldEventFeed();
    void ReleaseWorldEventFeed();
    void QueueCommittedWorldEvents(const TArray<FOGWorldEvent>& Events,uint64 Generation);
    void FlushCommittedWorldEvents();
    FTSTicker::FDelegateHandle WorldCombatTickerHandle;
    TWeakObjectPtr<UWorld> WorldCombatClockWorld;
    double LastWorldCombatSeconds=-1;
    double WorldCombatFractionalMilliseconds=0;
    TMap<FOGEntityId,FOGResolvedCombatAction> NativeWorldCastRequests;
    TUniquePtr<FOGCanonicalClockRuntime> CanonicalClock;
    TUniquePtr<FOGFoundationStrategicRuntime> StrategicRuntime;
    TSharedPtr<FOGLocallyInstalledPackageProvider> InstalledPackageProvider;
    TUniquePtr<FOGOptionalPackageHost> OptionalPackageHost;
    FOGOptionalPackageHostCallbacks OptionalPackageCallbacks;
    bool bOptionalPackageHostConfigured = false;
    FOGPerformanceTelemetry PerformanceTelemetry;
    FTSTicker::FDelegateHandle PerformanceTickerHandle;
    FTSTicker::FDelegateHandle CanonicalClockTickerHandle;
    FDelegateHandle WillDeactivateHandle;
    FDelegateHandle HasReactivatedHandle;
    FDelegateHandle WillEnterBackgroundHandle;
    FDelegateHandle HasEnteredForegroundHandle;
    bool bCoreReady = false;
    bool bReleasingCanonicalRuntime = false;
    bool bApplicationBackgrounded = false;
    bool bCanonicalClockNeedsCatchup = false;
};
