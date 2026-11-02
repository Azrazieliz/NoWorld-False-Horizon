// Gate00 director correction 2026-10-23:
// Field-wide authored abilities always affect EVERY eligible embodied
// active field unit. Only primary/center selection is player-directed;
// the player cannot reduce an AoE to a manually picked subset.
// This is a pure, read-only preparation step before the battle's atomic copy.
#include "Combat/OGTurnBattle.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

bool FOGTurnBattle::ExpandApprovedFieldTargets(
    const FOGResolvedCombatAction& Original,
    FOGResolvedCombatAction& Expanded, bool& bOutArea,
    FString& OutError) const
{
    Expanded=Original;
    bOutArea=false;
    OutError.Reset();
    bool Handled=false;
    if (!ExpandNativeAbilityTargets(Original,Expanded,Handled,bOutArea,OutError)) return false;
    if (Handled) return true;
    const FOGCombatUnitState* Source=State.Units.FindByPredicate(
        [&Original](const FOGCombatUnitState& Unit)
        { return Unit.UnitEntityId==Original.SourceUnitId; });
    if (!Source || !Source->CanAct())
        return true; // Normal action validator retains its own error.

    const FOGContentId Luminous(TEXT("character:luminous_valentine.identity"));
    if (Source->IdentityId!=Luminous)
        return true; // Other skills stay owned by their established executor.
    const auto& Skills=Source->SkillSet;
    const bool bBase=Skills.ActiveSkills.Contains(
        FOGContentId(TEXT("combat:luminous_valentine.base.skill.scarlet_cadence")));
    const bool bAlter=Skills.ActiveSkills.Contains(
        FOGContentId(TEXT("combat:luminous_valentine.alter.active_1")));
    if (bBase==bAlter || Skills.ActiveSkills.Num()!=3)
        return true;
    const bool bA2=Original.SkillId==Skills.ActiveSkills[1];
    const bool bA3=Original.SkillId==Skills.ActiveSkills[2];
    const bool bUlt=Original.SkillId==Skills.UltimateSkill;
    if (!bA2 && !bA3 && !bUlt) return true;

    FString Branch,Mode;
    if (bUlt || (bBase && bA3))
    {
        TSharedPtr<FJsonObject> Context;
        if (!FJsonSerializer::Deserialize(
                TJsonReaderFactory<>::Create(Original.ResolutionJson),
                Context) || !Context.IsValid())
            return true; // Canonical preflight rejects invalid branch context.
        Context->TryGetStringField(TEXT("branch"),Branch);
        Context->TryGetStringField(TEXT("mode"),Mode);
    }
    // All-in-field applies to authentic AoE branches, never to:
    // Base A3 covenant, Base Day revival preparation, Alter self A3,
    // or Alter Harvest's single-target execution.
    const bool bAllies=bBase &&
        ((bA3 && Mode!=TEXT("covenant")) ||
         (bUlt && Branch==TEXT("day") && Mode!=TEXT("revival_prepare")));
    const bool bEnemies=bA2 ||
        (bBase && bUlt && Branch==TEXT("night")) ||
        (bAlter && bUlt && Branch==TEXT("ruin"));
    if (!bAllies && !bEnemies) return true;
    bOutArea=true;

    // If a primary is supplied, retain its intrinsic primary/bonus damage
    // role, but NEVER discard the remaining active opponents or allies.
    const FOGEntityId Primary=Original.TargetUnitIds.IsEmpty()
        ?FOGEntityId():Original.TargetUnitIds[0];
    TArray<FOGEntityId> All;
    All.Reserve(State.Units.Num());
    for (const FOGCombatUnitState& Target:State.Units)
    {
        if (!Target.IsLivingFieldTarget()) continue; // Reserve/off-field excluded; inability to act is not immunity.
        if (bAllies != (Target.TeamIndex==Source->TeamIndex)) continue;
        All.Add(Target.UnitEntityId);
    }
    if (All.IsEmpty())
    {
        OutError=TEXT("No eligible active field participants for this area skill.");
        return false;
    }
    if (All.Contains(Primary))
    {
        All.Remove(Primary);
        All.Insert(Primary,0);
    }
    Expanded.TargetUnitIds=MoveTemp(All);
    return true;
}
