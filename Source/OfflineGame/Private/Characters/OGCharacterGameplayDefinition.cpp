#include "Characters/OGCharacterGameplayDefinition.h"
#include "Combat/OGNativeTurnAgentComponent.h"
#include "Combat/OGCombatResourceDefinition.h"
#include "Combat/OGNativeCombatAbility.h"
UOGCharacterGameplayDefinition::UOGCharacterGameplayDefinition() { System=TEXT("character"); }
bool UOGCharacterGameplayDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("character") || !IdentityId.IsValid() || Skills.CurrentForm!=DefinitionId ||
        Skills.HasDuplicateSkills() || OriginalSourceReference.TrimStartAndEnd().IsEmpty() ||
        IdentityId.Value.StartsWith(TEXT("diagnostic:")) || DefinitionId.Value.StartsWith(TEXT("diagnostic:")))
    { Error=TEXT("A character kit requires exact sourced Identity/Version and distinct skills."); return false; }
    TArray<FOGContentId> All=Skills.ActiveSkills; All.Append(Skills.PassiveSkills);
    if (!Skills.UltimateSkill.IsEmpty()) All.Add(Skills.UltimateSkill);
    for (const auto& Skill:All)
        if (!Skill.IsValid() || Skill.Value.StartsWith(TEXT("diagnostic:")))
        { Error=TEXT("Invalid skill in shared character definition."); return false; }
    if (!TurnPolicy.Id.IsEmpty() && (TurnPolicy.System!=TEXT("agent") || !TurnPolicy.Id.IsValid() || TurnPolicy.MinimumVersion<1))
    { Error=TEXT("Character turn policy requires a valid shared agent reference."); return false; }
    TSet<FOGContentId> Resources;
    for (const auto& Ref:BattleResources)
    {
        if (Ref.System!=TEXT("combat_resource") || !Ref.Id.IsValid() || Ref.MinimumVersion<1 || Resources.Contains(Ref.Id))
        { Error=TEXT("Character resources must uniquely reference shared combat resource definitions."); return false; }
        Resources.Add(Ref.Id);
    }
    return true;
}
const UOGCharacterGameplayDefinition* UOGCharacterGameplayDefinition::Find(
    const FOGContentId& Identity,const FOGContentId& Version,FString& Error)
{
    auto* Library=UOGDefinitionLibrary::Get();
    const auto* Definition=Library?Cast<UOGCharacterGameplayDefinition>(Library->Resolve(TEXT("character"),Version)):nullptr;
    if (!Definition || Definition->IdentityId!=Identity || !Definition->ValidateDefinition(Error))
    { if (Error.IsEmpty()) Error=TEXT("No exact shared character kit is installed for this Identity/Version."); return nullptr; }
    return Definition;
}
bool UOGCharacterGameplayDefinition::MatchesKit(const FOGResolvedSkillSet& Kit) const
{
    if (Kit.CurrentForm!=Skills.CurrentForm || Kit.ActiveSkills.Num()!=Skills.ActiveSkills.Num() ||
        Kit.PassiveSkills.Num()!=Skills.PassiveSkills.Num() || Kit.UltimateSkill!=Skills.UltimateSkill ||
        Kit.HasDuplicateSkills()) return false;
    for (const auto& Skill:Skills.ActiveSkills) if (!Kit.ActiveSkills.Contains(Skill)) return false;
    for (const auto& Skill:Skills.PassiveSkills) if (!Kit.PassiveSkills.Contains(Skill)) return false;
    return true;
}

void UOGCharacterGameplayDefinition::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{
    Super::CollectRequiredDefinitions(Out); Out.Append(BattleResources);
    if (TurnPolicy.Id.IsValid()) Out.Add(TurnPolicy);
    // Existing specialized canonical kits keep their native executors.
    if (IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"))) return;
    TArray<FOGContentId> Commands=Skills.ActiveSkills;
    if (Skills.UltimateSkill.IsValid()) Commands.Add(Skills.UltimateSkill);
    for (const auto& Skill:Commands) { FOGDefinitionReference R; R.System=TEXT("ability"); R.Id=Skill; Out.Add(R); }
}
bool UOGCharacterGameplayDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    if (TurnPolicy.Id.IsValid())
    {
        const auto* Policy=Cast<UOGNativeTurnPolicy>(Resolve(TurnPolicy.System,TurnPolicy.Id));
        if (!Policy || !Policy->MatchesSource(IdentityId,DefinitionId))
        { Error=TEXT("Character turn policy does not accept this source kit."); return false; }
        for (const auto& Rule:Policy->Rules)
            if (!Skills.ActiveSkills.Contains(Rule.SkillId) && Skills.UltimateSkill!=Rule.SkillId)
            { Error=TEXT("Shared AI policy references an ability outside this character's actual kit."); return false; }
    }
    TSet<FOGContentId> ResourceIds;
    for (const auto& Ref:BattleResources)
    {
        if (!Cast<UOGCombatResourceDefinition>(Resolve(Ref.System,Ref.Id)))
        { Error=TEXT("Character resource must resolve to the native resource definition class."); return false; }
        ResourceIds.Add(Ref.Id);
    }
    if (IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"))) return true;
    TArray<FOGContentId> Commands=Skills.ActiveSkills;
    if (Skills.UltimateSkill.IsValid()) Commands.Add(Skills.UltimateSkill);
    for (const auto& Skill:Commands)
    {
        const auto* Ability=Cast<UOGNativeCombatAbility>(Resolve(TEXT("ability"),Skill));
        if (!Ability || !Ability->Definition.MatchesSource(IdentityId,DefinitionId))
        { Error=TEXT("A character command must resolve to a compatible installed shared skill."); return false; }
        for (const auto* Rows:{&Ability->Definition.ResourceCosts,&Ability->Definition.ResourceChanges})
            for (const auto& Row:*Rows) if (!ResourceIds.Contains(Row.ResourceId))
            { Error=TEXT("A kit skill refers to a battle resource not declared by this character."); return false; }
    }
    return true;
}
