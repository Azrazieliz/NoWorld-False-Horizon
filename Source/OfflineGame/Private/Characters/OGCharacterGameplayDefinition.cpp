#include "Combat/OGNativeCombatAbility.h"
#include "Characters/OGCharacterGameplayDefinition.h"
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
    Super::CollectRequiredDefinitions(Out);
    // Existing specialized canonical kits keep their native executors.
    if (IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"))) return;
    TArray<FOGContentId> Commands=Skills.ActiveSkills;
    if (Skills.UltimateSkill.IsValid()) Commands.Add(Skills.UltimateSkill);
    for (const auto& Skill:Commands) { FOGDefinitionReference R; R.System=TEXT("ability"); R.Id=Skill; Out.Add(R); }
}
bool UOGCharacterGameplayDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    if (IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"))) return true;
    TArray<FOGContentId> Commands=Skills.ActiveSkills;
    if (Skills.UltimateSkill.IsValid()) Commands.Add(Skills.UltimateSkill);
    for (const auto& Skill:Commands)
    {
        const auto* Ability=Cast<UOGNativeCombatAbility>(Resolve(TEXT("ability"),Skill));
        if (!Ability || !Ability->Definition.MatchesSource(IdentityId,DefinitionId))
        { Error=TEXT("A character command must resolve to a compatible installed shared skill."); return false; }
    }
    return true;
}
