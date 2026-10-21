#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Persistence/OGWorldStore.h"
#include "Combat/OGLuminousCanonicalAuthorityBinding.h"
#include "Runtime/OGPerformanceTelemetry.h"
#include "Runtime/OGCanonicalClockRuntime.h"
#include "Runtime/OGFoundationStrategicRuntime.h"
#include "Runtime/OGOptionalPackageHost.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OGGameCoreSubsystem.generated.h"

class FOGLocallyInstalledPackageProvider;
struct FOGTrustedOptionalPackageInstall;

DECLARE_MULTICAST_DELEGATE(FOGCanonicalRuntimeReleasing);
// Presentation-only signal AFTER an authentic, successful native turn action.
// No gameplay executor may infer a hit or award a resource from this event.
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FOGLuminousCombatActionResolvedEvent,const FOGEntityId&,const FOGContentId&);

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
    FOGLuminousCombatActionResolvedEvent OnLuminousCombatActionResolved;
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
        FString& OutError);
    bool ApplyOwnedLuminousTurnAction(const FOGResolvedCombatAction& Action,
        FString& OutError);
    bool ResolveOwnedLuminousRegisteredContest(
        const FOGLuminousRegisteredEffectContest& Contest,
        const FOGLuminousRegisteredEffectRemoval& RemoveExactRegisteredInstance,
        bool& bOutWon, FString& OutError);
    const FOGTurnBattle* GetOwnedLuminousTurnBattle() const { return OwnedLuminousTurnBattle.Get(); }

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