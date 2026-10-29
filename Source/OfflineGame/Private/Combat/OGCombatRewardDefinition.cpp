#include "Combat/OGCombatRewardDefinition.h"
#include "Objects/OGObjectActionService.h"
#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FOGEntityId RewardId(const FString& Key)
{
    FTCHARToUTF8 Bytes(*Key); FMD5 Hash; Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length());
    uint8 Result[16]; Hash.Final(Result); FGuid Guid; FGuid::ParseExact(BytesToHex(Result,16),EGuidFormats::Digits,Guid);
    return FOGEntityId(Guid);
}
bool Json(const FString& Text,TSharedPtr<FJsonObject>& Out)
{ return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Out) && Out.IsValid(); }
}
bool UOGCombatRewardDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error)) return false;
    if (System!=TEXT("combat_reward") || DisplayName.IsEmpty() || EnemyEntityKind.IsNone() ||
        OriginalSourceReference.TrimStartAndEnd().IsEmpty() || (Objects.IsEmpty() && Resources.IsEmpty()))
    { Error=TEXT("Combat rewards require exact source grants and an enemy entity kind."); return false; }
    int64 Count=0;
    for (const auto& O:Objects)
    {
        if (!O.ObjectId.IsValid() || O.To!=EOGObjectActionParty::Ruler || O.InstanceCount<1 || O.InstanceCount>10000)
        { Error=TEXT("Victory objects must be positive bounded grants to the actual Ruler."); return false; }
        Count+=O.InstanceCount;
    }
    TSet<FOGContentId> Seen;
    for (const auto& R:Resources)
    {
        if (!R.ResourceId.IsValid() || R.DisplayName.IsEmpty() || R.Account!=EOGObjectActionParty::Ruler || R.Amount<=0 || Seen.Contains(R.ResourceId))
        { Error=TEXT("Victory resources must be distinct positive exact grants to the actual Ruler."); return false; }
        Seen.Add(R.ResourceId);
    }
    if (Count>10000) { Error=TEXT("Victory object allocation exceeds its native transaction capacity."); return false; }
    return true;
}
void UOGCombatRewardDefinition::CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const
{
    Super::CollectRequiredDefinitions(Out);
    for (const auto& O:Objects) { FOGDefinitionReference R; R.System=TEXT("object"); R.Id=O.ObjectId; Out.Add(R); }
}
bool UOGCombatRewardDefinition::ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const
{
    for (const auto& O:Objects) if (!Cast<UOGObjectDefinition>(Resolve(TEXT("object"),O.ObjectId)))
    { Error=TEXT("Combat reward object requires the shared native object definition."); return false; }
    return true;
}
bool UOGCombatRewardDefinitionReceiver::PrepareDefinition_Implementation(
    UDataAsset* Asset,const FOGDefinitionBindingContext& Context,FString& Error)
{
    const auto* Definition=Cast<UOGCombatRewardDefinition>(Asset);
    if (!Definition || !Definition->ValidateDefinition(Error) || Definition->EnemyEntityKind!=Context.EntityKind)
    { if (Error.IsEmpty()) Error=TEXT("Combat reward definition does not match this real entity kind."); return false; }
    return true;
}
bool FOGCombatRewardService::ApplyCommittedVictory(const FOGTurnBattleState& State,const FOGEntityId& Ruler,
    const FOGWorldEvent& Completion,FOGNativeBattleOutcome& Out,FString& Error)
{
    Out=FOGNativeBattleOutcome(); Error.Reset();
    if (!Store.IsWriteTransactionActive()) { Error=TEXT("Victory grants require the finishing action's active write transaction."); return false; }
    if (State.Status!=EOGTurnBattleStatus::Completed || !Ruler.IsValid() || Completion.PrimaryEntity!=Ruler ||
        !Completion.EventId.IsValid() || Completion.WorldTick<0)
    { Error=TEXT("Victory resolution requires an actual completed owned battle and durable finishing event."); return false; }
    bool Found=false; FOGWorldEvent Stored; TSharedPtr<FJsonObject> Proof; FString BattleId; double Status=-1;
    if (!Store.TryReadWorldEvent(Completion.EventId,Found,Stored,Error)) return false;
    if (!Found || Stored.PrimaryEntity!=Ruler || Stored.EventType!=Completion.EventType ||
        (Stored.EventType!=TEXT("combat.turn_action_resolved") && Stored.EventType!=TEXT("combat.encounter_started") &&
         Stored.EventType!=TEXT("combat.world_impact_resolved") && Stored.EventType!=TEXT("combat.world_clock_advanced") &&
         Stored.EventType!=TEXT("combat.world_cast_cancelled")) ||
        Stored.PayloadJson!=Completion.PayloadJson || !Json(Stored.PayloadJson,Proof) ||
        !Proof->TryGetStringField(TEXT("battle_id"),BattleId) || BattleId!=State.BattleId.ToString() ||
        !Proof->TryGetNumberField(TEXT("battle_status_after"),Status) || Status!=static_cast<uint8>(EOGTurnBattleStatus::Completed))
    { Error=TEXT("Finishing action proof does not match the durable completed battle."); return false; }
    Out.BattleId=State.BattleId; Out.CompletionEventId=Completion.EventId;
    for (const auto& Unit:State.Units)
    {
        bool Owned=Unit.UnitEntityId==Ruler;
        if (!Owned)
        {
            FOGCharacterManifestationRecord M; bool IsCopy=false;
            if (!Store.TryReadCharacterManifestation(Unit.UnitEntityId,IsCopy,M,Error)) return false;
            Owned=IsCopy && M.OwningRulerId==Ruler && M.IdentityId==Unit.IdentityId && M.ActiveVersionId==Unit.SkillSet.CurrentForm && M.LifecycleState==TEXT("active");
        }
        if (!Owned) continue;
        if (Out.bHasPlayerTeam && Out.PlayerTeamIndex!=Unit.TeamIndex)
        { Error=TEXT("Owned combatants span conflicting teams; no reward grant is inferred."); return false; }
        Out.bHasPlayerTeam=true; Out.PlayerTeamIndex=Unit.TeamIndex;
    }
    Out.bVictory=Out.bHasPlayerTeam && State.Units.ContainsByPredicate([&](const auto& U)
        { return U.TeamIndex==Out.PlayerTeamIndex && U.Presence!=EOGCombatPresence::Removed && U.IsAlive() &&
            (State.ExecutionMode!=EOGCombatExecutionMode::World || U.Presence==EOGCombatPresence::Active); });
    if (!Out.bVictory) return true;
    for (const auto& Unit:State.Units)
        if (Unit.TeamIndex!=Out.PlayerTeamIndex && Unit.Presence!=EOGCombatPresence::Removed && Unit.IsAlive() &&
            (State.ExecutionMode!=EOGCombatExecutionMode::World || Unit.Presence==EOGCombatPresence::Active))
        { Error=TEXT("A living opposing team remains; victory cannot grant rewards."); return false; }
    auto* Library=UOGDefinitionLibrary::Get();
    TArray<const FOGCombatUnitState*> Defeated;
    for (const auto& U:State.Units)
        if (U.TeamIndex!=Out.PlayerTeamIndex && U.Presence==EOGCombatPresence::Defeated && !U.IsAlive()) Defeated.Add(&U);
    Defeated.Sort([](const auto& A,const auto& B){return A.UnitEntityId.ToString()<B.UnitEntityId.ToString();});
    for (const auto* Unit:Defeated)
    {
        FName Kind; FString StateJson; int64 Revision=0; bool Exists=false;
        if (!Store.TryReadEntity(Unit->UnitEntityId,Exists,Kind,StateJson,Revision,Error)) return false;
        TSharedPtr<FJsonObject> Enemy; FString DefinitionText;
        if (!Exists) continue;
        if (!Json(StateJson,Enemy)) { Error=TEXT("Defeated enemy has invalid canonical state."); return false; }
        if (Enemy->HasField(TEXT("diagnosticFixture")))
        { Error=TEXT("Diagnostic entities cannot award canonical victory rewards."); return false; }
        if (Enemy->HasField(TEXT("combat_reward_definition_id")) &&
            !Enemy->TryGetStringField(TEXT("combat_reward_definition_id"),DefinitionText))
        { Error=TEXT("Enemy reward definition must be an exact installed ID."); return false; }
        if (Enemy->HasField(TEXT("system_binding_id")))
        {
            FString ProfileText;
            if (!Enemy->TryGetStringField(TEXT("system_binding_id"),ProfileText))
            { Error=TEXT("Enemy system binding must be an exact profile ID."); return false; }
            if (!ProfileText.IsEmpty())
            {
                const auto* Profile=Library?Cast<UOGSystemBindingDefinition>(Library->Resolve(TEXT("binding"),FOGContentId(ProfileText))):nullptr;
                if (!Profile || !Profile->ValidateDefinition(Error))
                { if (Error.IsEmpty()) Error=TEXT("Enemy system profile is unavailable."); return false; }
                FOGContentId ProfileReward;
                for (const auto& Binding:Profile->Bindings) if (Binding.Definition.System==TEXT("combat_reward"))
                {
                    if (ProfileReward.IsValid()) { Error=TEXT("Enemy profile declares conflicting reward sources."); return false; }
                    ProfileReward=Binding.Definition.Id;
                }
                if (ProfileReward.IsValid())
                {
                    if (!DefinitionText.IsEmpty() && DefinitionText!=ProfileReward.ToString())
                    { Error=TEXT("Enemy reward metadata conflicts with its canonical system profile."); return false; }
                    DefinitionText=ProfileReward.ToString();
                }
            }
        }
        if (DefinitionText.IsEmpty()) continue;
        const FOGContentId RewardDefinitionId(DefinitionText);
        const auto* D=Library?Cast<UOGCombatRewardDefinition>(Library->Resolve(TEXT("combat_reward"),RewardDefinitionId)):nullptr;
        if (!D || !D->ValidateDefinition(Error) || D->EnemyEntityKind!=Kind)
        { if (Error.IsEmpty()) Error=TEXT("Defeated enemy reward definition is missing or incompatible."); return false; }
        FOGEntityId Life=Unit->UnitEntityId;
        if (Enemy->HasField(TEXT("combat_reward_life_id")))
        {
            FString Text; FGuid Guid;
            if (!Enemy->TryGetStringField(TEXT("combat_reward_life_id"),Text) || !FGuid::Parse(Text,Guid) || !Guid.IsValid())
            { Error=TEXT("Enemy reward life identity is invalid."); return false; }
            Life=FOGEntityId(Guid);
        }
        const auto Claim=RewardId(TEXT("combat_reward_claim|")+Ruler.ToString()+TEXT("|")+Unit->UnitEntityId.ToString()+TEXT("|")+Life.ToString());
        FOGWorldEvent Prior; bool Claimed=false;
        if (!Store.TryReadWorldEvent(Claim,Claimed,Prior,Error)) return false;
        if (Claimed)
        {
            TSharedPtr<FJsonObject> PriorJson; FString EnemyId,LifeId;
            if (Prior.EventType!=TEXT("combat_reward_claimed") || Prior.PrimaryEntity!=Ruler || !Json(Prior.PayloadJson,PriorJson) ||
                !PriorJson->TryGetStringField(TEXT("enemy_id"),EnemyId) || EnemyId!=Unit->UnitEntityId.ToString() ||
                !PriorJson->TryGetStringField(TEXT("enemy_life_id"),LifeId) || LifeId!=Life.ToString())
            { Error=TEXT("Reward claim identity belongs to a conflicting event."); return false; }
            continue; // Same canonical enemy life never pays again after encounter restart.
        }
        TStrongObjectPtr<UOGObjectActionDefinition> Grant(NewObject<UOGObjectActionDefinition>());
        Grant->DefinitionId=D->DefinitionId; Grant->DisplayName=D->DisplayName; Grant->OriginalSourceReference=D->OriginalSourceReference;
        Grant->Kind=EOGObjectActionKind::Craft; Grant->bExplicitNoCost=true; Grant->Outputs=D->Objects; Grant->Resources=D->Resources;
        FOGObjectActionContext Context; Context.RulerId=Ruler; FOGObjectActionPreview Review;
        FOGObjectActionService Objects(Store);
        if (!Objects.Preview(*Grant.Get(),Context,Completion.WorldTick,Review,Error)) return false;
        bool Already=false; TArray<FOGEntityId> Created;
        const auto InventoryEvent=RewardId(Claim.ToString()+TEXT("|inventory"));
        if (!Objects.ApplyWithinTransaction(*Grant.Get(),Review,InventoryEvent,Completion.WorldTick,Already,Created,Error)) return false;
        auto Payload=MakeShared<FJsonObject>();
        Payload->SetStringField(TEXT("enemy_id"),Unit->UnitEntityId.ToString());
        Payload->SetStringField(TEXT("enemy_life_id"),Life.ToString());
        Payload->SetStringField(TEXT("battle_id"),State.BattleId.ToString());
        Payload->SetStringField(TEXT("completion_event_id"),Completion.EventId.ToString());
        Payload->SetStringField(TEXT("definition_id"),D->DefinitionId.ToString());
        Payload->SetStringField(TEXT("inventory_event_id"),InventoryEvent.ToString());
        FOGWorldEvent Award; Award.EventId=Claim; Award.EventType=TEXT("combat_reward_claimed"); Award.PrimaryEntity=Ruler;
        Award.RelatedEntities={Unit->UnitEntityId,State.BattleId,Completion.EventId,InventoryEvent};
        Award.WorldTick=Completion.WorldTick; Award.bChronicleEligible=true; Award.PayloadJson.Reset();
        FJsonSerializer::Serialize(Payload.ToSharedRef(),TJsonWriterFactory<>::Create(&Award.PayloadJson));
        if (!Store.AppendWorldEvent(Award,Error)) return false;
        Out.RewardEventIds.Add(Claim); Out.CreatedItemIds.Append(Created);
        for (const auto& Resource:D->Resources)
            Out.RewardLines.Add(Resource.DisplayName.ToString()+TEXT(" +")+LexToString(Resource.Amount));
        for (const auto& Object:D->Objects)
        {
            const auto* Spec=Cast<UOGObjectDefinition>(Library->Resolve(TEXT("object"),Object.ObjectId));
            Out.RewardLines.Add((Spec?Spec->DisplayName.ToString():FString(TEXT("Item")))+FString::Printf(TEXT(" x%d"),Object.InstanceCount));
        }
    }
    return true;
}
