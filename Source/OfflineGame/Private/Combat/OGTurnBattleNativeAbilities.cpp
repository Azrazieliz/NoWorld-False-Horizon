#include "Combat/OGTurnBattle.h"
#include "Combat/OGApprovedLuminousSourceKit.h"
#include "Combat/OGLuminousAlterV1Projection.h"
#include "Random/OGDeterministicRng.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString AbilityKey(const FOGContentId& Identity,const FOGContentId& Version,const FOGContentId& Skill)
{ return Identity.ToString()+TEXT("|")+Version.ToString()+TEXT("|")+Skill.ToString(); }
bool Owns(const FOGCombatUnitState& Source,const FOGContentId& Skill)
{ return Source.SkillSet.ActiveSkills.Contains(Skill) || Source.SkillSet.UltimateSkill==Skill; }
FOGLargeNumber StatValue(const FOGCombatStats& S,EOGNativeScaleStat Stat)
{ return Stat==EOGNativeScaleStat::Attack?S.Attack:Stat==EOGNativeScaleStat::MaxHp?S.MaxHp:S.Defense; }
uint64 AbilitySeed(const FOGTurnBattleState& State,const FOGResolvedCombatAction& Action,int64 Sequence)
{
    const FString Text=State.BattleId.ToString()+Action.SourceUnitId.ToString()+Action.SkillId.ToString()+FString::Printf(TEXT(":%lld"),static_cast<long long>(Sequence));
    FTCHARToUTF8 Bytes(*Text); uint64 Value=14695981039346656037ull;
    for (int32 I=0;I<Bytes.Length();++I) { Value^=static_cast<uint8>(Bytes.Get()[I]); Value*=1099511628211ull; }
    return Value; // ActionId and player-provided JSON cannot reroll an unchanged turn.
}
}
const FOGNativeAbilityDefinition* FOGTurnBattle::FindNativeAbility(const FOGCombatUnitState& Source,const FOGContentId& Skill) const
{ return NativeAbilities.Find(AbilityKey(Source.IdentityId,Source.SkillSet.CurrentForm,Skill)); }
FName FOGTurnBattle::ResolveNativeAbilityMotion(const FOGCombatUnitState& Source,const FOGContentId& Skill) const
{
    if (const auto* Definition=FindNativeAbility(Source,Skill)) return Definition->MotionSemantic;
    return NAME_None;
}
bool FOGTurnBattle::RegisterNativeAbility(const FOGNativeAbilityDefinition& Definition,FString& Error)
{
    if (State.Status!=EOGTurnBattleStatus::Running) { Error=TEXT("Native abilities require a running battle."); return false; }
    if (!Definition.Validate(Error)) return false;
    if (!State.Units.ContainsByPredicate([&Definition](const FOGCombatUnitState& U)
        { return U.IdentityId==Definition.IdentityId && U.SkillSet.CurrentForm==Definition.VersionId && Owns(U,Definition.SkillId); }))
    { Error=TEXT("Native ability has no matching actual battle participant/owned kit."); return false; }
    const FString Key=AbilityKey(Definition.IdentityId,Definition.VersionId,Definition.SkillId);
    if (const auto* Existing=NativeAbilities.Find(Key))
    {
        if (*Existing==Definition) return true;
        Error=TEXT("An admitted battle ability cannot change midway through an encounter."); return false;
    }
    NativeAbilities.Add(Key,Definition); ++NativeAbilityRevision; return true;
}
bool FOGTurnBattle::HasNativeActionExecutor(const FOGCombatUnitState& Source,const FOGContentId& Skill,FString& Error) const
{
    Error.Reset();
    if (Source.IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity")))
    {
        FOGApprovedLuminousSourceKitView Base; FOGLuminousAlterV1Projection Alter;
        if (!FOGApprovedLuminousSourceKit::LoadBaseV6(Base,Error) || !FOGLuminousAlterV1Projection::LoadApprovedV1(Alter,Error)) return false;
        if (Source.SkillSet.CurrentForm==Base.VersionId &&
            (Base.CoreSkills.ActiveSkills.Contains(Skill) || Base.CoreSkills.UltimateSkill==Skill || Base.WorldFantasmId==Skill)) return true;
        if (Source.SkillSet.CurrentForm==Alter.NativeVersionId &&
            (Alter.Skills.ActiveSkills.Contains(Skill) || Alter.Skills.UltimateSkill==Skill || Alter.WorldFantasmId==Skill)) return true;
    }
    else if (Owns(Source,Skill) && FindNativeAbility(Source,Skill)) return true;
    Error=TEXT("No real native executor is registered for this exact source Version and ability."); return false;
}
bool FOGTurnBattle::ExpandNativeAbilityTargets(const FOGResolvedCombatAction& Original,FOGResolvedCombatAction& Expanded,
    bool& Handled,bool& Area,FString& Error) const
{
    Handled=Area=false;
    const auto* Source=FindUnit(Original.SourceUnitId);
    const auto* Definition=Source?FindNativeAbility(*Source,Original.SkillId):nullptr;
    if (!Definition) return true;
    Handled=true; Expanded=Original;
    if (Definition->Targets==EOGNativeAbilityTargets::Self) { Expanded.TargetUnitIds={Source->UnitEntityId}; return true; }
    Area=Definition->Targets==EOGNativeAbilityTargets::AllEnemies || Definition->Targets==EOGNativeAbilityTargets::AllAllies;
    if (!Area) return true;
    const bool Allies=Definition->Targets==EOGNativeAbilityTargets::AllAllies;
    Expanded.TargetUnitIds.Reset();
    for (const auto& Unit:State.Units)
        if (Unit.CanAct() && ((Unit.TeamIndex==Source->TeamIndex)==Allies) &&
            (Unit.UnitEntityId!=Source->UnitEntityId || Definition->bIncludeSelf))
            Expanded.TargetUnitIds.Add(Unit.UnitEntityId);
    if (Expanded.TargetUnitIds.IsEmpty()) { Error=TEXT("No eligible active targets for this authored field ability."); return false; }
    if (!Original.TargetUnitIds.IsEmpty() && Expanded.TargetUnitIds.Contains(Original.TargetUnitIds[0]))
    { Expanded.TargetUnitIds.Remove(Original.TargetUnitIds[0]); Expanded.TargetUnitIds.Insert(Original.TargetUnitIds[0],0); }
    return true;
}
bool FOGTurnBattle::ValidateNativeAbility(const FOGResolvedCombatAction& Action,const FOGCombatUnitState& Source,FString& Error) const
{
    const auto* Definition=FindNativeAbility(Source,Action.SkillId);
    if (!Definition) return true;
    TSharedPtr<FJsonObject> Context;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Action.ResolutionJson),Context) ||
        !Context || !Context->Values.IsEmpty())
    { Error=TEXT("Ordinary authored packets do not accept unregistered branches or numerical command overrides."); return false; }
    if (!Owns(Source,Action.SkillId) || Action.InterruptPriority!=0)
    { Error=TEXT("Ordinary registered ability is not owned or cannot resolve an interrupt."); return false; }
    const bool Self=Definition->Targets==EOGNativeAbilityTargets::Self;
    const bool Allies=Self || Definition->Targets==EOGNativeAbilityTargets::SingleAlly || Definition->Targets==EOGNativeAbilityTargets::AllAllies;
    const bool Single=Self || Definition->Targets==EOGNativeAbilityTargets::SingleAlly || Definition->Targets==EOGNativeAbilityTargets::SingleEnemy;
    if (Action.TargetUnitIds.IsEmpty() || (Single && Action.TargetUnitIds.Num()!=1))
    { Error=TEXT("Registered ability requires its authored single/field target set."); return false; }
    TSet<FOGEntityId> Seen;
    for (const auto& Id:Action.TargetUnitIds)
    {
        const auto* Target=FindUnit(Id);
        if (!Target || !Target->CanAct() || Seen.Contains(Id) || (Self && Id!=Source.UnitEntityId) ||
            (!Self && ((Target->TeamIndex==Source.TeamIndex)!=Allies)) ||
            (!Self && Id==Source.UnitEntityId && !Definition->bIncludeSelf))
        { Error=TEXT("Registered ability target is absent, off-field, duplicate or has the wrong allegiance."); return false; }
        Seen.Add(Id);
    }
    for (const auto& Packet:Definition->Packets)
        if (Packet.Effect==EOGNativeAbilityEffect::Damage && Packet.DamageType==EOGBaseDamageType::Physical)
        {
            const auto Reference=Packet.DefenseReference==EOGNativeDefenseReference::Attack?Source.Stats.Attack:
                Packet.DefenseReference==EOGNativeDefenseReference::MaxHp?Source.Stats.MaxHp:Source.Stats.Defense;
            if (Reference.GetSign()<=0) { Error=TEXT("Authored physical damage reference must be positive."); return false; }
        }
    return true;
}
bool FOGTurnBattle::ExecuteNativeAbility(const FOGResolvedCombatAction& Action,FOGCombatUnitState& Source,FString& Error)
{
    const auto* Definition=FindNativeAbility(Source,Action.SkillId);
    if (!Definition) return true; // Strict production dispatch was checked before mutation.
    FOGDeterministicRng Rng(AbilitySeed(State,Action,Log.GetNextSequence()));
    for (const auto& Packet:Definition->Packets)
    for (int32 Hit=0;Hit<Packet.Repeats;++Hit)
    for (const auto& Id:Action.TargetUnitIds)
    {
        auto* Target=FindMutableUnit(Id); if (!Target || !Target->CanAct()) continue;
        FOGCombatStats Attacker=Source.Stats;
        int32 Attack=0,Speed=0,Healing=0,Reduction=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,Attack,Speed,Healing,Reduction);
        if (Attack>0) Attacker.Attack=FOGLargeNumber::ScaleByBasisPoints(Attacker.Attack,10000+Attack);
        FOGLargeNumber Amount;
        for (const auto& Term:Packet.Scaling)
            Amount=FOGLargeNumber::Add(Amount,FOGLargeNumber::ScaleByBasisPoints(StatValue(Attacker,Term.Stat),Term.MultiplierBps));
        if (Amount.GetSign()<0) { Error=TEXT("Authored scaling resolved to negative magnitude."); return false; }
        if (Packet.Effect==EOGNativeAbilityEffect::Healing)
        {
            if (!ApplyHealing(Source.UnitEntityId,Id,Amount,Action.SkillId,Error)) return false;
            continue;
        }
        FOGDamageRequest Request; Request.BaseDamage=Amount; Request.DamageType=Packet.DamageType;
        Request.DefenseReference=Packet.DefenseReference==EOGNativeDefenseReference::Attack?Attacker.Attack:
            Packet.DefenseReference==EOGNativeDefenseReference::MaxHp?Attacker.MaxHp:Attacker.Defense;
        Request.bCanCrit=Packet.bCanCrit; Request.bCanBeBlocked=Packet.bCanBeBlocked;
        Request.bAllowHitOverflowReplication=Packet.bAllowHitOverflow;
        if (!ResolveRankSuppressionMultiplier(Source.UnitEntityId,Id,OGRankSuppressionChannels::Damage(),Request.RankSuppressionMultiplierBps,Error)) return false;
        const auto Damage=FOGCombatMath::ResolveDamage(Request,Attacker,Target->Stats,Rng);
        if (!Damage.TotalDamage.IsZero() && !ApplyDamage(Source.UnitEntityId,Id,Damage.TotalDamage,Action.SkillId,Error)) return false;
    }
    return true;
}
