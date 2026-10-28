#include "Combat/OGNativeCombatAbility.h"

bool FOGNativeAbilityPacket::operator==(const FOGNativeAbilityPacket& V) const
{
    return Effect==V.Effect && Scaling==V.Scaling && FlatAmount==V.FlatAmount && DefenseReference==V.DefenseReference &&
        DamageType==V.DamageType && Repeats==V.Repeats && bCanCrit==V.bCanCrit &&
        bCanBeBlocked==V.bCanBeBlocked && bAllowHitOverflow==V.bAllowHitOverflow;
}
bool FOGNativeAbilityDefinition::operator==(const FOGNativeAbilityDefinition& V) const
{
    return IdentityId==V.IdentityId && VersionId==V.VersionId && SkillId==V.SkillId &&
        OriginalSourceReference==V.OriginalSourceReference && MotionSemantic==V.MotionSemantic &&
        bNoResourceCost==V.bNoResourceCost && ResourceCosts==V.ResourceCosts && ResourceChanges==V.ResourceChanges && CooldownOwnerActions==V.CooldownOwnerActions && Targets==V.Targets &&
        bIncludeSelf==V.bIncludeSelf && Packets==V.Packets && Effects==V.Effects;
}
bool FOGNativeAbilityDefinition::MatchesSource(const FOGContentId& Identity,const FOGContentId& Version) const
{ return (IdentityId.IsEmpty() || IdentityId==Identity) && (VersionId.IsEmpty() || VersionId==Version); }
bool FOGNativeAbilityDefinition::Validate(FString& Error) const
{
    Error.Reset();
    if ((!IdentityId.IsEmpty() && !IdentityId.IsValid()) || (!VersionId.IsEmpty() && !VersionId.IsValid()) || !SkillId.IsValid() ||
        IdentityId.Value.StartsWith(TEXT("diagnostic:")) || VersionId.Value.StartsWith(TEXT("diagnostic:")) ||
        SkillId.Value.StartsWith(TEXT("diagnostic:")) || OriginalSourceReference.TrimStartAndEnd().IsEmpty() ||
        static_cast<uint8>(Targets)>static_cast<uint8>(EOGNativeAbilityTargets::AllAllies) ||
        (Packets.IsEmpty() && ResourceChanges.IsEmpty() && Effects.IsEmpty()) || Packets.Num()>4096 || Effects.Num()>4096)
    { Error=TEXT("Native ability needs valid optional source restrictions, skill, provenance and packets."); return false; }
    if (IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity")))
    { Error=TEXT("Luminous requires its complete specialized kit executor; ordinary packets cannot replace it."); return false; }
    if (bNoResourceCost!=ResourceCosts.IsEmpty() || CooldownOwnerActions<0)
    { Error=TEXT("Declare either no resource cost or exact typed costs, with a valid own-action cooldown."); return false; }
    for (const auto* Rows:{&ResourceCosts,&ResourceChanges})
    {
        TSet<FOGContentId> Resources;
        for (const auto& Row:*Rows)
        {
            if (!Row.ResourceId.IsValid() || Resources.Contains(Row.ResourceId) || Row.Amount==0 ||
                Row.Amount==MIN_int64 || (Rows==&ResourceCosts && Row.Amount<0))
            { Error=TEXT("Resource costs/changes require distinct valid IDs and safe explicit nonzero amounts."); return false; }
            Resources.Add(Row.ResourceId);
        }
    }
    for (const auto& Effect:Effects) if (!Effect.Validate(Error)) return false;
    for (const auto& Packet:Packets)
    {
        if (static_cast<uint8>(Packet.Effect)>static_cast<uint8>(EOGNativeAbilityEffect::Healing) ||
            static_cast<uint8>(Packet.DamageType)>static_cast<uint8>(EOGBaseDamageType::TrueDamage) ||
            static_cast<uint8>(Packet.DefenseReference)>static_cast<uint8>(EOGNativeDefenseReference::Defense) ||
            Packet.Repeats<1 || Packet.Repeats>10000 || Packet.FlatAmount.GetSign()<0 ||
            (Packet.Scaling.IsEmpty() && Packet.FlatAmount.GetSign()<=0) || Packet.Scaling.Num()>3)
        { Error=TEXT("Native packet kind, repeat count or scaling is invalid."); return false; }
        TSet<EOGNativeScaleStat> Stats;
        for (const auto& Scale:Packet.Scaling)
        {
            if (static_cast<uint8>(Scale.Stat)>static_cast<uint8>(EOGNativeScaleStat::Defense) ||
                Scale.MultiplierBps<=0 || Stats.Contains(Scale.Stat))
            { Error=TEXT("Native packet needs distinct explicitly weighted source stats."); return false; }
            Stats.Add(Scale.Stat);
        }
        if (Packet.Effect==EOGNativeAbilityEffect::Healing &&
            (Packet.bCanCrit || Packet.bCanBeBlocked || Packet.bAllowHitOverflow ||
             Packet.DefenseReference!=EOGNativeDefenseReference::Unspecified || Packet.DamageType!=EOGBaseDamageType::Physical))
        { Error=TEXT("Ordinary healing does not support damage-only flags; use its exact registered mechanic."); return false; }
        if (Packet.Effect==EOGNativeAbilityEffect::Damage && Packet.DamageType==EOGBaseDamageType::Physical &&
            Packet.DefenseReference==EOGNativeDefenseReference::Unspecified)
        { Error=TEXT("Physical damage requires an explicit authored defense reference."); return false; }
    }
    return true;
}
bool UOGNativeCombatAbility::ValidateInstalled(FString& Error) const
{
    if (HasAnyFlags(RF_Transient|RF_ClassDefaultObject) || !GetPathName().StartsWith(TEXT("/Game/")))
    { Error=TEXT("Native ability must be an installed authored game asset, not a transient command object."); return false; }
    return Definition.Validate(Error);
}
