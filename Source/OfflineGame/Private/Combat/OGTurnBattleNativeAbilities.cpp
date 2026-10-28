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
bool CompleteKit(const FOGResolvedSkillSet& Source,const FOGResolvedSkillSet& Approved)
{
    if (Source.CurrentForm!=Approved.CurrentForm || Source.ActiveSkills.Num()!=Approved.ActiveSkills.Num() ||
        Source.PassiveSkills.Num()!=Approved.PassiveSkills.Num() || !Source.HasSameUltimates(Approved) ||
        Source.HasDuplicateSkills()) return false;
    for (const auto& Skill:Approved.ActiveSkills) if (!Source.ActiveSkills.Contains(Skill)) return false;
    for (const auto& Skill:Approved.PassiveSkills) if (!Source.PassiveSkills.Contains(Skill)) return false;
    return true;
}
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
{
    if (Source.IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity"))) return nullptr;
    const FOGContentId Any;
    for (const FString& Key:{AbilityKey(Source.IdentityId,Source.SkillSet.CurrentForm,Skill),
        AbilityKey(Source.IdentityId,Any,Skill),AbilityKey(Any,Source.SkillSet.CurrentForm,Skill),AbilityKey(Any,Any,Skill)})
        if (const auto* Found=NativeAbilities.Find(Key)) return Found;
    return nullptr;
}
FName FOGTurnBattle::ResolveNativeAbilityMotion(const FOGCombatUnitState& Source,const FOGContentId& Skill) const
{
    if (const auto* Definition=FindNativeAbility(Source,Skill)) return Definition->MotionSemantic;
    return NAME_None;
}
bool FOGTurnBattle::ConfigureNativeResource(const FOGEntityId& UnitId,
    const FOGCombatResourceSpecification& Specification,FString& Error)
{
    if (State.Status!=EOGTurnBattleStatus::Running || !Specification.Validate(Error))
    { if (Error.IsEmpty()) Error=TEXT("Resources require a running battle and valid specification."); return false; }
    auto* Unit=FindMutableUnit(UnitId);
    if (!Unit) { Error=TEXT("Combat resource owner is not an actual battle participant."); return false; }
    const FString Key=UnitId.ToString()+TEXT("|")+Specification.ResourceId.ToString();
    if (const auto* Existing=NativeResourceSpecifications.Find(Key))
    {
        if (*Existing==Specification) { Error.Reset(); return true; }
        Error=TEXT("An admitted battle resource cannot be changed or reset."); return false;
    }
    if (NativeOwnerActions.FindRef(UnitId)!=0)
    { Error=TEXT("Opening resource definitions cannot be added after this unit has acted."); return false; }
    FOGCombatResourceValue Value; Value.ResourceId=Specification.ResourceId; Value.DisplayName=Specification.DisplayName;
    Value.Maximum=Specification.Maximum; Value.Value=Specification.OpeningValue;
    Unit->BattleResources.Add(Value); NativeResourceSpecifications.Add(Key,Specification); ++NativeAbilityRevision;
    return true;
}
bool FOGTurnBattle::RegisterNativeAbility(const FOGNativeAbilityDefinition& Definition,FString& Error)
{
    if (State.Status!=EOGTurnBattleStatus::Running) { Error=TEXT("Native abilities require a running battle."); return false; }
    if (!Definition.Validate(Error)) return false;
    if (!State.Units.ContainsByPredicate([this,&Definition](const FOGCombatUnitState& U)
        { return Definition.MatchesSource(U.IdentityId,U.SkillSet.CurrentForm) &&
            U.IdentityId!=FOGContentId(TEXT("character:luminous_valentine.identity")) && CanAdmitNativeAbility(U,Definition.SkillId); }))
    { Error=TEXT("Native ability has no matching actual battle participant/owned kit."); return false; }
    for (const auto& Unit:State.Units)
        if (Unit.IdentityId!=FOGContentId(TEXT("character:luminous_valentine.identity")) &&
            Definition.MatchesSource(Unit.IdentityId,Unit.SkillSet.CurrentForm) && CanAdmitNativeAbility(Unit,Definition.SkillId))
            for (const auto* Rows:{&Definition.ResourceCosts,&Definition.ResourceChanges})
                for (const auto& Resource:*Rows)
                    if (!NativeResourceSpecifications.Contains(Unit.UnitEntityId.ToString()+TEXT("|")+Resource.ResourceId.ToString()))
                    { Error=TEXT("A shared ability references a battle resource not admitted for an actual source unit."); return false; }
    for (const auto& Operation:Definition.Effects)
        if (Operation.Operation==EOGNativeEffectOperation::Apply && !NativeEffectDefinitions.Contains(Operation.Id))
        { Error=TEXT("Shared ability references an effect not admitted to this encounter: ")+Operation.Id.ToString(); return false; }
    const FString Key=AbilityKey(Definition.IdentityId,Definition.VersionId,Definition.SkillId);
    if (const auto* Existing=NativeAbilities.Find(Key))
    {
        if (*Existing==Definition) return true;
        Error=TEXT("An admitted battle ability cannot change midway through an encounter."); return false;
    }
    for (const auto& Pair:NativeAbilities)
        if (Pair.Value.SkillId==Definition.SkillId)
            for (const auto& Unit:State.Units)
                if (Definition.MatchesSource(Unit.IdentityId,Unit.SkillSet.CurrentForm) &&
                    Pair.Value.MatchesSource(Unit.IdentityId,Unit.SkillSet.CurrentForm) && CanAdmitNativeAbility(Unit,Definition.SkillId))
                { Error=TEXT("Overlapping source scopes cannot replace an admitted shared ability."); return false; }
    NativeAbilities.Add(Key,Definition); ++NativeAbilityRevision; return true;
}
bool FOGTurnBattle::HasNativeActionExecutor(const FOGCombatUnitState& Source,const FOGContentId& Skill,FString& Error) const
{
    Error.Reset();
    if (Source.IdentityId==FOGContentId(TEXT("character:luminous_valentine.identity")))
    {
        FOGApprovedLuminousSourceKitView Base; FOGLuminousAlterV1Projection Alter;
        if (!FOGApprovedLuminousSourceKit::LoadBaseV6(Base,Error) || !FOGLuminousAlterV1Projection::LoadApprovedV1(Alter,Error)) return false;
        if (CompleteKit(Source.SkillSet,Base.CoreSkills) &&
            (Base.CoreSkills.ActiveSkills.Contains(Skill) || Base.CoreSkills.UltimateSkill==Skill || Base.WorldFantasmId==Skill)) return true;
        if (CompleteKit(Source.SkillSet,Alter.Skills) &&
            (Alter.Skills.ActiveSkills.Contains(Skill) || Alter.Skills.UltimateSkill==Skill || Alter.WorldFantasmId==Skill)) return true;
    }
    else if (IsNativeAbilityAuthorized(Source,Skill) && FindNativeAbility(Source,Skill)) return true;
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
    const FString CooldownKey=Source.UnitEntityId.ToString()+TEXT("|")+Action.SkillId.ToString();
    if (NativeOwnerActions.FindRef(Source.UnitEntityId)+1<NativeAbilityReadyActions.FindRef(CooldownKey))
    { Error=TEXT("The authored own-action cooldown is not ready."); return false; }
    for (const auto* Rows:{&Definition->ResourceCosts,&Definition->ResourceChanges})
    for (const auto& Row:*Rows)
    {
        const auto* Resource=Source.BattleResources.FindByPredicate([&](const auto& R){return R.ResourceId==Row.ResourceId;});
        const FString Key=Source.UnitEntityId.ToString()+TEXT("|")+Row.ResourceId.ToString();
        if (!Resource || !NativeResourceSpecifications.Contains(Key) || Resource->Value<0 ||
            Resource->Value>Resource->Maximum || (Rows==&Definition->ResourceCosts && Resource->Value<Row.Amount))
        { Error=TEXT("An authored battle resource is unavailable or its exact cost is unaffordable."); return false; }
    }
    TSharedPtr<FJsonObject> Context;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Action.ResolutionJson),Context) ||
        !Context || !Context->Values.IsEmpty())
    { Error=TEXT("Ordinary authored packets do not accept unregistered branches or numerical command overrides."); return false; }
    if (!IsNativeAbilityAuthorized(Source,Action.SkillId) ||
        (Action.InterruptPriority!=0 && !HasNativeReactionProof(Source,Action.SkillId)))
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
        const bool ReserveSelf=Target && Id==Source.UnitEntityId &&
            AllowsNativeReactionSource(Source,Action) && Target->Presence==EOGCombatPresence::Reserve && Target->IsAlive();
        if (!Target || (!Target->CanAct() && !ReserveSelf) || Seen.Contains(Id) || (Self && Id!=Source.UnitEntityId) ||
            (!Self && ((Target->TeamIndex==Source.TeamIndex)!=Allies)) ||
            (!Self && Id==Source.UnitEntityId && !Definition->bIncludeSelf))
        { Error=TEXT("Registered ability target is absent, off-field, duplicate or has the wrong allegiance."); return false; }
        Seen.Add(Id);
    }
    for (const auto& Packet:Definition->Packets)
        if (Packet.Effect==EOGNativeAbilityEffect::Damage && Packet.DamageType==EOGBaseDamageType::Physical)
        {
            const auto Effective=GetNativeEffectiveStats(Source.UnitEntityId);
            const auto Reference=Packet.DefenseReference==EOGNativeDefenseReference::Attack?Effective.Attack:
                Packet.DefenseReference==EOGNativeDefenseReference::MaxHp?Effective.MaxHp:Effective.Defense;
            if (Reference.GetSign()<=0) { Error=TEXT("Authored physical damage reference must be positive."); return false; }
        }
    return true;
}
bool FOGTurnBattle::ExecuteNativeAbility(const FOGResolvedCombatAction& Action,FOGCombatUnitState& Source,FString& Error)
{
    const auto* Definition=FindNativeAbility(Source,Action.SkillId);
    if (!Definition) return true; // Strict production dispatch was checked before mutation.
    for (const auto& Cost:Definition->ResourceCosts)
    {
        auto* Resource=Source.BattleResources.FindByPredicate([&](const auto& R){return R.ResourceId==Cost.ResourceId;});
        if (!Resource || Resource->Value<Cost.Amount) { Error=TEXT("Battle resource cost changed before execution."); return false; }
        Resource->Value-=Cost.Amount;
    }
    FOGDeterministicRng Rng(AbilitySeed(State,Action,Log.GetNextSequence()));
    if (!ExecuteNativeEffects(Action,true,Rng,Error)) return false;
    for (const auto& Packet:Definition->Packets)
    for (int32 Hit=0;Hit<Packet.Repeats;++Hit)
    for (const auto& Id:Action.TargetUnitIds)
    {
        auto* Target=FindMutableUnit(Id); if (!Target || (!Target->CanAct() && !(Id==Source.UnitEntityId && AllowsNativeReactionSource(Source,Action)))) continue;
        FOGCombatStats Attacker=GetNativeEffectiveStats(Source.UnitEntityId);
        int32 Attack=0,Speed=0,Healing=0,Reduction=0;
        GetApprovedDawnBlessingBps(Source.UnitEntityId,Attack,Speed,Healing,Reduction);
        if (Attack>0) Attacker.Attack=FOGLargeNumber::ScaleByBasisPoints(Attacker.Attack,10000+Attack);
        FOGLargeNumber Amount=Packet.FlatAmount;
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
        const auto Damage=FOGCombatMath::ResolveDamage(Request,Attacker,GetNativeEffectiveStats(Id),Rng);
        if (!Damage.TotalDamage.IsZero() && !ApplyDamage(Source.UnitEntityId,Id,Damage.TotalDamage,Action.SkillId,Error)) return false;
    }
    if (!ExecuteNativeEffects(Action,false,Rng,Error)) return false;
    for (const auto& Change:Definition->ResourceChanges)
    {
        auto* Resource=Source.BattleResources.FindByPredicate([&](const auto& R){return R.ResourceId==Change.ResourceId;});
        if (!Resource) { Error=TEXT("Battle resource change has no admitted destination."); return false; }
        // Saturation without signed overflow, including capacities near int64 maximum.
        Resource->Value=Change.Amount>0
            ? (Change.Amount>Resource->Maximum-Resource->Value?Resource->Maximum:Resource->Value+Change.Amount)
            : (Change.Amount < -Resource->Value?0:Resource->Value+Change.Amount);
    }
    NativeAbilityReadyActions.Add(Source.UnitEntityId.ToString()+TEXT("|")+Action.SkillId.ToString(),
        NativeOwnerActions.FindRef(Source.UnitEntityId)+Definition->CooldownOwnerActions+(Action.InterruptPriority>0?1:2));
    return true;
}
