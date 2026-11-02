#include "Skills/OGResolvedSkillSet.h"

bool FOGResolvedSkillSet::HasDuplicateSkills() const
{
    TSet<FOGContentId> Seen;

    auto AddUnique = [&Seen](const FOGContentId& SkillId)
    {
        if (SkillId.IsEmpty())
        {
            return true;
        }

        if (Seen.Contains(SkillId))
        {
            return false;
        }

        Seen.Add(SkillId);
        return true;
    };

    for (const FOGContentId& SkillId : ActiveSkills)
    {
        if (!AddUnique(SkillId))
        {
            return true;
        }
    }

    for (const FOGContentId& SkillId : PassiveSkills)
    {
        if (!AddUnique(SkillId))
        {
            return true;
        }
    }

    if (!UltimateSkill.IsEmpty() &&
        (!UltimateSkill.IsValid() || !AddUnique(UltimateSkill)))
    {
        return true;
    }

    if (!UltimateSkills.IsEmpty() && !UltimateSkill.IsEmpty() && UltimateSkills[0]!=UltimateSkill) return true;
    TSet<FOGContentId> Ultimates;
    for (const auto& Id:UltimateSkills)
    {
        if (!Id.IsValid() || Ultimates.Contains(Id)) return true;
        Ultimates.Add(Id);
        // The backwards-compatible primary alias is the same ability, never a duplicate grant.
        if (Id==UltimateSkill) continue;
        if (!AddUnique(Id)) return true;
    }
    return false;
}

TArray<FOGContentId> FOGResolvedSkillSet::GetUltimateSkills() const
{
    if (!UltimateSkills.IsEmpty()) return UltimateSkills;
    TArray<FOGContentId> Result;
    if (UltimateSkill.IsValid()) Result.Add(UltimateSkill);
    return Result;
}
FOGContentId FOGResolvedSkillSet::GetPrimaryUltimateSkill() const
{ return UltimateSkill.IsValid()?UltimateSkill:(UltimateSkills.IsEmpty()?FOGContentId():UltimateSkills[0]); }
bool FOGResolvedSkillSet::IsUltimate(const FOGContentId& Skill) const
{ return Skill.IsValid() && (UltimateSkills.IsEmpty()?UltimateSkill==Skill:UltimateSkills.Contains(Skill)); }
bool FOGResolvedSkillSet::HasSameUltimates(const FOGResolvedSkillSet& Other) const
{
    if (HasDuplicateSkills() || Other.HasDuplicateSkills()) return false;
    const auto A=GetUltimateSkills(),B=Other.GetUltimateSkills();
    return A==B;
}
