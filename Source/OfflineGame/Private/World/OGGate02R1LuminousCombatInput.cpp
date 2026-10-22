// Native first-World active owned Luminous battle input.
// Reuses real GameCore / WorldStore / FOGTurnBattle. Does not spawn
// fake enemies, invent Manifestations or bypass selected player's version.
#include "World/OGStartingRegionPresentation.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
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
    const FOGCombatUnitState* Source=nullptr;
    for(const FOGCombatUnitState& Unit:Battle.Units)
        if(Unit.UnitEntityId==Manifestation){Source=&Unit;break;}
    if(!Source || !Source->CanAct() ||
       Source->SkillSet.CurrentForm!=ApprovedSkills.CurrentForm)
    {
        OutError=TEXT("The owned Luminous unit is not a legal active battle participant.");
        return false;
    }
    if(!bHasTargetHudProjection || !TargetHudProjection.TargetEntityId.IsValid())
    {
        OutError=TEXT("A real target must be selected for the authored Luminous skill.");
        return false;
    }
    bool bTargetInEncounter=false;
    for(const FOGCombatUnitState& Target:Battle.Units)
        if(Target.UnitEntityId==TargetHudProjection.TargetEntityId)
        {
            bTargetInEncounter=true;
            break;
        }
    if(!bTargetInEncounter)
    {
        OutError=TEXT("HUD target does not belong to the active native encounter.");
        return false;
    }
    FOGResolvedCombatAction Action;
    Action.ActionId=FOGEntityId::NewId();
    Action.SourceUnitId=Manifestation;
    Action.SkillId=Skill;
    Action.TargetUnitIds.Add(TargetHudProjection.TargetEntityId);
    Action.ResolutionJson=Resolution;
    Action.ActionDelay=FMath::Max<int64>(1,Source->DefaultActionDelay);
    Action.InterruptPriority=0;
    // Preserve the authoritative executor as the only source of gameplay truth.
    // Presentation is attempted only after the action has been accepted.
    const bool bApplied = Core->ApplyApprovedLuminousResolvedAction(Action, OutError);
    if (bApplied && ActiveLuminous)
    {
        const FName Semantic = Slot == -1
            ? FName(TEXT("Combat.Ultimate")) : FName(TEXT("Combat.Skill"));
        if (!ActiveLuminous->PlaySharedGameplayAction(Semantic))
            UE_LOG(LogTemp, Warning,
                TEXT("Gate12 combat accepted but no approved exact-rig native clip for %s; not substituting unrelated motion"),
                *Semantic.ToString());
    }
    return bApplied;
}
