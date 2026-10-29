#include "Combat/OGCombatVitalityService.h"
#include "Combat/OGTurnBattle.h"
#include "Dom/JsonObject.h"
#include "Misc/LexFromString.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr const TCHAR* VitalityField=TEXT("native_combat_vitality");
FOGEntityId VitalityRowId(const FOGEntityId& Entity)
{
    FGuid Guid;
    FGuid::ParseExact(FMD5::HashAnsiString(*(Entity.ToString()+TEXT("|native_combat_vitality_v1"))),EGuidFormats::Digits,Guid);
    return FOGEntityId(Guid);
}
bool Parse(const FString& Text,TSharedPtr<FJsonObject>& Out)
{ return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out) && Out.IsValid(); }
bool GuidField(const FJsonObject& Object,const TCHAR* Key,FOGEntityId& Out)
{
    FString Text; FGuid Value;
    if (!Object.TryGetStringField(Key,Text) || !FGuid::Parse(Text,Value) || !Value.IsValid()) return false;
    Out=FOGEntityId(Value); return true;
}
bool LifeId(const FJsonObject& Object,const FOGEntityId& Entity,FOGEntityId& Out,FString& Error)
{
    Out=Entity;
    if (Object.HasField(TEXT("combat_reward_life_id")) && !GuidField(Object,TEXT("combat_reward_life_id"),Out))
    { Error=TEXT("Canonical combat life identity is malformed."); return false; }
    return true;
}
bool LargeField(const FJsonObject& Object,const TCHAR* Key,FOGLargeNumber& Out)
{
    const TSharedPtr<FJsonObject>* Number=nullptr; FString Text; int64 Significand=0,Exponent=0;
    if (!Object.TryGetObjectField(Key,Number) || !Number || !Number->IsValid() ||
        !(*Number)->TryGetStringField(TEXT("significand_exact"),Text) || !LexTryParseString(Significand,*Text) ||
        Text!=LexToString(Significand) || Significand<0 || Significand>FOGLargeNumber::MaxSignificand ||
        !(*Number)->TryGetStringField(TEXT("exponent10_exact"),Text) || !LexTryParseString(Exponent,*Text) ||
        Text!=LexToString(Exponent) || Exponent<MIN_int32 || Exponent>MAX_int32) return false;
    Out=FOGLargeNumber(Significand,static_cast<int32>(Exponent)); return true;
}
TSharedRef<FJsonObject> LargeJson(FOGLargeNumber Value)
{
    Value.Normalize(); auto Out=MakeShared<FJsonObject>();
    Out->SetStringField(TEXT("significand_exact"),LexToString(Value.Significand));
    Out->SetStringField(TEXT("exponent10_exact"),LexToString(Value.Exponent10)); return Out;
}
bool ReadRoot(IOGWorldStore& Store,const FOGEntityId& Entity,FName& Kind,TSharedPtr<FJsonObject>& Root,FString& Error)
{
    bool Found=false; FString Json; int64 Revision=0;
    if (!Entity.IsValid() || !Store.TryReadEntity(Entity,Found,Kind,Json,Revision,Error)) return false;
    if (!Found || !Parse(Json,Root) || Root->HasField(TEXT("diagnosticFixture")))
    { Error=TEXT("Combat vitality requires an existing canonical entity and valid protected state."); return false; }
    FName SavedKind; FString SavedJson; int64 SavedRevision=0; bool HasSaved=false;
    if (!Store.TryReadEntity(VitalityRowId(Entity),HasSaved,SavedKind,SavedJson,SavedRevision,Error)) return false;
    if (HasSaved)
    {
        TSharedPtr<FJsonObject> Saved;
        const TSharedPtr<FJsonObject>* State=nullptr;
        if (SavedKind!=TEXT("combat_vitality") || !Parse(SavedJson,Saved) ||
            !Saved->TryGetObjectField(VitalityField,State) || !State || !State->IsValid())
        { Error=TEXT("Combat vitality record identity is occupied by incompatible state."); return false; }
        Root->SetObjectField(VitalityField,*State);
    }
    return true;
}
bool Decode(const FOGEntityId& Entity,const FJsonObject& Root,bool& Found,FOGStoredCombatVitality& Out,FString& Error)
{
    Found=false; Out=FOGStoredCombatVitality(); FOGEntityId CurrentLife;
    if (!LifeId(Root,Entity,CurrentLife,Error)) return false;
    if (!Root.HasField(VitalityField)) return true;
    const TSharedPtr<FJsonObject>* Saved=nullptr;
    if (!Root.TryGetObjectField(VitalityField,Saved) || !Saved || !Saved->IsValid())
    { Error=TEXT("Stored combat vitality is malformed."); return false; }
    const auto& V=**Saved; double Schema=0; FString Identity,Version,Tick;
    if (!V.TryGetNumberField(TEXT("schema"),Schema) || Schema!=1 ||
        !GuidField(V,TEXT("entity_id"),Out.EntityId) || Out.EntityId!=Entity ||
        !GuidField(V,TEXT("life_id"),Out.LifeId) || !GuidField(V,TEXT("battle_id"),Out.BattleId) ||
        !GuidField(V,TEXT("receipt_id"),Out.ReceiptId) ||
        !V.TryGetStringField(TEXT("identity_id"),Identity) || !V.TryGetStringField(TEXT("version_id"),Version) ||
        !V.TryGetStringField(TEXT("world_tick_exact"),Tick) || !LexTryParseString(Out.WorldTick,*Tick) ||
        Tick!=LexToString(Out.WorldTick) || Out.WorldTick<0 ||
        !LargeField(V,TEXT("hp"),Out.CurrentHp) || !LargeField(V,TEXT("maximum_hp"),Out.MaximumHpAtCommit) ||
        Out.MaximumHpAtCommit.GetSign()<=0 || FOGLargeNumber::Compare(Out.CurrentHp,Out.MaximumHpAtCommit)>0)
    { Error=TEXT("Stored combat vitality has invalid exact values or provenance."); return false; }
    Out.IdentityId=FOGContentId(Identity); Out.VersionId=FOGContentId(Version);
    if (!Out.IdentityId.IsValid() || !Out.VersionId.IsValid())
    { Error=TEXT("Stored combat vitality has invalid character identity/version."); return false; }
    // Only the existing canonical life epoch can authorize a real respawn.
    Found=Out.LifeId==CurrentLife; return true;
}
const FOGCombatUnitState* UnitById(const FOGTurnBattleState& State,const FOGEntityId& Id)
{ return State.Units.FindByPredicate([&](const auto& Unit){return Unit.UnitEntityId==Id;}); }
bool HasHealthProof(const FJsonObject& Payload,const FOGCombatUnitState& Before,const FOGCombatUnitState& After,bool Opening)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if (!Payload.TryGetArrayField(Opening?TEXT("opening_units"):TEXT("unit_deltas"),Rows) || !Rows) return false;
    int32 Matches=0;
    for (const auto& Value:*Rows)
    {
        if (!Value || Value->Type!=EJson::Object) return false;
        const auto Row=Value->AsObject(); FString Id,Previous,Current;
        if (!Row || !Row->TryGetStringField(TEXT("unit_id"),Id)) return false;
        if (Id!=After.UnitEntityId.ToString()) continue;
        if (!Row->TryGetStringField(TEXT("hp_before"),Previous) || !Row->TryGetStringField(TEXT("hp_after"),Current) ||
            Previous!=Before.CurrentHp.ToDebugString() || Current!=After.CurrentHp.ToDebugString()) return false;
        ++Matches;
    }
    return Matches==1;
}
}
bool FOGCombatVitalityService::Read(const FOGEntityId& Entity,bool& Found,FOGStoredCombatVitality& Out,FString& Error) const
{
    Error.Reset(); Found=false; Out=FOGStoredCombatVitality();
    FName Kind; TSharedPtr<FJsonObject> Root;
    return ReadRoot(Store,Entity,Kind,Root,Error) && Decode(Entity,*Root,Found,Out,Error);
}
bool FOGCombatVitalityService::Project(FOGCombatUnitState& Unit,FString& Error) const
{
    bool Found=false; FOGStoredCombatVitality Saved;
    if (!Read(Unit.UnitEntityId,Found,Saved,Error)) return false;
    if (!Found) return true;
    if (Unit.IdentityId!=Saved.IdentityId || Unit.Stats.MaxHp.GetSign()<=0)
    { Error=TEXT("Stored vitality cannot be borrowed by a different identity or unresolved character."); return false; }
    Unit.CurrentHp=FOGLargeNumber::Compare(Saved.CurrentHp,Unit.Stats.MaxHp)>0?Unit.Stats.MaxHp:Saved.CurrentHp;
    if (!Unit.IsAlive()) { Unit.Presence=EOGCombatPresence::Defeated; Unit.OccupiedLaneIndex=INDEX_NONE; }
    return true;
}
bool FOGCombatVitalityService::Commit(const FOGTurnBattleState& Before,const FOGTurnBattleState& After,
    const FOGEntityId& Ruler,const FOGWorldEvent& Receipt,FString& Error)
{
    Error.Reset();
    if (!Store.IsWriteTransactionActive() || !Ruler.IsValid() || !Before.BattleId.IsValid() ||
        Before.BattleId!=After.BattleId || Receipt.PrimaryEntity!=Ruler || !Receipt.EventId.IsValid() ||
        Before.Units.Num()!=After.Units.Num())
    { Error=TEXT("Combat vitality requires the existing encounter's active receipt transaction."); return false; }
    bool Exists=false; FOGWorldEvent Durable; TSharedPtr<FJsonObject> Proof; FString Battle;
    if (!Store.TryReadWorldEvent(Receipt.EventId,Exists,Durable,Error)) return false;
    const bool Opening=Receipt.EventType==TEXT("combat.encounter_started");
    const bool Allowed=Opening || Receipt.EventType==TEXT("combat.turn_action_resolved") ||
        Receipt.EventType==TEXT("combat.world_mode_changed") || Receipt.EventType==TEXT("combat.world_cast_started") ||
        Receipt.EventType==TEXT("combat.world_impact_resolved") || Receipt.EventType==TEXT("combat.world_cast_cancelled") ||
        Receipt.EventType==TEXT("combat.world_clock_advanced");
    if (!Exists || !Allowed || Durable.EventType!=Receipt.EventType || Durable.PrimaryEntity!=Ruler ||
        Durable.WorldTick!=Receipt.WorldTick || Receipt.WorldTick<0 || Durable.PayloadJson!=Receipt.PayloadJson ||
        !Parse(Durable.PayloadJson,Proof) || !Proof->TryGetStringField(TEXT("battle_id"),Battle) || Battle!=After.BattleId.ToString())
    { Error=TEXT("Combat vitality has no matching durable gameplay receipt."); return false; }
    struct FWrite { FOGEntityId Entity; FName Kind; FString Json; };
    TArray<FWrite> Writes; TSet<FOGEntityId> Seen;
    for (const auto& Unit:After.Units)
    {
        const auto* Previous=UnitById(Before,Unit.UnitEntityId);
        if (!Previous || Seen.Contains(Unit.UnitEntityId) || Unit.IdentityId!=Previous->IdentityId ||
            !Unit.IdentityId.IsValid() || !Unit.SkillSet.CurrentForm.IsValid() || Unit.Stats.MaxHp.GetSign()<=0 ||
            Unit.CurrentHp.GetSign()<0 || FOGLargeNumber::Compare(Unit.CurrentHp,Unit.Stats.MaxHp)>0 ||
            (Unit.Presence==EOGCombatPresence::Defeated && Unit.IsAlive()))
        { Error=TEXT("Combat vitality cannot invent participants, invalid HP or ownership."); return false; }
        Seen.Add(Unit.UnitEntityId);
        FName Kind; TSharedPtr<FJsonObject> Root;
        if (!ReadRoot(Store,Unit.UnitEntityId,Kind,Root,Error)) return false;
        bool Found=false; FOGStoredCombatVitality Prior;
        if (!Decode(Unit.UnitEntityId,*Root,Found,Prior,Error)) return false;
        if (Found)
        {
            if (Prior.IdentityId!=Unit.IdentityId)
            { Error=TEXT("Canonical combat vitality belongs to another identity."); return false; }
            if (Prior.ReceiptId==Receipt.EventId)
            {
                if (FOGLargeNumber::Compare(Prior.CurrentHp,Unit.CurrentHp)!=0) { Error=TEXT("The same vitality receipt cannot have conflicting HP."); return false; }
                continue;
            }
            const auto Expected=FOGLargeNumber::Compare(Prior.CurrentHp,Previous->Stats.MaxHp)>0?Previous->Stats.MaxHp:Prior.CurrentHp;
            if (FOGLargeNumber::Compare(Expected,Previous->CurrentHp)!=0 || Prior.WorldTick>Receipt.WorldTick)
            { Error=TEXT("Canonical health changed since this encounter snapshot; stale damage/healing is rejected."); return false; }
        }
        if (!Found && Root->HasField(VitalityField) && !Opening)
        { Error=TEXT("Canonical life changed during this encounter; stale effects cannot affect a new life."); return false; }
        if (Found && FOGLargeNumber::Compare(Previous->CurrentHp,Unit.CurrentHp)==0 &&
            FOGLargeNumber::Compare(Prior.CurrentHp,Unit.CurrentHp)==0) continue;
        if (!HasHealthProof(*Proof,*Previous,Unit,Opening))
        { Error=TEXT("The durable receipt does not prove this actual HP projection."); return false; }
        FOGEntityId Life; if (!LifeId(*Root,Unit.UnitEntityId,Life,Error)) return false;
        auto Saved=MakeShared<FJsonObject>(); Saved->SetNumberField(TEXT("schema"),1);
        Saved->SetStringField(TEXT("entity_id"),Unit.UnitEntityId.ToString()); Saved->SetStringField(TEXT("life_id"),Life.ToString());
        Saved->SetStringField(TEXT("identity_id"),Unit.IdentityId.ToString()); Saved->SetStringField(TEXT("version_id"),Unit.SkillSet.CurrentForm.ToString());
        Saved->SetStringField(TEXT("battle_id"),After.BattleId.ToString()); Saved->SetStringField(TEXT("receipt_id"),Receipt.EventId.ToString());
        Saved->SetStringField(TEXT("world_tick_exact"),LexToString(Receipt.WorldTick));
        Saved->SetObjectField(TEXT("hp"),LargeJson(Unit.CurrentHp)); Saved->SetObjectField(TEXT("maximum_hp"),LargeJson(Unit.Stats.MaxHp));
        auto Record=MakeShared<FJsonObject>(); Record->SetObjectField(VitalityField,Saved);
        FWrite Write; Write.Entity=VitalityRowId(Unit.UnitEntityId); Write.Kind=TEXT("combat_vitality");
        if (!FJsonSerializer::Serialize(Record.ToSharedRef(),TJsonWriterFactory<>::Create(&Write.Json)))
        { Error=TEXT("Cannot serialize canonical combat vitality."); return false; }
        Writes.Add(MoveTemp(Write));
    }
    for (const auto& Write:Writes) if (!Store.UpsertEntity(Write.Entity,Write.Kind,Receipt.WorldTick,Write.Json,Error)) return false;
    return true;
}
