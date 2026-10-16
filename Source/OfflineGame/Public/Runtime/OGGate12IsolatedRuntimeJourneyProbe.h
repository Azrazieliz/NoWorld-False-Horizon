#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "OGGate12IsolatedRuntimeJourneyProbe.generated.h"

/**
 * Real-game no-cook integration check, explicitly confined to an isolated
 * project directory. The test uses existing HUD click handlers and canonical
 * local-Ruler transactions, NEVER synthetic ownership or a production save.
 *
 * Not enabled in shipping, PIE development sessions, ordinary gameplay, or
 * original project Saved paths. Opt-in only with -Gate12RuntimeJourney.
 */
UCLASS()
class OFFLINEGAME_API UOGGate12IsolatedRuntimeJourneyProbe final
    : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    struct FJourneyStep
    {
        FString Name;
        FString Detail;
        bool bPassed = false;
    };

    void RunNextStep();
    void ScheduleNext(float Seconds);
    void Record(const FString& Name, bool bSuccess, const FString& Detail);
    void SaveReceipt(bool bComplete);
    void RequestActualGameScreenshot(const FString& Label);

    TArray<FJourneyStep> Events;
    FTimerHandle NextStepTimer;
    FString ReceiptPath;
    int32 Phase = 0;
    int32 ReadinessRetries = 0;
    bool bFailed = false;
    FGuid InitialPawnGuid;
    TWeakObjectPtr<APawn> InitialPawn;
    FString RulerIdAtCreation;
};
