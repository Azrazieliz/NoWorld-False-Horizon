#include "Runtime/OGAdaptiveScoreComponent.h"

#include "Combat/OGTurnBattle.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGPresentationModeSubsystem.h"

namespace
{
bool ReservedScoreTag(FName Tag)
{
    const FString Name = Tag.ToString();
    return Name.StartsWith(TEXT("mode.")) || Name.StartsWith(TEXT("battle."));
}
}

bool UOGAdaptiveScoreDefinition::Validate(FString& Error) const
{
    Error.Reset();
    TSet<FName> Ids;
    for (const FOGAdaptiveScoreRule& Rule : Rules)
    {
        if (Rule.RuleId.IsNone() || Ids.Contains(Rule.RuleId) ||
            Rule.Audio.Category != EOGSemanticAudioCategory::Music || Rule.Audio.bSpatial ||
            (!Rule.Audio.Sound && Rule.Audio.SoftSound.IsNull()) ||
            Rule.Audio.Haptic || !Rule.Audio.SoftHaptic.IsNull() ||
            !FMath::IsFinite(Rule.Audio.Gain) || Rule.Audio.Gain < 0.f || Rule.Audio.Gain > 1.f ||
            !FMath::IsFinite(Rule.FadeInSeconds) || Rule.FadeInSeconds < 0.f ||
            !FMath::IsFinite(Rule.FadeOutSeconds) || Rule.FadeOutSeconds < 0.f ||
            !FMath::IsFinite(Rule.MinimumHoldSeconds) || Rule.MinimumHoldSeconds < 0.f ||
            Rule.Audio.CooldownSeconds != 0.f)
        {
            Error = TEXT("Score rules need unique IDs, real nonspatial music, finite envelopes and no haptic/cooldown.");
            return false;
        }
        Ids.Add(Rule.RuleId);
        TSet<FName> Seen;
        for (FName Tag : Rule.RequiredTags)
        {
            if (Tag.IsNone() || Seen.Contains(Tag)) { Error = TEXT("Duplicate/empty required score tag."); return false; }
            Seen.Add(Tag);
        }
        for (FName Tag : Rule.ExcludedTags)
        {
            if (Tag.IsNone() || Seen.Contains(Tag)) { Error = TEXT("Duplicate/contradictory score tag."); return false; }
            Seen.Add(Tag);
        }
    }
    return true;
}

const FOGAdaptiveScoreRule* UOGAdaptiveScoreDefinition::Select(const TSet<FName>& Tags, bool bWorldPresentation) const
{
    const FOGAdaptiveScoreRule* Selected = nullptr;
    for (const FOGAdaptiveScoreRule& Rule : Rules)
    {
        if (Rule.Audio.bWorldOnly && !bWorldPresentation) continue;
        bool bMatches = true;
        for (FName Tag : Rule.RequiredTags) if (!Tags.Contains(Tag)) { bMatches = false; break; }
        if (!bMatches) continue;
        for (FName Tag : Rule.ExcludedTags) if (Tags.Contains(Tag)) { bMatches = false; break; }
        if (bMatches && (!Selected || Rule.Priority > Selected->Priority ||
            (Rule.Priority == Selected->Priority && Rule.RuleId.LexicalLess(Selected->RuleId)))) Selected = &Rule;
    }
    return Selected;
}

UOGAdaptiveScoreComponent::UOGAdaptiveScoreComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.25f;
    ComponentTags.Add(TEXT("OG.PresentationPersistent"));
}

bool UOGAdaptiveScoreComponent::SetContextTags(FName Provider, const TArray<FName>& Tags)
{
    if (Provider.IsNone()) return false;
    TSet<FName> Seen;
    for (FName Tag : Tags)
    {
        if (Tag.IsNone() || ReservedScoreTag(Tag) || Seen.Contains(Tag)) return false;
        Seen.Add(Tag);
    }
    ProviderTags.Add(Provider, Tags);
    RefreshScore();
    return true;
}

void UOGAdaptiveScoreComponent::ClearContextTags(FName Provider)
{
    ProviderTags.Remove(Provider);
    RefreshScore();
}

void UOGAdaptiveScoreComponent::BeginPlay()
{
    Super::BeginPlay();
    Audio = GetOwner() ? GetOwner()->FindComponentByClass<UOGSemanticAudioRuntime>() : nullptr;
    if (Audio) Audio->OnSemanticEventPlayed.AddUObject(this, &ThisClass::HandleEventPlayed);
    RefreshScore();
}

void UOGAdaptiveScoreComponent::RefreshScore()
{
    if (!GetWorld() || !Audio || !GetWorld()->IsGameWorld()) return;
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn && !Pawn->IsLocallyControlled()) { ClearOwnedScore(); return; }
    if (!bAttemptedLoad)
    {
        bAttemptedLoad = true;
        ActiveScore = Score.LoadSynchronous();
        FString Error;
        if (ActiveScore && !ActiveScore->Validate(Error))
        {
            UE_LOG(LogTemp, Error, TEXT("Authored adaptive score rejected: %s"), *Error);
            ActiveScore = nullptr;
        }
        if (ActiveScore)
            for (const FOGAdaptiveScoreRule& Rule : ActiveScore->Rules)
            {
                const FName Event(*FString::Printf(TEXT("Score.%s.%s"), *GetName(), *Rule.RuleId.ToString()));
                Audio->RegisterEventDefinition(Event, Rule.Audio);
                EventsByRule.Add(Rule.RuleId, Event);
            }
    }
    if (!ActiveScore) return;
    UGameInstance* GI = GetWorld()->GetGameInstance();
    const UOGGameCoreSubsystem* Core = GI ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    if (!Core || !Core->IsCoreReady()) { ClearOwnedScore(); return; }
    TSet<FName> Tags;
    const UOGPresentationModeSubsystem* Modes = GI->GetSubsystem<UOGPresentationModeSubsystem>();
    const bool bWorld = !Modes || Modes->GetMode() == EOGPresentationMode::World;
    Tags.Add(bWorld ? FName(TEXT("mode.world")) : FName(TEXT("mode.ruler")));
    const FOGTurnBattle* Battle = Core->GetOwnedLuminousTurnBattle();
    Tags.Add(Battle && Battle->GetState().Status == EOGTurnBattleStatus::Running
        ? FName(TEXT("battle.running")) : FName(TEXT("battle.idle")));
    for (const auto& Pair : ProviderTags) for (FName Tag : Pair.Value) Tags.Add(Tag);
    const FOGAdaptiveScoreRule* Desired = ActiveScore->Select(Tags, bWorld);
    const FName DesiredId = Desired ? Desired->RuleId : NAME_None;
    const double Now = GetWorld()->GetTimeSeconds();
    // Retire a completed fade from our bookkeeping without touching other audio.
    for (auto It = FadingEvents.CreateIterator(); It; ++It)
        if (!Audio->IsSemanticEventActiveOrPending(*It)) It.RemoveCurrent();
    const FOGAdaptiveScoreRule* Current = ActiveScore->Rules.FindByPredicate(
        [this](const FOGAdaptiveScoreRule& R) { return R.RuleId == CurrentRule; });
    // Privacy/mode invalidation never waits for a musical minimum hold.
    bool bCurrentEligible = Current && (!Current->Audio.bWorldOnly || bWorld);
    if (Current)
    {
        for (FName Tag : Current->RequiredTags) bCurrentEligible &= Tags.Contains(Tag);
        for (FName Tag : Current->ExcludedTags) bCurrentEligible &= !Tags.Contains(Tag);
    }
    const bool bHoldingCurrent = Current && bCurrentEligible && DesiredId != CurrentRule &&
        Now - CurrentStartedAt < Current->MinimumHoldSeconds;
    // Cancel an obsolete async load even when the new selection is the
    // currently audible track. Its completion must not change the score later.
    if (!PendingRule.IsNone() && (PendingRule != DesiredId || bHoldingCurrent))
    {
        Audio->StopSemanticEvent(EventsByRule.FindRef(PendingRule));
        PendingRule = NAME_None;
    }
    if (bHoldingCurrent) return;
    if (DesiredId == CurrentRule && !CurrentRule.IsNone() &&
        Audio->IsSemanticEventActiveOrPending(EventsByRule.FindRef(CurrentRule))) return;
    if (DesiredId == PendingRule && !PendingRule.IsNone() &&
        Audio->IsSemanticEventActiveOrPending(EventsByRule.FindRef(PendingRule))) return;
    if (!PendingRule.IsNone()) Audio->StopSemanticEvent(EventsByRule.FindRef(PendingRule));
    PendingRule = NAME_None;
    if (!Desired)
    {
        CommitTransition(NAME_None);
        return;
    }
    const FName Event = EventsByRule.FindRef(DesiredId);
    // A fast reversal can reuse the still-fading original sound without restarting it.
    if (FadingEvents.Remove(Event) > 0 && Audio->IsSemanticEventActiveOrPending(Event))
    {
        CommitTransition(DesiredId);
        return;
    }
    PendingRule = DesiredId; // set before an immediate synchronous completion callback
    if (!Audio->PlaySemanticEvent(Event, GetOwner()->GetActorLocation())) PendingRule = NAME_None;
}

void UOGAdaptiveScoreComponent::HandleEventPlayed(FName Event, UAudioComponent* Component)
{
    if (!Component || PendingRule.IsNone() || EventsByRule.FindRef(PendingRule) != Event) return;
    const FName CompletedRule = PendingRule;
    // Context may have changed while the soft asset loaded. Re-evaluation
    // cancels obsolete playback and applies the current rule's hold policy.
    RefreshScore();
    if (PendingRule != CompletedRule || !Audio->IsSemanticEventActiveOrPending(Event)) return;
    Audio->TransitionSemanticEvent(Event, 0.f, 0.f, false);
    CommitTransition(PendingRule);
}

void UOGAdaptiveScoreComponent::CommitTransition(FName RuleId)
{
    if (!Audio || !ActiveScore) return;
    if (!CurrentRule.IsNone() && CurrentRule != RuleId)
    {
        const FOGAdaptiveScoreRule* Previous = ActiveScore->Rules.FindByPredicate(
            [this](const FOGAdaptiveScoreRule& R) { return R.RuleId == CurrentRule; });
        const FName Old = EventsByRule.FindRef(CurrentRule);
        Audio->TransitionSemanticEvent(Old, 0.f, Previous ? Previous->FadeOutSeconds : 0.f, true);
        FadingEvents.Add(Old);
    }
    CurrentRule = RuleId;
    PendingRule = NAME_None;
    CurrentStartedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (!RuleId.IsNone())
    {
        const FOGAdaptiveScoreRule* Next = ActiveScore->Rules.FindByPredicate(
            [RuleId](const FOGAdaptiveScoreRule& R) { return R.RuleId == RuleId; });
        Audio->TransitionSemanticEvent(EventsByRule.FindRef(RuleId), 1.f, Next ? Next->FadeInSeconds : 0.f, false);
    }
}

void UOGAdaptiveScoreComponent::ClearOwnedScore()
{
    if (Audio) for (const auto& Pair : EventsByRule) Audio->StopSemanticEvent(Pair.Value);
    CurrentRule = PendingRule = NAME_None;
    FadingEvents.Reset();
}

void UOGAdaptiveScoreComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshScore();
}

void UOGAdaptiveScoreComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearOwnedScore();
    if (Audio)
    {
        Audio->OnSemanticEventPlayed.RemoveAll(this);
        for (const auto& Pair : EventsByRule) Audio->UnregisterEventDefinition(Pair.Value);
    }
    EventsByRule.Reset();
    Super::EndPlay(Reason);
}
