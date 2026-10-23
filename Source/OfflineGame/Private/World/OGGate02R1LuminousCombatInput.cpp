// Native first-World active owned Luminous battle input.
// Reuses real GameCore / WorldStore / FOGTurnBattle. Does not spawn
// fake enemies, invent Manifestations or bypass selected player's version.
#include "World/OGStartingRegionPresentation.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Combat/OGCanonicalCombatActorBindingComponent.h"
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

bool AOGWorldPrototypeCharacter::HasNativeOwnedLuminousEncounter() const
{
    UGameInstance* GI=GetGameInstance();
    const UOGGameCoreSubsystem* Core=GI?
        GI->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    const FOGTurnBattle* Battle=Core?
        Core->GetOwnedLuminousTurnBattle():nullptr;
    return Battle && Battle->GetState().Status==EOGTurnBattleStatus::Running;
}

bool AOGWorldPrototypeCharacter::SubmitApprovedLuminousWorldAction(
    int32 Slot,FName Branch,FString& OutError)
{
    OutError.Reset();
    if(!GetWorld() || !GetWorld()->IsGameWorld() ||
       !IsFoundationWorldInputAllowed() || !IsLocallyControlled())
    {
        OutError=TEXT("Native Luminous World input is not active.");
        return false;
    }
    UGameInstance* GI=GetGameInstance();
    UOGGameCoreSubsystem* Core=GI?
        GI->GetSubsystem<UOGGameCoreSubsystem>():nullptr;
    UOGGate12RulerSessionSubsystem* Session=GI?
        GI->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    if(!Core || !Session || !Session->IsBoundToStoredRuler() ||
       !HasNativeOwnedLuminousEncounter())
    {
        OutError=TEXT("No real locally owned Luminous encounter.");
        return false;
    }
    const FOGEntityId Manifestation=Session->SelectedManifestationId;
    const FOGEntityId Ruler=Session->BoundRulerId;
    FOGResolvedSkillSet ApprovedSkills;
    if(!Core->ReadOwnedApprovedLuminousSkills(
         Manifestation,Ruler,ApprovedSkills,OutError))
        return false;
    AOGLuminousV3PlayableCharacter* ActiveLuminous=
        Cast<AOGLuminousV3PlayableCharacter>(this);
    if(!ActiveLuminous ||
       !ActiveLuminous->IsBoundToOwnedLuminousCombatUnit(
           Ruler,Manifestation,ApprovedSkills.CurrentForm))
    {
        OutError=TEXT("Selected Manifestation is not the actually possessed Luminous World character.");
        return false;
    }
    if(Slot < -1 || Slot>2)
    {
        OutError=TEXT("Only the three authored actives or explicit Ultimate exist.");
        return false;
    }
    FOGContentId Skill;
    if(Slot>=0)
        Skill=ApprovedSkills.ActiveSkills[Slot];
    else
        Skill=ApprovedSkills.UltimateSkill;
    if(!Skill.IsValid())
    {
        OutError=TEXT("Exact approved active/ultimate skill ID is missing.");
        return false;
    }
    FString Resolution=TEXT("{}");
    if(Slot==-1)
    {
        const FString Choice=Branch.ToString().ToLower();
        const bool bBase=ApprovedSkills.CurrentForm==
            FOGContentId(TEXT("character:luminous_valentine.version_ln_base"));
        if((bBase && Choice!=TEXT("day") && Choice!=TEXT("night")) ||
           (!bBase && Choice!=TEXT("ruin") && Choice!=TEXT("harvest")))
        {
            OutError=TEXT("Choose the approved Day/Night or Ruin/Harvest ultimate branch explicitly.");
            return false;
        }
        Resolution=FString::Printf(TEXT("{\"branch\":\"%s\"}"),*Choice);
    }
    const FOGTurnBattle* Encounter=Core->GetOwnedLuminousTurnBattle();
    if(!Encounter || Encounter->GetState().Status!=EOGTurnBattleStatus::Running)
    {
        OutError=TEXT("Live encounter ended before the action was submitted.");
        return false;
    }
    const FOGTurnBattleState& Battle=Encounter->GetState();
    const int32 ActingIndex=Encounter->SelectNextActingUnitIndex();
    if(!Battle.Units.IsValidIndex(ActingIndex) ||
       Battle.Units[ActingIndex].UnitEntityId!=Manifestation)
    {
        OutError=TEXT("Only the canonical next acting possessed Manifestation may submit a turn.");
        return false;
    }
    const FOGCombatUnitState* Source=nullptr;
    for(const FOGCombatUnitState& Unit:Battle.Units)
        if(Unit.UnitEntityId==Manifestation){Source=&Unit;break;}
    if(!Source || !Source->CanAct() ||
       Source->SkillSet.CurrentForm!=ApprovedSkills.CurrentForm)
    {
        OutError=TEXT("The owned Luminous unit is not a legal active battle participant.");
        return false;
    }
    TArray<FOGEntityId> Chosen;
    if(SelectedCanonicalBattleId==Battle.BattleId &&
       !SelectedCanonicalTurnTargets.IsEmpty())
        Chosen=SelectedCanonicalTurnTargets;
    else if(bHasTargetHudProjection && TargetHudProjection.TargetEntityId.IsValid())
        Chosen.Add(TargetHudProjection.TargetEntityId);
    // The central executor, not the HUD, classifies an AoE and expands it
    // to EVERY eligible on-field enemy/ally (with no 3/4 target ceiling).
    // Optional manual focus is only the primary strike's bonus recipient.
    FOGResolvedCombatAction Proposed;
    Proposed.SourceUnitId=Manifestation;
    Proposed.SkillId=Skill;
    Proposed.TargetUnitIds=Chosen;
    Proposed.ResolutionJson=Resolution;
    FOGResolvedCombatAction Expanded;
    bool bArea=false;
    if (!Encounter->ExpandApprovedFieldTargets(Proposed,Expanded,bArea,OutError))
        return false;
    Chosen=Expanded.TargetUnitIds;
    if(Chosen.IsEmpty())
    {
        OutError=TEXT("Select a legal target for single-target actions; area skills use all eligible active participants.");
        return false;
    }
    // No presentation-side target count/allegiance restrictions: the exact
    // approved Gate04 preflight still validates every native gameplay action.
    TMap<FOGEntityId,FOGLargeNumber> HpBefore;
    for(const FOGEntityId& Selected:Chosen)
    {
        const FOGCombatUnitState* Actual=Battle.Units.FindByPredicate(
            [&Selected](const FOGCombatUnitState& U)
            { return U.UnitEntityId==Selected; });
        if(!Actual || !Actual->CanAct() || HpBefore.Contains(Selected))
        {
            OutError=TEXT("An actual active, nonduplicated battle target is required.");
            return false;
        }
        HpBefore.Add(Selected,Actual->CurrentHp);
    }
    FOGResolvedCombatAction Action;
    Action.ActionId=FOGEntityId::NewId();
    Action.SourceUnitId=Manifestation;
    Action.SkillId=Skill;
    Action.TargetUnitIds=Chosen;
    Action.ResolutionJson=Resolution;
    Action.ActionDelay=FMath::Max<int64>(1,Source->DefaultActionDelay);
    Action.InterruptPriority=0;
    // Preserve the authoritative executor as the only source of gameplay truth.
    // Presentation is attempted only after the action has been accepted.
    const bool bApplied = Core->ApplyApprovedLuminousResolvedAction(Action, OutError);
    if (bApplied && ActiveLuminous)
    {
        ClearSelectedCanonicalTurnTargets();
        // Actual actor facing and reactions are presentation of a committed
        // native action. The selected physical target must be a source-backed
        // embodied participant, never an inferred temporary mesh or proxy.
        if (const FOGTurnBattle* Updated=Core->GetOwnedLuminousTurnBattle())
        {
            AActor* FirstActualTarget=nullptr;
            for(const FOGEntityId& TargetId:Chosen)
            {
                AActor* TargetActor=
                    UOGCanonicalCombatActorBindingComponent::FindActualWorldActor(
                        GetWorld(),Battle.BattleId,TargetId);
                if(!TargetActor) continue;
                if(!FirstActualTarget) FirstActualTarget=TargetActor;
                const FOGCombatUnitState* After=Updated->GetState().Units.FindByPredicate(
                    [&TargetId](const FOGCombatUnitState& U)
                    { return U.UnitEntityId==TargetId; });
                const FOGLargeNumber* Before=HpBefore.Find(TargetId);
                if(!After || !Before) continue;
                const int32 Change=FOGLargeNumber::Compare(After->CurrentHp,*Before);
                if(Change!=0)
                    if(UOGSharedMotionControllerComponent* Motion=
                        TargetActor->FindComponentByClass<UOGSharedMotionControllerComponent>())
                    {
                        const FName Reaction=After->IsAlive()
                            ? (Change<0 ? FName(TEXT("Combat.Reaction.Hit")) :
                               FName(TEXT("Combat.Reaction.Heal")))
                            : FName(TEXT("Combat.Reaction.Death"));
                        // No registered exact-rig clip = NO fake reaction.
                        Motion->PlayAction(Reaction,TEXT("Combat"),false,1);
                    }
            }
            if(FirstActualTarget && FirstActualTarget!=ActiveLuminous)
            {
                const FVector Delta=FirstActualTarget->GetActorLocation()-GetActorLocation();
                if(Delta.SizeSquared2D()>FMath::Square(5.f))
                    SetActorRotation(FRotator(0.f,Delta.Rotation().Yaw,0.f));
            }
        }
        // Central semantic keys identify the ACTUAL approved skill and
        // Ultimate branch, not one generic Luminous pose for every Version.
        // Content artists register exact-rig derivatives globally under the
        // same action. Missing approved combat clips never trigger a fake pose.
        const FString MotionKey = Slot == -1
            ? FString::Printf(TEXT("Combat.Ultimate.%s.%s"),
                *Skill.ToString(),*Branch.ToString().ToLower())
            : FString::Printf(TEXT("Combat.Skill.%s"),*Skill.ToString());
        const FName Semantic(*MotionKey);
        if (!ActiveLuminous->PlaySharedGameplayAction(Semantic))
            UE_LOG(LogTemp, Warning,
                TEXT("Gate12 combat accepted but no approved exact-rig native clip for %s; not substituting unrelated motion"),
                *Semantic.ToString());
    }
    return bApplied;
}
