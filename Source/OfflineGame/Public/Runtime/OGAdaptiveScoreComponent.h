#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Runtime/OGSemanticAudioRuntime.h"
#include "OGAdaptiveScoreComponent.generated.h"

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGAdaptiveScoreRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RuleId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> RequiredTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> ExcludedTags;
    /** Authored sound/MetaSound owns looping and any synchronized stems. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGSemanticAudioEventDefinition Audio;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float FadeInSeconds = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float FadeOutSeconds = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float MinimumHoldSeconds = 0.f;
};

/** Original score/rules supplied by content production, no generated music or lore. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGAdaptiveScoreDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGAdaptiveScoreRule> Rules;
    bool Validate(FString& Error) const;
    /** Equal priorities use the RuleId lexical order, independent of array order. */
    const FOGAdaptiveScoreRule* Select(const TSet<FName>& Tags, bool bWorldPresentation = true) const;
};

/** Read-only adaptive soundtrack director using the pawn's existing semantic audio/profile. */
UCLASS(ClassGroup=(OfflineGame), meta=(BlueprintSpawnableComponent))
class OFFLINEGAME_API UOGAdaptiveScoreComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UOGAdaptiveScoreComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio|Score")
    TSoftObjectPtr<UOGAdaptiveScoreDefinition> Score;
    /** Named state from actual biome/faction/weather/cinematic presentation providers.
     * Reserved mode.* and battle.* are always read from the native core. */
    UFUNCTION(BlueprintCallable, Category="Audio|Score")
    bool SetContextTags(FName Provider, const TArray<FName>& Tags);
    UFUNCTION(BlueprintCallable, Category="Audio|Score")
    void ClearContextTags(FName Provider);
    UFUNCTION(BlueprintPure, Category="Audio|Score")
    FName GetCurrentRule() const { return CurrentRule; }
    /** Refresh now at a meaningful transition; does not mutate simulation. */
    UFUNCTION(BlueprintCallable, Category="Audio|Score")
    void RefreshScore();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
private:
    UPROPERTY(Transient) TObjectPtr<UOGAdaptiveScoreDefinition> ActiveScore;
    UPROPERTY(Transient) TObjectPtr<UOGSemanticAudioRuntime> Audio;
    TMap<FName, TArray<FName>> ProviderTags;
    TMap<FName, FName> EventsByRule;
    TSet<FName> FadingEvents;
    FName CurrentRule;
    FName PendingRule;
    double CurrentStartedAt = 0.0;
    bool bAttemptedLoad = false;
    void HandleEventPlayed(FName Event, UAudioComponent* Component);
    void CommitTransition(FName RuleId);
    void ClearOwnedScore();
};
