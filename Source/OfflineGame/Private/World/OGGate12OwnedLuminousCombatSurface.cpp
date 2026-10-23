// Actual owned Luminous turn controls OVER the native rendered First World.
// This is not the diagnostic black-screen arena and does not create an enemy,
// a phantom-owned Manifestation, an animation or a second gameplay executor.
#include "World/OGStartingRegionPresentation.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Combat/OGTurnBattle.h"
#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

bool AOGWorldPrototypeCharacter::SelectOwnedLuminousBattleTarget(
    int32 UnitIndex, FString& OutError)
{
    OutError.Reset();
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !IsLocallyControlled() ||
        !HasNativeOwnedLuminousEncounter())
    {
        OutError = TEXT("No real locally controlled owned battle is active.");
        return false;
    }
    UGameInstance* GI = GetGameInstance();
    const UOGGameCoreSubsystem* Core = GI
        ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    const FOGTurnBattle* Battle = Core ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!Battle || !Session || !Session->IsBoundToStoredRuler())
    {
        OutError = TEXT("Owned encounter/Registry session was not available.");
        return false;
    }
    const FOGTurnBattleState& State = Battle->GetState();
    const int32 ActingIndex = Battle->SelectNextActingUnitIndex();
    if (State.Status != EOGTurnBattleStatus::Running ||
        !State.Units.IsValidIndex(ActingIndex) ||
        !State.Units.IsValidIndex(UnitIndex))
    {
        OutError = TEXT("No legal actor or selected target exists in the live turn state.");
        return false;
    }
    const FOGCombatUnitState& Source = State.Units[ActingIndex];
    const FOGCombatUnitState& Target = State.Units[UnitIndex];
    const AOGLuminousV3PlayableCharacter* Luminous =
        Cast<AOGLuminousV3PlayableCharacter>(this);
    if (!Luminous || Source.UnitEntityId != Session->SelectedManifestationId ||
        !Luminous->IsBoundToOwnedLuminousCombatUnit(Session->BoundRulerId,
            Session->SelectedManifestationId, Source.SkillSet.CurrentForm) ||
        !Source.CanAct() || !Target.CanAct() ||
        Source.TeamIndex == Target.TeamIndex)
    {
        OutError = TEXT("Select a live opposing unit during the actual possessed Luminous turn.");
        return false;
    }
    // Keep the established knowledge/privacy projection for an already
    // observed target; NEVER reveal undiscovered HP/identity from battle math.
    FOGWorldTargetViewModel Projection;
    if (HasTargetHudProjectionForHud() &&
        GetTargetHudProjectionForHud().TargetEntityId == Target.UnitEntityId)
        Projection = GetTargetHudProjectionForHud();
    Projection.TargetEntityId = Target.UnitEntityId;
    Projection.bPrincipalTarget = true;
    SetTargetHudProjection(Projection);
    return true;
}

bool AOGWorldPresentationHud::DrawOwnedLuminousCombatSurface(
    AOGWorldPrototypeCharacter* Character, float Scale)
{
    if (!Canvas || !Character || !Character->HasNativeOwnedLuminousEncounter() ||
        !Cast<AOGLuminousV3PlayableCharacter>(Character))
        return false;
    UGameInstance* GI = GetGameInstance();
    const UOGGameCoreSubsystem* Core = GI
        ? GI->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
    const UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    const FOGTurnBattle* Battle = Core ? Core->GetOwnedLuminousTurnBattle() : nullptr;
    if (!Battle || !Session || !Session->IsBoundToStoredRuler())
        return false;
    const FOGTurnBattleState& State = Battle->GetState();
    if (State.Status != EOGTurnBattleStatus::Running)
        return false;
    const int32 ActingIndex = Battle->SelectNextActingUnitIndex();
    if (!State.Units.IsValidIndex(ActingIndex))
        return false;

    const FOGCombatUnitState& Acting = State.Units[ActingIndex];
    const FOGCombatUnitState* Owned = State.Units.FindByPredicate(
        [Session](const FOGCombatUnitState& U)
        { return U.UnitEntityId == Session->SelectedManifestationId; });
    const AOGLuminousV3PlayableCharacter* Luminous =
        CastChecked<AOGLuminousV3PlayableCharacter>(Character);
    if (!Owned || !Luminous->IsBoundToOwnedLuminousCombatUnit(
        Session->BoundRulerId,Session->SelectedManifestationId,
        Owned->SkillSet.CurrentForm)) return false;
    const bool bOwnTurn =
        Acting.UnitEntityId == Owned->UnitEntityId && Acting.CanAct();

    const float W = Canvas->ClipX, H = Canvas->ClipY;
    const float S = FMath::Clamp(FMath::Min(W/1280.f,H/720.f), .52f, 1.30f);
    const float Edge = 14.f*S;
    const float HeaderHeight = 49.f*S;
    const FLinearColor Pale(.89f,.86f,.91f,1.f);
    const FLinearColor Accent(.79f,.65f,.87f,1.f);
    const FLinearColor Dim(.49f,.48f,.55f,1.f);
    const FLinearColor Panel(.018f,.014f,.031f,.88f);

    // Keep the REAL rendered level and actual native actors visible between
    // bands. Never DrawGameBackdrop or DrawFoundationCharacterStandIn here.
    DrawGamePanel(Panel,Edge,Edge,W-2.f*Edge,HeaderHeight);
    DrawGameText(FString::Printf(
        TEXT("LIVE ENCOUNTER  |  ACTION VALUE %lld  |  %s"),
        static_cast<long long>(State.CurrentActionValue),
        bOwnTurn ? TEXT("YOUR TURN") : TEXT("OTHER COMBATANT'S TURN")),
        Edge+16.f*S,Edge+10.f*S,22.f*S,Pale,W*.64f);
    DrawGameText(FString::Printf(TEXT("OWN HP %s / %s"),
        *Owned->CurrentHp.ToDebugString(),*Owned->Stats.MaxHp.ToDebugString()),
        W*.68f,Edge+13.f*S,17.f*S,Accent,W*.29f);

    // Actual battle opponents, never synthetic units: each selectable card
    // refers to a live unit INDEX in the canonical state, not an actor name.
    const float CardW = FMath::Min(166.f*S,(W-2.f*Edge-16.f*S)/6.f);
    const float CardH = 64.f*S;
    const float Top = Edge+HeaderHeight+7.f*S;
    int32 EnemyOrdinal=0;
    for (int32 I=0; I<State.Units.Num(); ++I)
    {
        const FOGCombatUnitState& Unit = State.Units[I];
        if (Unit.TeamIndex == Owned->TeamIndex || EnemyOrdinal>=6) continue;
        const float X = Edge+EnemyOrdinal*(CardW+5.f*S);
        const bool bSelected = Character->HasTargetHudProjectionForHud() &&
            Character->GetTargetHudProjectionForHud().TargetEntityId==Unit.UnitEntityId;
        const FLinearColor Border = bSelected ? Accent : Dim;
        DrawGamePanel(Panel,X,Top,CardW,CardH);
        DrawLine(X,Top,X+CardW,Top,Border,bSelected?2.5f:1.f);
        DrawGameText(FString::Printf(TEXT("Opponent %d"),EnemyOrdinal+1),
            X+7.f*S,Top+7.f*S,17.f*S,Pale,CardW-14.f*S);
        DrawGameText(Unit.CanAct() ?
            (bSelected ? TEXT("TARGET SELECTED") : TEXT("SELECT TARGET")) :
            TEXT("DEFEATED"),X+7.f*S,Top+35.f*S,12.f*S,
            Unit.CanAct()?Border:Dim,CardW-14.f*S);
        if (bOwnTurn && Unit.CanAct())
        {
            AddFoundationHitBox(FVector2D(X,Top),FVector2D(CardW,CardH),
                FName(*FString::Printf(TEXT("OG.Battle.Target.%d"),I)),
                true,162);
        }
        ++EnemyOrdinal;
    }

    // Buttons map to the real approved ActiveSkill slots, not diagnostic
    // Basic/Skill1/Skill2 placeholders. Ultimate selection is always explicit.
    const float ButtonW = FMath::Min(140.f*S,(W-2.f*Edge)/5.5f);
    const float ButtonH = 54.f*S;
    const float Bottom = H-ButtonH-Edge;
    for (int32 Slot=0; Slot<3; ++Slot)
    {
        const FString Label = FString::Printf(TEXT("ACTIVE %d"),Slot+1);
        DrawGameButton(Label,
            FName(*FString::Printf(TEXT("OG.Battle.A%d"),Slot+1)),
            Edge+Slot*(ButtonW+8.f*S),Bottom,ButtonW,ButtonH,
            Slot==0,bOwnTurn && Owned->SkillSet.ActiveSkills.IsValidIndex(Slot) &&
            Owned->SkillSet.ActiveSkills[Slot].IsValid(),158);
    }
    const bool bBase = Owned->SkillSet.CurrentForm ==
        FOGContentId(TEXT("character:luminous_valentine.version_ln_base"));
    const FName LeftId = bBase ? TEXT("OG.Battle.Ultimate.Day") :
        TEXT("OG.Battle.Ultimate.Ruin");
    const FName RightId = bBase ? TEXT("OG.Battle.Ultimate.Night") :
        TEXT("OG.Battle.Ultimate.Harvest");
    DrawGameButton(bBase ? TEXT("DAY") : TEXT("RUIN"),LeftId,
        Edge+3*(ButtonW+8.f*S),Bottom,ButtonW,ButtonH,
        false,bOwnTurn && Owned->SkillSet.UltimateSkill.IsValid(),158);
    DrawGameButton(bBase ? TEXT("NIGHT") : TEXT("HOLLOW CROWN"),RightId,
        Edge+4*(ButtonW+8.f*S),Bottom,ButtonW,ButtonH,
        false,bOwnTurn && Owned->SkillSet.UltimateSkill.IsValid(),158);

    // Error messages are actual canonical preflight errors, not invented
    // animation/gameplay consequences.
    if (!FoundationStatusMessage.IsEmpty())
        DrawGameText(FoundationStatusMessage,Edge+5.f*S,
            Bottom-35.f*S,18.f*S,Accent,W-2.f*Edge);
    return true;
}
