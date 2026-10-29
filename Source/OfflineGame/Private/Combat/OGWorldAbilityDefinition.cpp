#include "Combat/OGWorldAbilityDefinition.h"
#include "Engine/StaticMesh.h"

bool FOGWorldAbilitySpecification::Matches(const FOGCombatUnitState& Unit) const
{ return (IdentityId.IsEmpty() || IdentityId==Unit.IdentityId) && (VersionId.IsEmpty() || VersionId==Unit.SkillSet.CurrentForm); }
bool FOGWorldAbilitySpecification::Validate(FString& Error) const
{
    Error.Reset();
    if (!SkillId.IsValid() || (!IdentityId.IsEmpty() && !IdentityId.IsValid()) || (!VersionId.IsEmpty() && !VersionId.IsValid()) ||
        OriginalSourceReference.TrimStartAndEnd().IsEmpty() || CostTiming==EOGWorldAbilityCostTiming::Unspecified ||
        static_cast<uint8>(CostTiming)>static_cast<uint8>(EOGWorldAbilityCostTiming::FirstContact) ||
        DurationMilliseconds<1 || CooldownMilliseconds<0 || Steps.IsEmpty() || Steps.Num()>4096)
    { Error=TEXT("World execution requires exact sourced timing, cost commitment and impact steps."); return false; }
    for (const auto& Window:TraversalAndCancel.CancelWindows)
        if (!FMath::IsFinite(Window.OpensAtSeconds) || !FMath::IsFinite(Window.ClosesAtSeconds) ||
            Window.OpensAtSeconds<0 || Window.ClosesAtSeconds<Window.OpensAtSeconds ||
            Window.ClosesAtSeconds>DurationMilliseconds/1000.0)
        { Error=TEXT("World cancel windows must fall inside the authored action duration."); return false; }
    for (const auto& Step:Steps)
    {
        if (Step.OpensAtMilliseconds<0 || Step.ClosesAtMilliseconds<Step.OpensAtMilliseconds ||
            Step.ClosesAtMilliseconds>DurationMilliseconds ||
            static_cast<uint8>(Step.Delivery)>static_cast<uint8>(EOGWorldAbilityDelivery::Projectile) ||
            !FMath::IsFinite(Step.RangeCentimeters) || Step.RangeCentimeters<0 ||
            !FMath::IsFinite(Step.RadiusCentimeters) || Step.RadiusCentimeters<0 ||
            (Step.Delivery!=EOGWorldAbilityDelivery::Direct && Step.RadiusCentimeters<=0) ||
            ((Step.Delivery==EOGWorldAbilityDelivery::Sphere || Step.Delivery==EOGWorldAbilityDelivery::Projectile) && Step.RangeCentimeters<=0) ||
            (Step.Delivery==EOGWorldAbilityDelivery::MeleeSweep && (Step.StartSocket.IsNone() || Step.EndSocket.IsNone())) ||
            (Step.Delivery==EOGWorldAbilityDelivery::Projectile && (!FMath::IsFinite(Step.ProjectileSpeed) || Step.ProjectileSpeed<=0 ||
                !FMath::IsFinite(Step.ProjectileGravityScale) || Step.ProjectileMesh.IsNull() || !Step.ProjectileMesh.ToSoftObjectPath().GetAssetPathString().StartsWith(TEXT("/Game/")) || Step.ProjectileCollisionProfile.IsNone() || Step.ClosesAtMilliseconds==Step.OpensAtMilliseconds || !Step.ProjectileMeshTransform.IsValid())))
        { Error=TEXT("World impact delivery, interval, geometry or original projectile presentation is invalid."); return false; }
        for (const auto& Slice:Step.Packets)
            if (Slice.PacketIndex<0 || Slice.FirstRepeat<0 || Slice.RepeatCount<1 ||
                Slice.FirstRepeat>MAX_int32-Slice.RepeatCount)
            { Error=TEXT("World packet slices require bounded positive repeat intervals."); return false; }
    }
    return true;
}
bool FOGWorldAbilitySpecification::ValidatePackets(const FOGNativeAbilityDefinition& Ability,FString& Error) const
{
    if (!Validate(Error) || !Ability.Validate(Error) || SkillId!=Ability.SkillId)
    { if (Error.IsEmpty()) Error=TEXT("World timing does not match the actual shared ability."); return false; }
    for (const auto& Step:Steps)
        if (Ability.Targets!=EOGNativeAbilityTargets::Self && Step.Delivery==EOGWorldAbilityDelivery::Direct && Step.RangeCentimeters<=0)
        { Error=TEXT("Non-self direct delivery requires an explicit sourced range."); return false; }
    struct FInterval { int32 Start,End; };
    TMap<int32,TArray<FInterval>> Intervals;
    for (const auto& Step:Steps) for (const auto& Slice:Step.Packets)
    {
        if (!Ability.Packets.IsValidIndex(Slice.PacketIndex) ||
            Slice.FirstRepeat+Slice.RepeatCount>Ability.Packets[Slice.PacketIndex].Repeats)
        { Error=TEXT("World timing references a packet/repeat outside the original skill."); return false; }
        Intervals.FindOrAdd(Slice.PacketIndex).Add({Slice.FirstRepeat,Slice.FirstRepeat+Slice.RepeatCount});
    }
    for (int32 Index=0;Index<Ability.Packets.Num();++Index)
    {
        auto& Ranges=Intervals.FindOrAdd(Index); Ranges.Sort([](const auto& A,const auto& B){return A.Start<B.Start;});
        int32 Cursor=0;
        for (const auto& Range:Ranges)
        { if (Range.Start!=Cursor) { Error=TEXT("World steps must cover each original repeat exactly once, without overlap."); return false; } Cursor=Range.End; }
        if (Cursor!=Ability.Packets[Index].Repeats)
        { Error=TEXT("World steps omit original skill packet repeats."); return false; }
    }
    return true;
}
bool UOGWorldAbilityDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("world_ability") || DefinitionId!=Specification.SkillId)
    { Error=TEXT("World ability key must equal its actual shared skill ID."); return false; }
    return Specification.Validate(Error);
}
void UOGWorldAbilityDefinition::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{ Super::CollectRequiredDefinitions(Out); FOGDefinitionReference Ref; Ref.System=TEXT("ability"); Ref.Id=Specification.SkillId; Out.Add(Ref); }
bool UOGWorldAbilityDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    const auto* Ability=Cast<UOGNativeCombatAbility>(Resolve(TEXT("ability"),Specification.SkillId));
    if (!Ability) { Error=TEXT("World staging needs the existing native ability, including its original mechanics."); return false; }
    return Specification.ValidatePackets(Ability->Definition,Error);
}
