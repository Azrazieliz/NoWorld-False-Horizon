#include "Combat/OGNativeTurnAgentComponent.h"
#include "Content/OGDefinitionLibrary.h"
#include "Characters/OGCharacterGameplayDefinition.h"

#include "Animation/OGSharedMotionControllerComponent.h"
#include "Characters/OGCharacterAppearanceComponent.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Combat/OGNativeCombatAbility.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGPresentationModeSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "World/OGStartingRegionPresentation.h"

namespace
{
bool ReadActualNpc(IOGWorldStore& Store, const UOGNativeTurnPolicy& Policy,
    const FOGCombatUnitState& Unit, const FOGEntityId& LocalRuler, FName& Phase, FString& Error)
{
    Phase = NAME_None;
    bool bFound = false; FName Kind; FString Saved; int64 Revision = 0;
    if (!Store.TryReadEntity(Unit.UnitEntityId, bFound, Kind, Saved, Revision, Error)) return false;
    if (!bFound || Kind == TEXT("ruler"))
    { Error = TEXT("Opponent must already exist as a canonical non-Ruler entity."); return false; }
    TSharedPtr<FJsonObject> State;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Saved), State) || !State || State->HasField(TEXT("diagnosticFixture")))
    { Error = TEXT("Opponent state is missing, malformed or diagnostic-only."); return false; }
    FString Identity, Version, Life;
    if (!State->TryGetStringField(Policy.CanonicalIdentityField.ToString(), Identity) || Identity != Unit.IdentityId.ToString() ||
        !State->TryGetStringField(Policy.CanonicalVersionField.ToString(), Version) || Version != Unit.SkillSet.CurrentForm.ToString() ||
        !State->TryGetStringField(Policy.CanonicalLifeField.ToString(), Life) || Life != Policy.RequiredLifeValue)
    { Error = TEXT("Canonical opponent identity, kit or life state does not match the authored policy."); return false; }
    FOGCharacterManifestationRecord Copy;
    if (!Store.TryReadCharacterManifestation(Unit.UnitEntityId, bFound, Copy, Error)) return false;
    if (bFound && (Copy.IdentityId != Unit.IdentityId || Copy.ActiveVersionId != Unit.SkillSet.CurrentForm ||
        Copy.LifecycleState != TEXT("active") || (LocalRuler.IsValid() && Copy.OwningRulerId == LocalRuler)))
    { Error = TEXT("An opponent agent cannot control the local Ruler's owned roster or an invalid manifestation."); return false; }
    FOGWorldPresenceRecord Presence;
    if (!Store.TryReadWorldPresence(Unit.UnitEntityId, bFound, Presence, Error) || !bFound)
    { if (Error.IsEmpty()) Error = TEXT("Canonical opponent has no real World presence."); return false; }
    if (!Policy.CanonicalPhaseField.IsNone())
    {
        FString SavedPhase;
        if (!State->TryGetStringField(Policy.CanonicalPhaseField.ToString(), SavedPhase) || SavedPhase.IsEmpty())
        { Error = TEXT("Authored canonical opponent phase is unavailable."); return false; }
        Phase = FName(*SavedPhase);
    }
    return true;
}

UOGSharedMotionControllerComponent* ActualMotion(AActor* Actor)
{
    if (!Actor) return nullptr;
    if (UOGCharacterAppearanceComponent* Appearance = Actor->FindComponentByClass<UOGCharacterAppearanceComponent>())
        if (UOGSharedMotionControllerComponent* Motion = Appearance->GetMotionController()) return Motion;
    return Actor->FindComponentByClass<UOGSharedMotionControllerComponent>();
}
}

UOGNativeTurnAgentComponent::UOGNativeTurnAgentComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}

void UOGNativeTurnAgentComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Core.IsValid()) Core->OnCanonicalRuntimeReleasing.RemoveAll(this);
    Core = GetWorld() && GetWorld()->GetGameInstance()
        ? GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    if (Core.IsValid()) Core->OnCanonicalRuntimeReleasing.AddUObject(this, &ThisClass::HandleRuntimeReleasing);
}

bool UOGNativeTurnAgentComponent::ConfigureBattleUnit(const FOGEntityId& BattleId,
    const FOGEntityId& UnitId, FString& Error)
{
    Error.Reset();
    UOGGameCoreSubsystem* Runtime = GetWorld() && GetWorld()->GetGameInstance()
        ? GetWorld()->GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const FOGTurnBattle* Battle = Runtime ? Runtime->GetOwnedLuminousTurnBattle() : nullptr;
    UOGNativeTurnPolicy* CandidatePolicy=Policy.LoadSynchronous();
    if (Policy.IsNull() && Battle)
    {
        const auto* Unit=Battle->GetState().Units.FindByPredicate([&](const auto& U){return U.UnitEntityId==UnitId;});
        auto* Library=UOGDefinitionLibrary::Get();
        const auto* Kit=Library && Unit?Cast<UOGCharacterGameplayDefinition>(
            Library->Resolve(TEXT("character"),Unit->SkillSet.CurrentForm)):nullptr;
        if (Kit && Kit->IdentityId==Unit->IdentityId && Kit->TurnPolicy.Id.IsValid())
            CandidatePolicy=Cast<UOGNativeTurnPolicy>(Library->Resolve(Kit->TurnPolicy.System,Kit->TurnPolicy.Id));
    }
    TArray<UOGCanonicalCombatActorBindingComponent*> SourceBindings;
    if (GetOwner()) GetOwner()->GetComponents<UOGCanonicalCombatActorBindingComponent>(SourceBindings);
    UOGCanonicalCombatActorBindingComponent* CandidateBody = SourceBindings.Num() == 1 ? SourceBindings[0] : nullptr;
    if (!IsInGameThread() || bSubmitting || !GetWorld() || !GetWorld()->IsGameWorld() ||
        !Runtime || !Runtime->IsCoreReady() || !Runtime->GetWorldStore() || !Battle ||
        Battle->GetState().BattleId != BattleId || Battle->GetState().Status != EOGTurnBattleStatus::Running ||
        !CandidatePolicy || !CandidatePolicy->Validate(Error) || !CandidateBody ||
        !CandidateBody->IsBoundTo(BattleId, UnitId))
    { if (Error.IsEmpty()) Error = TEXT("An authored policy and existing actual native battle body are required."); return false; }
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (Pawn && Cast<APlayerController>(Pawn->GetController()))
    { Error = TEXT("Opponent agents never command a human-controlled pawn."); return false; }
    const FOGCombatUnitState* Unit = Battle->GetState().Units.FindByPredicate(
        [&UnitId](const FOGCombatUnitState& U) { return U.UnitEntityId == UnitId; });
    const UOGGate12RulerSessionSubsystem* Session = GetWorld()->GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>();
    if (!Session || !Session->IsBoundToStoredRuler() || Session->BoundRulerId != Runtime->GetOwnedBattleRulerId() ||
        !Unit || !Unit->CanAct() || !CandidatePolicy->MatchesSource(Unit->IdentityId,Unit->SkillSet.CurrentForm) ||
        UnitId == Session->BoundRulerId || UnitId == Session->SelectedManifestationId)
    { Error = TEXT("Opponent policy cannot borrow a different source unit, kit or the local player."); return false; }
    for (const FOGNativeTurnRule& Rule : CandidatePolicy->Rules)
        if (!Unit->SkillSet.ActiveSkills.Contains(Rule.SkillId) && Unit->SkillSet.UltimateSkill != Rule.SkillId)
        { Error = TEXT("An AI rule refers to an ability absent from the source unit's exact kit."); return false; }
    FName Phase;
    if (!ReadActualNpc(*Runtime->GetWorldStore(), *CandidatePolicy, *Unit,
        Session ? Session->BoundRulerId : FOGEntityId(), Phase, Error)) return false;
    TArray<UOGNativeCombatAbility*> Abilities; TArray<FOGContentId> Required;
    for (const auto& Rule:CandidatePolicy->Rules)
    {
        Required.AddUnique(Rule.SkillId);
        if (!Rule.NativeAbility.IsNull())
        {
            auto* Ability=Rule.NativeAbility.LoadSynchronous();
            if (!Ability) { Error=TEXT("Authored opponent ability asset is unavailable."); return false; }
            if (Ability->Definition.SkillId!=Rule.SkillId)
            { Error=TEXT("AI rule and actual ability asset have different skill identities."); return false; }
            Abilities.AddUnique(Ability);
        }
    }
    if (!Runtime->ConfigureOwnedNativeAbilities(UnitId,Abilities,Required,Error)) return false;

    if (Core.IsValid()) Core->OnCanonicalRuntimeReleasing.RemoveAll(this);
    Core = Runtime; Body = CandidateBody; ActivePolicy = CandidatePolicy;
    BoundBattleId = BattleId; BoundUnitId = UnitId;
    Core->OnCanonicalRuntimeReleasing.AddUObject(this, &ThisClass::HandleRuntimeReleasing);
    RetryDecision();
    Status.Reset();
    return true;
}

void UOGNativeTurnAgentComponent::RetryDecision()
{
    bAutoBindingEnabled = true;
    FailedSequence = ObservedSequence = -1; FailedAbilityRevision=-1;
    ReadyAtSeconds = -1.0;
    LastAutoBattleId = LastAutoUnitId = FOGEntityId(); LastAutoAbilityRevision=-1; LastAutoDefinitionRevision=-1;
}

void UOGNativeTurnAgentComponent::ReleaseBattleUnit()
{
    BoundBattleId = BoundUnitId = FOGEntityId();
    ActivePolicy = nullptr; Body.Reset(); RetryDecision();
    bAutoBindingEnabled = false;
}

bool UOGNativeTurnAgentComponent::ReadCanonicalPhase(const FOGCombatUnitState& Unit,
    FName& OutPhase, FString& Error) const
{
    if (!Core.IsValid() || !Core->IsCoreReady() || !Core->GetWorldStore() || !ActivePolicy) return false;
    const UOGGate12RulerSessionSubsystem* Session = GetWorld()->GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>();
    if (!Session || !Session->IsBoundToStoredRuler() || Session->BoundRulerId != Core->GetOwnedBattleRulerId())
    { Error = TEXT("The local Ruler no longer owns this actual encounter."); return false; }
    return ReadActualNpc(*Core->GetWorldStore(), *ActivePolicy, Unit,
        Session ? Session->BoundRulerId : FOGEntityId(), OutPhase, Error);
}

bool UOGNativeTurnAgentComponent::StepOwnTurn(FString& Error)
{
    Error.Reset();
    const FOGTurnBattle* Battle = Core.IsValid() ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!IsInGameThread() || bSubmitting || !Core.IsValid() || !Core->IsCoreReady() || !Battle || !ActivePolicy ||
        !Body.IsValid() || !Body->IsBoundTo(BoundBattleId, BoundUnitId) ||
        Battle->GetState().BattleId != BoundBattleId || Battle->GetState().Status != EOGTurnBattleStatus::Running)
    { Error = TEXT("No live authored opponent turn is bound."); return false; }
    const UOGPresentationModeSubsystem* Mode = GetWorld()->GetGameInstance()->GetSubsystem<UOGPresentationModeSubsystem>();
    if (Mode && Mode->GetMode() != EOGPresentationMode::World) return false;
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && Cast<APlayerController>(OwnerPawn->GetController()))
    { Error = TEXT("Human control replaced this opponent's AI control."); Status = Error; ReadyAtSeconds = -1.0; return false; }
    const int32 Acting = Battle->SelectNextActingUnitIndex();
    if (!Battle->GetState().Units.IsValidIndex(Acting) || Battle->GetState().Units[Acting].UnitEntityId != BoundUnitId) return false;
    const int64 Sequence = Battle->GetLog().GetNextSequence();
    if (FailedSequence == Sequence && FailedAbilityRevision==Battle->GetNativeAbilityRevision()) { Error = Status; return false; }
    TGuardValue<bool> Submitting(bSubmitting, true);
    FName Phase;
    if (!ReadCanonicalPhase(Battle->GetState().Units[Acting], Phase, Error))
    { FailedSequence = Sequence; FailedAbilityRevision=Battle->GetNativeAbilityRevision(); Status = Error; return false; }
    TSet<FName> RejectedRules;
    FString LastFailure;
    for (int32 Attempt = 0; Attempt < ActivePolicy->Rules.Num(); ++Attempt)
    {
        FOGResolvedCombatAction Proposal, Action; FName Rule;
        if (!ActivePolicy->BuildDecision(Battle->GetState(), BoundUnitId, Phase, Proposal, Rule, Error, &RejectedRules)) break;
        RejectedRules.Add(Rule);
        bool bArea = false;
        if (!Battle->ExpandApprovedFieldTargets(Proposal, Action, bArea, LastFailure)) continue;
        TMap<FOGEntityId, FOGLargeNumber> Before;
        for (const FOGCombatUnitState& Unit : Battle->GetState().Units) Before.Add(Unit.UnitEntityId, Unit.CurrentHp);
        if (!Core->ApplyOwnedLuminousTurnAction(Action, LastFailure)) continue;
        Battle = Core->GetOwnedLuminousTurnBattle();
        if (!Battle || Battle->GetState().BattleId != BoundBattleId) return true;
        AActor* FirstTarget = nullptr;
        APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0);
        AOGWorldPresentationHud* Hud = Player ? Cast<AOGWorldPresentationHud>(Player->GetHUD()) : nullptr;
        for (const FOGCombatUnitState& Unit : Battle->GetState().Units)
        {
            AActor* Target = UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(GetWorld(), BoundBattleId, Unit.UnitEntityId);
            if (Target && !FirstTarget && Action.TargetUnitIds.Contains(Unit.UnitEntityId)) FirstTarget = Target;
            const FOGLargeNumber* OldHp = Before.Find(Unit.UnitEntityId);
            if (!Target || !OldHp || FOGLargeNumber::Compare(*OldHp, Unit.CurrentHp) == 0) continue;
            if (Hud) Hud->RegisterCanonicalCombatImpact(Target, *OldHp, Unit.CurrentHp);
            if (UOGSharedMotionControllerComponent* Motion = ActualMotion(Target))
                Motion->PlayAction(!Unit.IsAlive() ? FName(TEXT("Combat.Reaction.Death")) :
                    FOGLargeNumber::Compare(Unit.CurrentHp, *OldHp) < 0 ? FName(TEXT("Combat.Reaction.Hit")) : FName(TEXT("Combat.Reaction.Heal")),
                    TEXT("Combat"), false, 1);
        }
        if (FirstTarget && FirstTarget != GetOwner())
        {
            const FVector Delta = FirstTarget->GetActorLocation() - GetOwner()->GetActorLocation();
            if (Delta.SizeSquared2D() > SMALL_NUMBER) GetOwner()->SetActorRotation(FRotator(0.f, Delta.Rotation().Yaw, 0.f));
        }
        if (UOGSharedMotionControllerComponent* Motion = ActualMotion(GetOwner()))
        {
            const FOGCombatUnitState* Source = Battle->GetState().Units.FindByPredicate(
                [this](const FOGCombatUnitState& U) { return U.UnitEntityId == BoundUnitId; });
            FString Semantic = Source && Action.SkillId == Source->SkillSet.UltimateSkill ? TEXT("Combat.Ultimate.") : TEXT("Combat.Skill.");
            Semantic += Action.SkillId.ToString();
            TSharedPtr<FJsonObject> Resolution; FString Branch;
            if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Action.ResolutionJson), Resolution) && Resolution &&
                Resolution->TryGetStringField(TEXT("branch"), Branch) && !Branch.IsEmpty()) Semantic += TEXT(".") + Branch.ToLower();
            const FName Authored=Source?Battle->ResolveNativeAbilityMotion(*Source,Action.SkillId):NAME_None;
            Motion->PlayAction(Authored.IsNone()?FName(*Semantic):Authored, TEXT("Combat"), false, 1);
        }
        Status.Reset(); FailedSequence = -1; ReadyAtSeconds = -1.0;
        return true;
    }
    FailedSequence = Battle->GetLog().GetNextSequence(); FailedAbilityRevision=Battle->GetNativeAbilityRevision();
    Error = LastFailure.IsEmpty() ? Error : LastFailure;
    Status = Error.IsEmpty() ? TEXT("No approved opponent rule can execute in the current battle state.") : Error;
    UE_LOG(LogTemp, Warning, TEXT("Native opponent waits for a changed state or repaired source: %s"), *Status);
    return false;
}

void UOGNativeTurnAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bAutoBindingEnabled || !Core.IsValid() || !Core->IsCoreReady() || bSubmitting) return;
    const UOGPresentationModeSubsystem* Mode = GetWorld()->GetGameInstance()->GetSubsystem<UOGPresentationModeSubsystem>();
    if (Mode && Mode->GetMode() != EOGPresentationMode::World) { ReadyAtSeconds = -1.0; return; }
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && Cast<APlayerController>(OwnerPawn->GetController())) { ReadyAtSeconds = -1.0; return; }
    const FOGTurnBattle* Battle = Core->GetOwnedLuminousTurnBattle();
    if (!Battle || Battle->GetState().Status != EOGTurnBattleStatus::Running) { ReadyAtSeconds = -1.0; return; }
    if (BoundBattleId != Battle->GetState().BattleId || !Body.IsValid() || !Body->IsBoundTo(BoundBattleId, BoundUnitId))
    {
        UOGCanonicalCombatActorBindingComponent* Existing = GetOwner()->FindComponentByClass<UOGCanonicalCombatActorBindingComponent>();
        if (!Existing || Existing->GetBoundBattleId() != Battle->GetState().BattleId || !Existing->GetBoundUnitId().IsValid()) return;
        if (LastAutoBattleId == Existing->GetBoundBattleId() && LastAutoUnitId == Existing->GetBoundUnitId() &&
            LastAutoAbilityRevision==Battle->GetNativeAbilityRevision() &&
            LastAutoDefinitionRevision==(UOGDefinitionLibrary::Get()?UOGDefinitionLibrary::Get()->GetRevision():-1)) return;
        LastAutoBattleId = Existing->GetBoundBattleId(); LastAutoUnitId = Existing->GetBoundUnitId();
        LastAutoAbilityRevision=Battle->GetNativeAbilityRevision();
        LastAutoDefinitionRevision=UOGDefinitionLibrary::Get()?UOGDefinitionLibrary::Get()->GetRevision():-1;
        FString Error;
        if (!ConfigureBattleUnit(LastAutoBattleId, LastAutoUnitId, Error)) { Status = Error; return; }
    }
    const int32 Acting = Battle->SelectNextActingUnitIndex();
    if (!Battle->GetState().Units.IsValidIndex(Acting) || Battle->GetState().Units[Acting].UnitEntityId != BoundUnitId)
    { ReadyAtSeconds = -1.0; return; }
    const int64 Sequence = Battle->GetLog().GetNextSequence();
    if (FailedSequence == Sequence && FailedAbilityRevision==Battle->GetNativeAbilityRevision()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (ObservedSequence != Sequence || ReadyAtSeconds < 0.0)
    { ObservedSequence = Sequence; ReadyAtSeconds = Now + ActivePolicy->DecisionDelaySeconds; }
    if (Now < ReadyAtSeconds) return;
    FString Error;
    StepOwnTurn(Error);
}

void UOGNativeTurnAgentComponent::HandleRuntimeReleasing()
{
    ReleaseBattleUnit();
}

void UOGNativeTurnAgentComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Core.IsValid()) Core->OnCanonicalRuntimeReleasing.RemoveAll(this);
    ReleaseBattleUnit();
    Super::EndPlay(Reason);
}
