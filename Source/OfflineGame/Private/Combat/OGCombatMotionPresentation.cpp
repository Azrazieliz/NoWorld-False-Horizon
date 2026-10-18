#include "Combat/OGCombatMotionPresentation.h"

#include "Runtime/OGFoundationCharacterRuntime.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FName FOGCombatMotionPresentation::ParseProfile(const FString& ExistingEvolutionStateJson)
{
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(
            TJsonReaderFactory<>::Create(ExistingEvolutionStateJson), Object) ||
        !Object.IsValid())
        return NAME_None;
    FString Profile;
    if (!Object->TryGetStringField(TEXT("combat_motion_profile"), Profile) ||
        Profile.IsEmpty() || Profile.Len() > 64)
        return NAME_None;
    // Authoring keys, not weapons inferred from the definition's display name.
    // Avoid paths, separators and accidental asset lookups from untrusted JSON.
    for (TCHAR Char : Profile)
    {
        if (!FChar::IsAlnum(Char) && Char != TEXT('_') && Char != TEXT('-'))
            return NAME_None;
    }
    return FName(*Profile.ToLower());
}

FName FOGCombatMotionPresentation::ResolveAuthoredProfile(
    const FOGFoundationCharacterProjection& Projection)
{
    FName Result;
    for (const FOGFoundationEquippedItemProjection& Equipped : Projection.Equipment)
    {
        if (Equipped.bBroken) continue;
        const FName Candidate=ParseProfile(Equipped.Item.EvolutionStateJson);
        if (Candidate.IsNone()) continue;
        if (!Result.IsNone() && Candidate != Result)
        {
            // Multiweapon/natural anatomy requires authored combined actions.
            // Never pick "rifle" or a random handedness from conflicting items.
            return NAME_None;
        }
        Result=Candidate;
    }
    return Result;
}

FName FOGCombatMotionPresentation::ActionClipId(FName Profile, int32 Chain, bool bBasic)
{
    if (Profile.IsNone()) return NAME_None;
    const FString Name=Profile.ToString();
    if (Name.Len() > 64) return NAME_None;
    const int32 Step=FMath::Clamp(Chain, 0, 2) + 1;
    // Unreal format literals are compile-time checked: never use a runtime
    // conditional as FString::Printf's format argument.
    return FName(*FString::Printf(TEXT("Combat%s.%s.%d"),
        bBasic ? TEXT("Attack") : TEXT("Skill"), *Name, Step));
}
