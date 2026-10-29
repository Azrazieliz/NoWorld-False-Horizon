#include "Progression/OGCharacterDevelopmentService.h"

#include "Content/OGDefinitionLibrary.h"
#include "Objects/OGObjectActionDefinition.h"
#include "Dom/JsonObject.h"
#include "Misc/LexFromString.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
const FName DevelopmentEvent(TEXT("character.development_applied"));
const TCHAR* CompletedKey = TEXT("completed_development");
const TCHAR* GrantsKey = TEXT("development_stat_grants");

bool ReadState(const FString& Text, TSharedPtr<FJsonObject>& Out, FString& Error)
{
    if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Out) && Out.IsValid())
        return true;
    Error = TEXT("Character reinforcement state is unreadable; no resources were consumed.");
    return false;
}

FString WriteState(const TSharedRef<FJsonObject>& State)
{
    FString Text;
    FJsonSerializer::Serialize(State, TJsonWriterFactory<>::Create(&Text));
    return Text;
}

bool ReadCompleted(const TSharedPtr<FJsonObject>& State, TArray<FString>& Out, FString& Error)
{
    Out.Reset();
    if (!State->HasField(CompletedKey)) return true;
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!State->TryGetArrayField(CompletedKey, Values))
    { Error = TEXT("Development completion history is malformed."); return false; }
    for (const auto& V : *Values)
    {
        FString Id;
        if (!V.IsValid() || !V->TryGetString(Id) || !FOGContentId(Id).IsValid() || Out.Contains(Id))
        { Error = TEXT("Development completion history contains an invalid or duplicate ID."); return false; }
        Out.Add(Id);
    }
    return true;
}

bool ReadGrants(const TSharedPtr<FJsonObject>& State,
    TMap<FName, FOGLargeNumber>& Out, FString& Error)
{
    Out.Reset();
    if (!State->HasField(GrantsKey)) return true;
    const TSharedPtr<FJsonObject>* Object = nullptr;
    if (!State->TryGetObjectField(GrantsKey, Object) || !Object || !Object->IsValid())
    { Error = TEXT("Earned development stats are malformed."); return false; }
    for (const auto& Pair : (*Object)->Values)
    {
        const TSharedPtr<FJsonObject>* Value = nullptr;
        FString Digits;
        int32 Exponent = 0;
        int64 Significand = 0;
        if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(Value) || !Value ||
            !(*Value)->TryGetStringField(TEXT("significand"), Digits) ||
            !LexTryParseString(Significand, *Digits) ||
            !(*Value)->TryGetNumberField(TEXT("exponent10"), Exponent) ||
            Significand < 0 || Significand > FOGLargeNumber::MaxSignificand ||
            Exponent < MIN_int32 / 4 || Exponent > MAX_int32 / 4)
        { Error = TEXT("Earned stat grant contains an invalid large number."); return false; }
        Out.Add(FName(*Pair.Key), FOGLargeNumber(Significand, Exponent));
    }
    return true;
}

TSharedRef<FJsonObject> WriteGrants(const TMap<FName, FOGLargeNumber>& Grants)
{
    auto Object = MakeShared<FJsonObject>();
    TArray<FName> Names;
    Grants.GetKeys(Names);
    Names.Sort(FNameLexicalLess());
    for (FName Name : Names)
    {
        const auto& Number = Grants[Name];
        auto Value = MakeShared<FJsonObject>();
        Value->SetStringField(TEXT("significand"), LexToString(Number.Significand));
        Value->SetNumberField(TEXT("exponent10"), Number.Exponent10);
        Object->SetObjectField(Name.ToString(), Value);
    }
    return Object;
}

// Length-prefix every field: content labels and arbitrary JSON cannot create
// ambiguous confirmation payloads by containing separator characters.
void FingerprintPart(FString& Text, const FString& Part)
{
    Text += FString::Printf(TEXT("%d:"), Part.Len()) + Part;
}

FString DefinitionFingerprint(const FOGCharacterDevelopmentDefinition& D)
{
    FString Text;
    for (const FString& Part : {D.DevelopmentId.ToString(), D.IdentityId.ToString(),
        D.VersionId.ToString(), D.DisplayName, D.Description, LexToString(D.MinimumLevel),
        D.RequiredRarity.ToString(), D.RequiredReinforcementState.ToString(),
        LexToString(D.ResultLevel), D.ResultRarity.ToString(),
        D.ResultReinforcementState.ToString(), D.bCompletesReinforcement ? FString(TEXT("1")) : FString(TEXT("0"))})
        FingerprintPart(Text, Part);
    for (const auto& Id : D.PrerequisiteDevelopmentIds) FingerprintPart(Text, TEXT("required:") + Id.ToString());
    for (const auto& Id : D.ExcludedDevelopmentIds) FingerprintPart(Text, TEXT("excluded:") + Id.ToString());
    auto Routes = [&Text](const TArray<FOGDevelopmentRouteRequirement>& Values, const TCHAR* Kind)
    {
        for (const auto& R : Values)
        {
            FingerprintPart(Text, Kind);
            FingerprintPart(Text, R.RouteId.ToString());
            FingerprintPart(Text, R.NodeId.ToString());
            FingerprintPart(Text, R.State.ToString());
        }
    };
    Routes(D.RequiredRoutes, TEXT("required_route"));
    Routes(D.GrantedRoutes, TEXT("granted_route"));
    for (const auto& R : D.RequiredFacts)
    {
        FingerprintPart(Text, R.FactKey.ToString());
        FingerprintPart(Text, R.DisplayName);
        FingerprintPart(Text, R.bSubjectIsRuler ? TEXT("ruler") : TEXT("manifestation"));
    }
    for (const auto& Cost : D.Costs)
    {
        FingerprintPart(Text, Cost.ResourceId.ToString());
        FingerprintPart(Text, Cost.DisplayName);
        FingerprintPart(Text, LexToString(Cost.Amount));
    }
    for (const auto& Cost : D.ItemCosts)
    {
        FingerprintPart(Text, TEXT("item_cost"));
        FingerprintPart(Text, Cost.ObjectId.ToString());
        FingerprintPart(Text, Cost.RankId.ToString());
        FingerprintPart(Text, Cost.QualityId.ToString());
        FingerprintPart(Text, LexToString(Cost.InstanceCount));
    }
    FingerprintPart(Text, WriteState(WriteGrants(D.StatGrants)));
    for (const auto& Form : D.GrantedForms) FingerprintPart(Text, Form.ToString());
    return Text;
}

FString Digest(const FString& Text)
{
    FTCHARToUTF8 Utf8(*Text);
    FMD5 Md5;
    Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
    uint8 Bytes[16];
    Md5.Final(Bytes);
    return BytesToHex(Bytes, 16);
}

// Follow the inventory/object-action ownership model: the Ruler and its
// genuine Manifestations share available inputs; equipped instances stay put.
bool PreviewItemCosts(IOGWorldStore& Store, const FOGEntityId& RulerId,
    const TArray<FOGDevelopmentItemCost>& Costs, FOGCharacterDevelopmentPreview& Out,
    FString& Token, FString& Error)
{
    if (Costs.IsEmpty()) return true;
    TArray<FOGEntityId> Owners = {RulerId};
    TArray<FOGCharacterManifestationRecord> Copies;
    if (!Store.ListCharacterManifestationsByOwner(RulerId, Copies, Error)) return false;
    for (const auto& Copy : Copies)
        if (Copy.OwningRulerId == RulerId) Owners.AddUnique(Copy.ManifestationId);
    TArray<FOGItemInstanceRecord> Items;
    TSet<FOGEntityId> Seen, Equipped;
    for (const auto& Owner : Owners)
    {
        TArray<FOGItemInstanceRecord> Owned;
        TArray<FOGEquipmentBindingRecord> Bindings;
        if (!Store.ListItemInstancesByOwner(Owner, Owned, Error) ||
            !Store.ListEquipmentBindings(Owner, Bindings, Error)) return false;
        for (const auto& Item : Owned)
        {
            if (!Item.ItemId.IsValid() || Item.OwnerEntityId != Owner || Seen.Contains(Item.ItemId))
            { Error = TEXT("Development inventory has inconsistent ownership."); return false; }
            Seen.Add(Item.ItemId);
            Items.Add(Item);
        }
        for (const auto& Binding : Bindings) Equipped.Add(Binding.ItemId);
    }
    Items.Sort([](const auto& A, const auto& B)
        { return A.ItemId.ToString() < B.ItemId.ToString(); });
    auto* Library = UOGDefinitionLibrary::Get();
    for (const auto& Cost : Costs)
    {
        FOGDevelopmentItemCostPreview P;
        P.Cost = Cost;
        P.DisplayName = Cost.ObjectId.ToString();
        const auto* Definition = Library
            ? Cast<UOGObjectDefinition>(Library->Resolve(TEXT("object"), Cost.ObjectId)) : nullptr;
        FString DefinitionError;
        if (!Definition || !Definition->ValidateDefinition(DefinitionError))
        {
            Out.MissingRequirements.Add(TEXT("Required item definition unavailable: ") +
                Cost.ObjectId.ToString() + (DefinitionError.IsEmpty() ? FString() : TEXT(" / ") + DefinitionError));
            Out.ItemCosts.Add(MoveTemp(P));
            continue;
        }
        P.DisplayName = Definition->DisplayName.ToString();
        FingerprintPart(Token, P.DisplayName);
        // Exact selectors and catalog uniqueness keep one instance from paying
        // two input rows. Never substitute a rank, quality, or resource balance.
        for (const auto& Item : Items)
        {
            if (Item.DefinitionId != Cost.ObjectId || Item.CurrentRankId != Cost.RankId ||
                Item.QualityId != Cost.QualityId || Equipped.Contains(Item.ItemId)) continue;
            ++P.AvailableInstances;
            if (P.SelectedInstances.Num() < Cost.InstanceCount) P.SelectedInstances.Add(Item);
        }
        FingerprintPart(Token, LexToString(P.AvailableInstances));
        if (P.AvailableInstances < Cost.InstanceCount)
            Out.MissingRequirements.Add(FString::Printf(TEXT("%s: need %d unequipped instances, have %d (exact rank/quality)."),
                *P.DisplayName, Cost.InstanceCount, P.AvailableInstances));
        for (const auto& Item : P.SelectedInstances)
        {
            TSharedPtr<FJsonObject> History;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Item.HistoryStateJson), History) || !History.IsValid())
            { Error = TEXT("An input item's provenance is unreadable; no assets were consumed."); return false; }
            bool Found = false;
            FName Kind;
            FString State;
            int64 Revision = 0;
            if (!Store.TryReadEntity(Item.ItemId, Found, Kind, State, Revision, Error)) return false;
            if (!Found || Kind != FName(TEXT("item")))
            { Error = TEXT("An input is no longer a canonical item instance."); return false; }
            for (const auto& Part : {Item.ItemId.ToString(), Item.OwnerEntityId.ToString(),
                Item.DefinitionId.ToString(), Item.CurrentRankId.ToString(), Item.QualityId.ToString(),
                Item.DurabilityStateJson, Item.EvolutionStateJson, Item.HistoryStateJson, State, LexToString(Revision)})
                FingerprintPart(Token, Part);
            TArray<FOGItemModifierRecord> Modifiers;
            if (!Store.ListItemModifiers(Item.ItemId, Modifiers, Error)) return false;
            for (const auto& Modifier : Modifiers)
            {
                FingerprintPart(Token, Modifier.ModifierId.ToString());
                FingerprintPart(Token, LexToString(Modifier.Ordinal));
                FingerprintPart(Token, Modifier.StateJson);
            }
        }
        Out.ItemCosts.Add(MoveTemp(P));
    }
    return true;
}

bool AddStatGrants(FOGCombatStats& Stats, const TMap<FName, FOGLargeNumber>& Grants, FString& Error)
{
    FOGCombatStats Candidate = Stats;
    for (const auto& Pair : Grants)
    {
        if (Pair.Value.GetSign() < 0)
        { Error = TEXT("Earned reinforcement contains a negative stat grant."); return false; }
        if (Pair.Key == TEXT("max_hp")) Candidate.MaxHp = FOGLargeNumber::Add(Candidate.MaxHp, Pair.Value);
        else if (Pair.Key == TEXT("attack")) Candidate.Attack = FOGLargeNumber::Add(Candidate.Attack, Pair.Value);
        else if (Pair.Key == TEXT("defense")) Candidate.Defense = FOGLargeNumber::Add(Candidate.Defense, Pair.Value);
        else
        {
            int32* Field = nullptr;
#define OG_DEV_STAT(Key, Member) if (Pair.Key == TEXT(Key)) Field = &Candidate.Member
            OG_DEV_STAT("speed", Speed);
            else OG_DEV_STAT("crit_rate_bps", CritRateBps);
            else OG_DEV_STAT("crit_damage_bonus_bps", CritDamageBonusBps);
            else OG_DEV_STAT("crit_rate_resistance_bps", CritRateResistanceBps);
            else OG_DEV_STAT("crit_damage_resistance_bps", CritDamageResistanceBps);
            else OG_DEV_STAT("hit_bps", HitBps);
            else OG_DEV_STAT("dodge_bps", DodgeBps);
            else OG_DEV_STAT("block_rate_bps", BlockRateBps);
            else OG_DEV_STAT("block_reduction_bps", BlockReductionBps);
            else OG_DEV_STAT("defense_penetration_bps", DefensePenetrationBps);
            else OG_DEV_STAT("damage_dealt_multiplier_bps", DamageDealtMultiplierBps);
            else OG_DEV_STAT("damage_taken_multiplier_bps", DamageTakenMultiplierBps);
            else OG_DEV_STAT("true_damage_resistance_bps", TrueDamageResistanceBps);
#undef OG_DEV_STAT
            if (!Field || FOGLargeNumber::Compare(Pair.Value, FOGLargeNumber::FromInt64(MAX_int32)) > 0)
            { Error = TEXT("Earned reinforcement stat is unknown or exceeds its integer range."); return false; }
            int64 Value = Pair.Value.Significand;
            int32 Exponent = Pair.Value.Exponent10;
            if (Value == 0) Exponent = 0;
            while (Exponent < 0 && Value % 10 == 0) { Value /= 10; ++Exponent; }
            if (Exponent < 0 || Exponent > 9)
            { Error = TEXT("Reinforcement integer stat cannot contain a fractional value."); return false; }
            while (Exponent-- > 0) Value *= 10;
            if (Value > MAX_int32 - static_cast<int64>(*Field))
            { Error = TEXT("Reinforcement would overflow a combat stat."); return false; }
            *Field += static_cast<int32>(Value);
        }
    }
    Stats = Candidate;
    return true;
}
}

bool FOGCharacterDevelopmentService::Preview(const FOGEntityId& RulerId,
    const FOGEntityId& ManifestationId, const FOGCharacterDevelopmentDefinition& D,
    FOGCharacterDevelopmentPreview& Out, FString& Error) const
{
    Out = FOGCharacterDevelopmentPreview();
    Error.Reset();
    if (!FOGCharacterDevelopmentCatalog::ValidateDefinition(D, Error)) return false;
    // Catalog validation also applies to native callers. Prerequisite graph
    // links are validated at catalog admission; each row is validated here.
    if (!Store.IsOpen() || !RulerId.IsValid() || !ManifestationId.IsValid() ||
        !D.DevelopmentId.IsValid() || !D.IdentityId.IsValid() || !D.VersionId.IsValid())
    { Error = TEXT("A valid owned character and installed development choice are required."); return false; }
    bool OwnerFound = false;
    FName OwnerKind;
    FString OwnerState;
    int64 OwnerRevision = 0;
    if (!Store.TryReadEntity(RulerId, OwnerFound, OwnerKind, OwnerState, OwnerRevision, Error)) return false;
    if (!OwnerFound || OwnerKind != FName(TEXT("ruler")) ||
        OwnerState.Contains(TEXT("\"diagnosticFixture\"")))
    { Error = TEXT("Development requires a real stored Ruler."); return false; }
    bool Found = false;
    FOGCharacterManifestationRecord M;
    if (!Store.TryReadCharacterManifestation(ManifestationId, Found, M, Error)) return false;
    if (!Found || M.OwningRulerId != RulerId || M.IdentityId != D.IdentityId ||
        M.ActiveVersionId != D.VersionId || M.LifecycleState != FName(TEXT("active")))
    { Error = TEXT("Development choice does not belong to this active owned Manifestation."); return false; }
    FOGManifestationReinforcementRecord R;
    if (!Store.TryReadManifestationReinforcement(ManifestationId, Found, R, Error)) return false;
    if (!Found) R.ManifestationId = ManifestationId;
    TSharedPtr<FJsonObject> State;
    TArray<FString> Completed;
    if (!ReadState(R.StateJson, State, Error) || !ReadCompleted(State, Completed, Error)) return false;

    Out.DevelopmentId = D.DevelopmentId;
    Out.ManifestationId = ManifestationId;
    Out.DisplayName = D.DisplayName;
    Out.Description = D.Description;
    Out.Current = M;
    Out.Result = M;
    Out.CurrentReinforcementState = R.ReinforcementState;
    Out.ResultReinforcementState = D.ResultReinforcementState.IsNone() ? R.ReinforcementState : D.ResultReinforcementState;
    Out.bCurrentlyMaxReinforced = R.bMaxReinforced;
    Out.bResultMaxReinforced = R.bMaxReinforced || D.bCompletesReinforcement;
    Out.StatGrants = D.StatGrants;
    Out.GrantedRoutes = D.GrantedRoutes;
    Out.GrantedForms = D.GrantedForms;
    Out.bAlreadyCompleted = Completed.Contains(D.DevelopmentId.ToString());
    if (Out.bAlreadyCompleted) Out.MissingRequirements.Add(TEXT("This development is already completed by this copy."));
    if (M.Level < D.MinimumLevel)
        Out.MissingRequirements.Add(FString::Printf(TEXT("Requires level %d."), D.MinimumLevel));
    if (!D.RequiredRarity.IsNone() && M.CurrentRarity != D.RequiredRarity)
        Out.MissingRequirements.Add(TEXT("Requires rarity ") + D.RequiredRarity.ToString());
    if (!D.RequiredReinforcementState.IsNone() && R.ReinforcementState != D.RequiredReinforcementState)
        Out.MissingRequirements.Add(TEXT("Requires development stage ") + D.RequiredReinforcementState.ToString());
    if (D.ResultLevel != 0)
    {
        // A lower authored level floor cannot regress an already developed
        // copy, nor make every old tier break the entire options screen.
        Out.Result.Level = FMath::Max(M.Level, D.ResultLevel);
    }
    if (!D.ResultRarity.IsNone())
    {
        const TArray<FName> Rarities = {TEXT("R"), TEXT("SR"), TEXT("SSR"), TEXT("UR"), TEXT("LR"), TEXT("MR")};
        const int32 From = Rarities.IndexOfByKey(M.CurrentRarity);
        const int32 To = Rarities.IndexOfByKey(D.ResultRarity);
        if (D.ResultRarity == M.CurrentRarity || (From != INDEX_NONE && To >= From))
            Out.Result.CurrentRarity = D.ResultRarity;
        else Out.MissingRequirements.Add(TEXT("Current Rarity already exceeds or differs from this authored promotion."));
    }
    for (const auto& Id : D.PrerequisiteDevelopmentIds)
        if (!Completed.Contains(Id.ToString())) Out.MissingRequirements.Add(TEXT("Complete ") + Id.ToString());
    for (const auto& Id : D.ExcludedDevelopmentIds)
        if (Completed.Contains(Id.ToString())) Out.MissingRequirements.Add(TEXT("Incompatible with completed choice ") + Id.ToString());
    TArray<FOGManifestationRouteNodeRecord> Routes;
    if (!D.RequiredRoutes.IsEmpty() && !Store.ListManifestationRouteNodes(ManifestationId, Routes, Error)) return false;
    for (const auto& Required : D.RequiredRoutes)
        if (!Routes.ContainsByPredicate([&Required](const FOGManifestationRouteNodeRecord& Node)
            { return Node.RouteId == Required.RouteId && Node.NodeId == Required.NodeId && Node.State == Required.State; }))
            Out.MissingRequirements.Add(TEXT("Requires route ") + Required.NodeId.ToString() + TEXT(" / ") + Required.State.ToString());
    for (const auto& Required : D.RequiredFacts)
    {
        TArray<FOGWorldEvent> Events;
        if (!Store.ListWorldEvents(Required.bSubjectIsRuler ? RulerId : ManifestationId,
            Required.FactKey, false, 1, Events, Error)) return false;
        if (Events.IsEmpty()) Out.MissingRequirements.Add(Required.DisplayName.IsEmpty()
            ? Required.FactKey.ToString() : Required.DisplayName);
    }
    FString TokenInput = DefinitionFingerprint(D);
    FingerprintPart(TokenInput, OwnerState);
    FingerprintPart(TokenInput, LexToString(OwnerRevision));
    if (!PreviewItemCosts(Store, RulerId, D.ItemCosts, Out, TokenInput, Error)) return false;
    for (const auto& Cost : D.Costs)
    {
        FOGDevelopmentCostPreview P;
        P.Cost = Cost;
        if (!Cost.ResourceId.IsValid() || Cost.Amount <= 0)
        { Error = TEXT("Development contains an invalid resource cost."); return false; }
        if (!Store.TryReadResourceBalance(RulerId, Cost.ResourceId, Found, P.Balance, Error)) return false;
        if (!Found) P.Balance = 0;
        if (P.Balance < Cost.Amount) Out.MissingRequirements.Add(FString::Printf(TEXT("%s: need %lld, have %lld"),
            Cost.DisplayName.IsEmpty() ? *Cost.ResourceId.ToString() : *Cost.DisplayName, Cost.Amount, P.Balance));
        FingerprintPart(TokenInput, LexToString(P.Balance));
        Out.Costs.Add(P);
    }
    // Grants overwrite route/form state. Capture those destination records too,
    // so confirmation cannot silently overwrite progression earned after review.
    if (!D.GrantedRoutes.IsEmpty() && D.RequiredRoutes.IsEmpty() &&
        !Store.ListManifestationRouteNodes(ManifestationId, Routes, Error)) return false;
    for (const auto& Grant : D.GrantedRoutes)
    {
        const auto* Previous = Routes.FindByPredicate([&](const auto& Node)
            { return Node.RouteId == Grant.RouteId && Node.NodeId == Grant.NodeId; });
        FingerprintPart(TokenInput, Previous ? TEXT("route_present") : TEXT("route_absent"));
        if (Previous)
        {
            FingerprintPart(TokenInput, Previous->State.ToString());
            FingerprintPart(TokenInput, Previous->StateJson);
            FingerprintPart(TokenInput, Previous->bHasEnteredWorldTick ? LexToString(Previous->EnteredWorldTick) : TEXT("unset"));
            FingerprintPart(TokenInput, Previous->bHasCompletedWorldTick ? LexToString(Previous->CompletedWorldTick) : TEXT("unset"));
        }
    }
    if (!D.GrantedForms.IsEmpty())
    {
        TArray<FOGManifestationFormRecord> Forms;
        if (!Store.ListManifestationForms(ManifestationId, Forms, Error)) return false;
        for (const auto& Id : D.GrantedForms)
        {
            const auto* Previous = Forms.FindByPredicate([&](const auto& Form) { return Form.FormId == Id; });
            FingerprintPart(TokenInput, Previous ? TEXT("form_present") : TEXT("form_absent"));
            if (Previous)
            {
                FingerprintPart(TokenInput, Previous->State.ToString());
                FingerprintPart(TokenInput, Previous->StateJson);
                FingerprintPart(TokenInput, LexToString(Previous->UnlockedWorldTick));
            }
        }
    }
    TMap<FName, FOGLargeNumber> Grants;
    if (!ReadGrants(State, Grants, Error)) return false;
    Out.CurrentEarnedStats = Grants;
    for (const auto& Grant : D.StatGrants)
        Grants.FindOrAdd(Grant.Key) = FOGLargeNumber::Add(Grants.FindRef(Grant.Key), Grant.Value);
    Out.ResultEarnedStats = Grants;
    FOGCombatStats ValidateStats;
    if (!AddStatGrants(ValidateStats, Grants, Error)) return false;
    FingerprintPart(TokenInput, RulerId.ToString());
    FingerprintPart(TokenInput, ManifestationId.ToString());
    FingerprintPart(TokenInput, LexToString(M.Level));
    FingerprintPart(TokenInput, M.CurrentRarity.ToString());
    FingerprintPart(TokenInput, M.ProgressionStateJson);
    FingerprintPart(TokenInput, R.StateJson);
    FingerprintPart(TokenInput, R.ReinforcementState.ToString());
    FingerprintPart(TokenInput, LexToString(R.UpdatedWorldTick));
    FingerprintPart(TokenInput, R.bMaxReinforced ? TEXT("max") : TEXT("developing"));
    FOGEntityRankStateRecord Rank;
    bool HasRank = false;
    if (!Store.TryReadEntityRankState(ManifestationId, HasRank, Rank, Error)) return false;
    FingerprintPart(TokenInput, HasRank ? TEXT("rank_present") : TEXT("rank_absent"));
    if (HasRank)
    {
        for (const FString& Part : {Rank.AttainedRankId.ToString(), LexToString(Rank.AttainedLevel),
            Rank.EffectiveRankId.ToString(), LexToString(Rank.EffectiveLevel),
            Rank.PeakRankId.ToString(), LexToString(Rank.PeakLevel),
            LexToString(Rank.UpdatedWorldTick), Rank.StateJson})
            FingerprintPart(TokenInput, Part);
    }
    Out.ConfirmationToken = Digest(TokenInput);
    Out.bCanApply = Out.MissingRequirements.IsEmpty();
    return true;
}

bool FOGCharacterDevelopmentService::Apply(const FOGEntityId& RulerId,
    const FOGEntityId& ManifestationId, const FOGCharacterDevelopmentDefinition& D,
    const FString& Token, const FOGEntityId& RequestId, int64 Tick,
    bool& bOutAlreadyApplied, FString& Error)
{
    bOutAlreadyApplied = false;
    Error.Reset();
    if (!IsInGameThread() || !RequestId.IsValid() || Token.IsEmpty() || Tick < 0)
    { Error = TEXT("An explicit reviewed development request is required."); return false; }
    if (!Store.BeginTransaction(Error)) return false;
    auto Fail = [&]() { FString RollbackError; Store.RollbackTransaction(RollbackError); return false; };
    bool Found = false;
    FOGWorldEvent Existing;
    if (!Store.TryReadWorldEvent(RequestId, Found, Existing, Error)) return Fail();
    if (Found)
    {
        TSharedPtr<FJsonObject> Receipt;
        FString Owner, Choice, Confirmed;
        if (!ReadState(Existing.PayloadJson, Receipt, Error) || Existing.EventType != DevelopmentEvent ||
            Existing.PrimaryEntity != ManifestationId ||
            !Receipt->TryGetStringField(TEXT("ruler_id"), Owner) || Owner != RulerId.ToString() ||
            !Receipt->TryGetStringField(TEXT("development_id"), Choice) || Choice != D.DevelopmentId.ToString() ||
            !Receipt->TryGetStringField(TEXT("confirmation_token"), Confirmed) || Confirmed != Token)
        { Error = TEXT("This request ID already belongs to a different canonical action."); return Fail(); }
        if (!Store.CommitTransaction(Error)) return Fail();
        bOutAlreadyApplied = true;
        return true;
    }
    FOGCharacterDevelopmentPreview Quote;
    if (!Preview(RulerId, ManifestationId, D, Quote, Error)) return Fail();
    if (!Quote.bCanApply || Quote.ConfirmationToken != Token)
    { Error = Quote.bCanApply ? TEXT("Character, costs or content changed. Review the upgrade again.") : FString::Join(Quote.MissingRequirements, TEXT("\n")); return Fail(); }
    for (const auto& Cost : Quote.Costs)
        if (!Store.SetResourceBalance(RulerId, Cost.Cost.ResourceId, Cost.Balance - Cost.Cost.Amount, Error)) return Fail();
    TArray<TSharedPtr<FJsonValue>> ConsumedItems;
    for (const auto& Cost : Quote.ItemCosts)
        for (const auto& Selected : Cost.SelectedInstances)
        {
            auto Item = Selected;
            TSharedPtr<FJsonObject> History;
            if (!ReadState(Item.HistoryStateJson, History, Error)) return Fail();
            History->SetStringField(TEXT("consumed_by_development_event"), RequestId.ToString());
            Item.HistoryStateJson = WriteState(History.ToSharedRef());
            Item.OwnerEntityId = FOGEntityId();
            if (!Store.DeleteContainerContentsForItem(Item.ItemId, Error) ||
                !Store.DeleteEquipmentBindingsForItem(Item.ItemId, Error) ||
                !Store.UpsertItemInstance(Item, Tick, Error)) return Fail();
            auto Change = MakeShared<FJsonObject>();
            Change->SetStringField(TEXT("item_id"), Item.ItemId.ToString());
            Change->SetStringField(TEXT("object_id"), Item.DefinitionId.ToString());
            Change->SetStringField(TEXT("rank_id"), Item.CurrentRankId.ToString());
            Change->SetStringField(TEXT("quality_id"), Item.QualityId.ToString());
            Change->SetStringField(TEXT("owner_before"), Selected.OwnerEntityId.ToString());
            ConsumedItems.Add(MakeShared<FJsonValueObject>(Change));
        }
    FOGManifestationReinforcementRecord R;
    if (!Store.TryReadManifestationReinforcement(ManifestationId, Found, R, Error)) return Fail();
    if (Found && Tick < R.UpdatedWorldTick)
    { Error = TEXT("Development cannot precede the saved progression history."); return Fail(); }
    R.ManifestationId = ManifestationId;
    TSharedPtr<FJsonObject> State;
    TArray<FString> Completed;
    TMap<FName, FOGLargeNumber> Grants;
    if (!ReadState(R.StateJson, State, Error) || !ReadCompleted(State, Completed, Error) || !ReadGrants(State, Grants, Error)) return Fail();
    Completed.Add(D.DevelopmentId.ToString());
    TArray<TSharedPtr<FJsonValue>> CompletedJson;
    for (const auto& Id : Completed) CompletedJson.Add(MakeShared<FJsonValueString>(Id));
    State->SetArrayField(CompletedKey, CompletedJson);
    for (const auto& Grant : D.StatGrants)
        Grants.FindOrAdd(Grant.Key) = FOGLargeNumber::Add(Grants.FindRef(Grant.Key), Grant.Value);
    State->SetObjectField(GrantsKey, WriteGrants(Grants));
    State->SetStringField(TEXT("last_development_request"), RequestId.ToString());
    R.StateJson = WriteState(State.ToSharedRef());
    R.ReinforcementState = Quote.ResultReinforcementState;
    R.bMaxReinforced = Quote.bResultMaxReinforced;
    R.UpdatedWorldTick = Tick;
    if (!Store.UpsertCharacterManifestation(Quote.Result, Tick, Error) ||
        !Store.UpsertManifestationReinforcement(R, Error)) return Fail();
    if (Quote.Result.Level != Quote.Current.Level)
    {
        FOGEntityRankStateRecord Rank;
        if (!Store.TryReadEntityRankState(ManifestationId, Found, Rank, Error)) return Fail();
        if (Found)
        {
            Rank.AttainedLevel = FMath::Max(Rank.AttainedLevel, Quote.Result.Level);
            if (Rank.PeakRankId == Rank.AttainedRankId) Rank.PeakLevel = FMath::Max(Rank.PeakLevel, Rank.AttainedLevel);
            Rank.UpdatedWorldTick = Tick;
            // Effective suppression/seals are deliberately preserved.
            if (!Store.UpsertEntityRankState(Rank, Error)) return Fail();
        }
    }
    for (const auto& Grant : D.GrantedRoutes)
    {
        TArray<FOGManifestationRouteNodeRecord> ExistingRoutes;
        if (!Store.ListManifestationRouteNodes(ManifestationId, ExistingRoutes, Error)) return Fail();
        const auto* Previous = ExistingRoutes.FindByPredicate([&Grant](const FOGManifestationRouteNodeRecord& Node)
            { return Node.RouteId == Grant.RouteId && Node.NodeId == Grant.NodeId; });
        FOGManifestationRouteNodeRecord Node = Previous ? *Previous : FOGManifestationRouteNodeRecord();
        Node.ManifestationId = ManifestationId;
        Node.RouteId = Grant.RouteId;
        Node.NodeId = Grant.NodeId;
        Node.State = Grant.State;
        if (!Node.bHasEnteredWorldTick) { Node.bHasEnteredWorldTick = true; Node.EnteredWorldTick = Tick; }
        if (Grant.State == TEXT("completed")) { Node.bHasCompletedWorldTick = true; Node.CompletedWorldTick = Tick; }
        if (!Store.UpsertManifestationRouteNode(Node, Error)) return Fail();
    }
    for (const auto& Id : D.GrantedForms)
    {
        TArray<FOGManifestationFormRecord> Forms;
        if (!Store.ListManifestationForms(ManifestationId, Forms, Error)) return Fail();
        const auto* Previous = Forms.FindByPredicate([&Id](const FOGManifestationFormRecord& F) { return F.FormId == Id; });
        if (Previous && Previous->State == FName(TEXT("unlocked"))) continue;
        FOGManifestationFormRecord Form = Previous ? *Previous : FOGManifestationFormRecord();
        Form.ManifestationId = ManifestationId;
        Form.FormId = Id;
        Form.State = TEXT("unlocked");
        Form.UnlockedWorldTick = Tick;
        if (!Store.UpsertManifestationForm(Form, Error)) return Fail();
    }
    auto Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("ruler_id"), RulerId.ToString());
    Payload->SetStringField(TEXT("development_id"), D.DevelopmentId.ToString());
    Payload->SetStringField(TEXT("confirmation_token"), Token);
    Payload->SetNumberField(TEXT("previous_level"), Quote.Current.Level);
    Payload->SetNumberField(TEXT("result_level"), Quote.Result.Level);
    Payload->SetStringField(TEXT("previous_rarity"), Quote.Current.CurrentRarity.ToString());
    Payload->SetStringField(TEXT("result_rarity"), Quote.Result.CurrentRarity.ToString());
    Payload->SetObjectField(TEXT("stat_grants"), WriteGrants(D.StatGrants));
    TArray<TSharedPtr<FJsonValue>> Costs;
    for (const auto& C : Quote.Costs)
    {
        auto Cost = MakeShared<FJsonObject>();
        Cost->SetStringField(TEXT("resource_id"), C.Cost.ResourceId.ToString());
        Cost->SetStringField(TEXT("amount"), LexToString(C.Cost.Amount));
        Cost->SetStringField(TEXT("balance_before"), LexToString(C.Balance));
        Cost->SetStringField(TEXT("balance_after"), LexToString(C.Balance - C.Cost.Amount));
        Costs.Add(MakeShared<FJsonValueObject>(Cost));
    }
    Payload->SetArrayField(TEXT("costs"), Costs);
    Payload->SetArrayField(TEXT("consumed_items"), ConsumedItems);
    FOGWorldEvent Event;
    Event.EventId = RequestId;
    Event.EventType = DevelopmentEvent;
    Event.PrimaryEntity = ManifestationId;
    Event.RelatedEntities.Add(RulerId);
    for (const auto& Cost : Quote.ItemCosts)
        for (const auto& Item : Cost.SelectedInstances) Event.RelatedEntities.AddUnique(Item.ItemId);
    Event.WorldTick = Tick;
    Event.bChronicleEligible = true;
    Event.PayloadJson = WriteState(Payload);
    if (!Store.AppendWorldEvent(Event, Error) || !Store.CommitTransaction(Error)) return Fail();
    return true;
}

bool FOGCharacterDevelopmentService::ApplyEarnedStats(const FOGEntityId& ManifestationId,
    FOGCombatStats& InOutStats, FString& Error) const
{
    Error.Reset();
    bool Found = false;
    FOGManifestationReinforcementRecord R;
    if (!Store.TryReadManifestationReinforcement(ManifestationId, Found, R, Error)) return false;
    if (!Found) return true;
    TSharedPtr<FJsonObject> State;
    TMap<FName, FOGLargeNumber> Grants;
    return ReadState(R.StateJson, State, Error) && ReadGrants(State, Grants, Error) &&
        AddStatGrants(InOutStats, Grants, Error);
}

bool FOGCharacterDevelopmentService::ProjectCombatUnit(FOGCombatUnitState& Unit, FString& Error) const
{
    bool Found = false;
    FOGCharacterManifestationRecord M;
    if (!Store.TryReadCharacterManifestation(Unit.UnitEntityId, Found, M, Error)) return false;
    if (!Found) return true; // protagonist/NPC stats come from their own rules
    if (M.IdentityId != Unit.IdentityId || M.ActiveVersionId != Unit.SkillSet.CurrentForm ||
        M.LifecycleState != FName(TEXT("active")))
    { Error = TEXT("Combat snapshot does not match the current active Manifestation."); return false; }
    const FOGCombatStats Base = Unit.bHasDevelopmentStatProjection ? Unit.DevelopmentBaseStats : Unit.Stats;
    FOGCombatStats Developed = Base;
    if (!ApplyEarnedStats(Unit.UnitEntityId, Developed, Error)) return false;
    FOGEntityRankStateRecord Rank;
    if (!Store.TryReadEntityRankState(Unit.UnitEntityId, Found, Rank, Error)) return false;
    Unit.DevelopmentBaseStats = Base;
    Unit.bHasDevelopmentStatProjection = true;
    Unit.Stats = Developed;
    if (Found)
    {
        Unit.RankProjection.bUsingEffectiveOverride = Rank.EffectiveRankId.IsValid();
        Unit.RankProjection.RankId = Unit.RankProjection.bUsingEffectiveOverride ? Rank.EffectiveRankId : Rank.AttainedRankId;
        Unit.RankProjection.Level = Unit.RankProjection.bUsingEffectiveOverride ? Rank.EffectiveLevel : Rank.AttainedLevel;
    }
    // HP is owned by the combat/healing executor, including authored overflow
    // HP. A stat refresh neither heals wounds nor removes valid overflow.
    return true;
}
