#include "Content/OGDefinitionLibrary.h"
#include "Progression/OGCharacterDevelopmentService.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <initializer_list>

namespace
{
using FObject = TSharedPtr<FJsonObject>;
using FArray = TArray<TSharedPtr<FJsonValue>>;

// docs/EXISTENCE_POWER_RANK.md, section 2: each Rank has Levels 1-100.
// Development stays inside the current rank; breakthroughs use the rank engine.
constexpr int32 MaxRankLevel = 100;

// Leave room for normalization, exponent subtraction and multiplication in
// FOGLargeNumber. These are representation bounds, not progression maxima.
constexpr int32 MinGrantExponent = MIN_int32 / 4;
constexpr int32 MaxGrantExponent = MAX_int32 / 4;

bool Fail(FString& Error, const FString& Path, const FString& Message)
{
    Error = Path + TEXT(": ") + Message;
    return false;
}

bool Fields(const FObject& Object, std::initializer_list<const TCHAR*> Allowed,
    const FString& Path, FString& Error)
{
    for (const auto& Pair : Object->Values)
    {
        bool bAllowed = false;
        for (const TCHAR* Key : Allowed)
            bAllowed |= FString(*Pair.Key).Equals(Key, ESearchCase::CaseSensitive);
        if (!bAllowed)
            return Fail(Error, Path + TEXT(".") + FString(*Pair.Key), TEXT("unknown field"));
    }
    return true;
}

bool ObjectValue(const TSharedPtr<FJsonValue>& Value, FObject& Out,
    const FString& Path, FString& Error)
{
    if (!Value.IsValid() || Value->Type != EJson::Object)
        return Fail(Error, Path, TEXT("expected an object"));
    Out = Value->AsObject(); // Type checked before the asserting accessor.
    return Out.IsValid() || Fail(Error, Path, TEXT("invalid object"));
}

bool String(const FObject& Object, const TCHAR* Key, FString& Out,
    const FString& Path, FString& Error, bool bRequired = true,
    bool bAllowEmpty = false)
{
    const auto* Value = Object->Values.Find(Key);
    if (!Value)
        return !bRequired || Fail(Error, Path + TEXT(".") + Key, TEXT("required field"));
    if (!Value->IsValid() || (*Value)->Type != EJson::String ||
        !(*Value)->TryGetString(Out) ||
        (!bAllowEmpty && Out.TrimStartAndEnd().IsEmpty()))
        return Fail(Error, Path + TEXT(".") + Key, TEXT("expected a nonempty string"));
    return true;
}

bool Integer(const FObject& Object, const TCHAR* Key, int32& Out,
    const FString& Path, FString& Error, int32 Min, int32 Max,
    bool bRequired = false)
{
    const auto* Value = Object->Values.Find(Key);
    if (!Value)
        return !bRequired || Fail(Error, Path + TEXT(".") + Key, TEXT("required integer"));
    double Number = 0;
    if (!Value->IsValid() || (*Value)->Type != EJson::Number ||
        !(*Value)->TryGetNumber(Number) || !FMath::IsFinite(Number) ||
        Number < static_cast<double>(Min) || Number > static_cast<double>(Max))
        return Fail(Error, Path + TEXT(".") + Key, TEXT("integer outside supported range"));
    const int32 Candidate = static_cast<int32>(Number);
    if (static_cast<double>(Candidate) != Number)
        return Fail(Error, Path + TEXT(".") + Key, TEXT("expected an integral JSON number"));
    Out = Candidate;
    return true;
}

bool Boolean(const FObject& Object, const TCHAR* Key, bool& Out,
    const FString& Path, FString& Error)
{
    const auto* Value = Object->Values.Find(Key);
    if (!Value)
        return true;
    return (Value->IsValid() && (*Value)->Type == EJson::Boolean &&
        (*Value)->TryGetBool(Out)) ||
        Fail(Error, Path + TEXT(".") + Key, TEXT("expected a boolean"));
}

bool Array(const FObject& Object, const TCHAR* Key, const FArray*& Out,
    const FString& Path, FString& Error, bool bRequired = false)
{
    static const FArray Empty;
    Out = &Empty;
    if (!Object->HasField(Key))
        return !bRequired || Fail(Error, Path + TEXT(".") + Key, TEXT("required array"));
    return (Object->TryGetArrayField(Key, Out) && Out) ||
        Fail(Error, Path + TEXT(".") + Key, TEXT("expected an array"));
}

bool Token(const FString& Value)
{
    if (Value.IsEmpty() || Value.Len() >= NAME_SIZE ||
        Value.Equals(TEXT("None"), ESearchCase::IgnoreCase))
        return false;
    for (TCHAR C : Value)
        if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') ||
            (C >= '0' && C <= '9') || C == '_' || C == '-' || C == '.' ||
            C == ':' || C == '/'))
            return false;
    return true;
}

bool Name(const FObject& Object, const TCHAR* Key, FName& Out,
    const FString& Path, FString& Error, bool bRequired = false)
{
    FString Value;
    if (!String(Object, Key, Value, Path, Error, bRequired))
        return false;
    if (!Object->HasField(Key))
        return true;
    if (!Token(Value))
        return Fail(Error, Path + TEXT(".") + Key, TEXT("expected a stable name token"));
    Out = FName(*Value);
    return true;
}

bool Id(const FObject& Object, const TCHAR* Key, FOGContentId& Out,
    const FString& Path, FString& Error)
{
    if (!String(Object, Key, Out.Value, Path, Error))
        return false;
    return Out.IsValid() || Fail(Error, Path + TEXT(".") + Key,
        TEXT("expected a stable lowercase namespace:key content ID"));
}

// Checked decimal accumulation never sends authored integers through double.
bool Decimal(const FString& Text, int64& Out, bool bPositive)
{
    if (Text.IsEmpty() || (Text.Len() > 1 && Text[0] == '0'))
        return false;
    int64 Value = 0;
    for (TCHAR C : Text)
    {
        if (C < '0' || C > '9' || Value > (MAX_int64 - (C - '0')) / 10)
            return false;
        Value = Value * 10 + (C - '0');
    }
    Out = Value;
    return !bPositive || Value > 0;
}

bool StatKey(const FString& Key)
{
    static const TCHAR* Keys[] = { TEXT("max_hp"), TEXT("attack"), TEXT("defense"),
        TEXT("speed"), TEXT("crit_rate_bps"), TEXT("crit_damage_bonus_bps"),
        TEXT("crit_rate_resistance_bps"), TEXT("crit_damage_resistance_bps"),
        TEXT("hit_bps"), TEXT("dodge_bps"), TEXT("block_rate_bps"),
        TEXT("block_reduction_bps"), TEXT("defense_penetration_bps"),
        TEXT("damage_dealt_multiplier_bps"), TEXT("damage_taken_multiplier_bps"),
        TEXT("true_damage_resistance_bps") };
    for (const TCHAR* Allowed : Keys)
        if (Key.Equals(Allowed, ESearchCase::CaseSensitive))
            return true;
    return false;
}

bool ValidGrant(const FString& Key, const FOGLargeNumber& Grant)
{
    if (!StatKey(Key) || Grant.Significand < 0 ||
        Grant.Significand > FOGLargeNumber::MaxSignificand ||
        Grant.Exponent10 < MinGrantExponent || Grant.Exponent10 > MaxGrantExponent)
        return false;
    FOGLargeNumber Normalized = Grant;
    Normalized.Normalize();
    if (Normalized.Exponent10 < MinGrantExponent || Normalized.Exponent10 > MaxGrantExponent)
        return false;
    if (Key == TEXT("max_hp") || Key == TEXT("attack") || Key == TEXT("defense") ||
        Grant.IsZero())
        return true;

    // Remaining combat fields are int32. Require exact nonnegative integral
    // increments; do not truncate fractions or overflow when applying them.
    int64 Value = Grant.Significand;
    int32 Exponent = Grant.Exponent10;
    while (Exponent < 0)
    {
        if (Value % 10 != 0)
            return false;
        Value /= 10;
        ++Exponent;
    }
    while (Exponent > 0)
    {
        if (Value > MAX_int32 / 10)
            return false;
        Value *= 10;
        --Exponent;
    }
    return Value <= MAX_int32;
}

bool Ids(const FObject& Object, const TCHAR* Key, TArray<FOGContentId>& Out,
    const FString& Path, FString& Error)
{
    const FArray* Values = nullptr;
    if (!Array(Object, Key, Values, Path, Error))
        return false;
    for (int32 Index = 0; Index < Values->Num(); ++Index)
    {
        FString Value;
        const auto& Item = (*Values)[Index];
        if (!Item.IsValid() || Item->Type != EJson::String ||
            !Item->TryGetString(Value) || !FOGContentId(Value).IsValid())
            return Fail(Error, Path + TEXT(".") + Key + FString::Printf(TEXT("[%d]"), Index),
                TEXT("expected a stable content ID string"));
        Out.Emplace(Value);
    }
    return true;
}

bool Routes(const FObject& Object, const TCHAR* Key,
    TArray<FOGDevelopmentRouteRequirement>& Out, const FString& Path, FString& Error)
{
    const FArray* Values = nullptr;
    if (!Array(Object, Key, Values, Path, Error))
        return false;
    for (int32 Index = 0; Index < Values->Num(); ++Index)
    {
        const FString At = Path + TEXT(".") + Key + FString::Printf(TEXT("[%d]"), Index);
        FObject Row;
        FOGDevelopmentRouteRequirement Route;
        if (!ObjectValue((*Values)[Index], Row, At, Error) ||
            !Fields(Row, {TEXT("route_id"), TEXT("node_id"), TEXT("state")}, At, Error) ||
            !Id(Row, TEXT("route_id"), Route.RouteId, At, Error) ||
            !Id(Row, TEXT("node_id"), Route.NodeId, At, Error) ||
            !Name(Row, TEXT("state"), Route.State, At, Error, true))
            return false;
        Out.Add(MoveTemp(Route));
    }
    return true;
}

bool ReadOffer(const FObject& Row, FOGCharacterDevelopmentDefinition& Out,
    const FString& Path, FString& Error)
{
    if (!Fields(Row, {TEXT("development_id"), TEXT("identity_id"), TEXT("version_id"),
        TEXT("display_name"), TEXT("description"), TEXT("minimum_level"),
        TEXT("required_rarity"), TEXT("required_reinforcement_state"),
        TEXT("prerequisite_development_ids"), TEXT("excluded_development_ids"),
        TEXT("required_routes"), TEXT("required_facts"), TEXT("costs"), TEXT("item_costs"),
        TEXT("result_level"), TEXT("result_rarity"), TEXT("result_reinforcement_state"),
        TEXT("completes_reinforcement"), TEXT("stat_grants"), TEXT("granted_routes"),
        TEXT("granted_forms")}, Path, Error) ||
        !Id(Row, TEXT("development_id"), Out.DevelopmentId, Path, Error) ||
        !Id(Row, TEXT("identity_id"), Out.IdentityId, Path, Error) ||
        !Id(Row, TEXT("version_id"), Out.VersionId, Path, Error) ||
        !String(Row, TEXT("display_name"), Out.DisplayName, Path, Error) ||
        !String(Row, TEXT("description"), Out.Description, Path, Error, false, true) ||
        !Integer(Row, TEXT("minimum_level"), Out.MinimumLevel, Path, Error, 1, MaxRankLevel) ||
        !Integer(Row, TEXT("result_level"), Out.ResultLevel, Path, Error, 0, MaxRankLevel) ||
        !Name(Row, TEXT("required_rarity"), Out.RequiredRarity, Path, Error) ||
        !Name(Row, TEXT("result_rarity"), Out.ResultRarity, Path, Error) ||
        !Name(Row, TEXT("required_reinforcement_state"), Out.RequiredReinforcementState, Path, Error) ||
        !Name(Row, TEXT("result_reinforcement_state"), Out.ResultReinforcementState, Path, Error) ||
        !Boolean(Row, TEXT("completes_reinforcement"), Out.bCompletesReinforcement, Path, Error) ||
        !Ids(Row, TEXT("prerequisite_development_ids"), Out.PrerequisiteDevelopmentIds, Path, Error) ||
        !Ids(Row, TEXT("excluded_development_ids"), Out.ExcludedDevelopmentIds, Path, Error) ||
        !Ids(Row, TEXT("granted_forms"), Out.GrantedForms, Path, Error) ||
        !Routes(Row, TEXT("required_routes"), Out.RequiredRoutes, Path, Error) ||
        !Routes(Row, TEXT("granted_routes"), Out.GrantedRoutes, Path, Error))
        return false;

    const FArray* Costs = nullptr;
    const FArray* Facts = nullptr;
    const FArray* ItemCosts = nullptr;
    if (!Array(Row, TEXT("costs"), Costs, Path, Error) ||
        !Array(Row, TEXT("required_facts"), Facts, Path, Error) ||
        !Array(Row, TEXT("item_costs"), ItemCosts, Path, Error))
        return false;
    for (int32 Index = 0; Index < Costs->Num(); ++Index)
    {
        const FString At = Path + FString::Printf(TEXT(".costs[%d]"), Index);
        FObject Cost;
        FOGDevelopmentCost Result;
        FString Amount;
        if (!ObjectValue((*Costs)[Index], Cost, At, Error) ||
            !Fields(Cost, {TEXT("resource_id"), TEXT("display_name"), TEXT("amount")}, At, Error) ||
            !Id(Cost, TEXT("resource_id"), Result.ResourceId, At, Error) ||
            !String(Cost, TEXT("display_name"), Result.DisplayName, At, Error) ||
            !String(Cost, TEXT("amount"), Amount, At, Error))
            return false;
        if (!Decimal(Amount, Result.Amount, true))
            return Fail(Error, At + TEXT(".amount"), TEXT("expected a positive int64 decimal string"));
        Out.Costs.Add(MoveTemp(Result));
    }
    for (int32 Index = 0; Index < ItemCosts->Num(); ++Index)
    {
        const FString At = Path + FString::Printf(TEXT(".item_costs[%d]"), Index);
        FObject Cost;
        FOGDevelopmentItemCost Result;
        if (!ObjectValue((*ItemCosts)[Index], Cost, At, Error) ||
            !Fields(Cost, {TEXT("object_id"), TEXT("rank_id"), TEXT("quality_id"), TEXT("instance_count")}, At, Error) ||
            !Id(Cost, TEXT("object_id"), Result.ObjectId, At, Error) ||
            !Integer(Cost, TEXT("instance_count"), Result.InstanceCount, At, Error, 1, MAX_int32, true))
            return false;
        if (Cost->HasField(TEXT("rank_id")) && !Id(Cost, TEXT("rank_id"), Result.RankId, At, Error))
            return false;
        if (Cost->HasField(TEXT("quality_id")) && !Id(Cost, TEXT("quality_id"), Result.QualityId, At, Error))
            return false;
        Out.ItemCosts.Add(MoveTemp(Result));
    }
    for (int32 Index = 0; Index < Facts->Num(); ++Index)
    {
        const FString At = Path + FString::Printf(TEXT(".required_facts[%d]"), Index);
        FObject Fact;
        FOGDevelopmentFactRequirement Result;
        FString Subject;
        if (!ObjectValue((*Facts)[Index], Fact, At, Error) ||
            !Fields(Fact, {TEXT("fact_key"), TEXT("display_name"), TEXT("subject")}, At, Error) ||
            !Name(Fact, TEXT("fact_key"), Result.FactKey, At, Error, true) ||
            !String(Fact, TEXT("display_name"), Result.DisplayName, At, Error) ||
            !String(Fact, TEXT("subject"), Subject, At, Error))
            return false;
        if (!Subject.Equals(TEXT("ruler"), ESearchCase::CaseSensitive) &&
            !Subject.Equals(TEXT("manifestation"), ESearchCase::CaseSensitive))
            return Fail(Error, At + TEXT(".subject"), TEXT("expected ruler or manifestation"));
        Result.bSubjectIsRuler = Subject.Equals(TEXT("ruler"), ESearchCase::CaseSensitive);
        Out.RequiredFacts.Add(MoveTemp(Result));
    }
    if (const auto* Grants = Row->Values.Find(TEXT("stat_grants")))
    {
        FObject Stats;
        if (!ObjectValue(*Grants, Stats, Path + TEXT(".stat_grants"), Error))
            return false;
        for (const auto& Pair : Stats->Values)
        {
            const FString At = Path + TEXT(".stat_grants.") + FString(*Pair.Key);
            if (!StatKey(FString(*Pair.Key)))
                return Fail(Error, At, TEXT("unknown or incorrectly cased stat key"));
            FObject Number;
            FString Significand;
            int64 Significant = 0;
            int32 Exponent = 0;
            if (!ObjectValue(Pair.Value, Number, At, Error) ||
                !Fields(Number, {TEXT("significand"), TEXT("exponent10")}, At, Error) ||
                !String(Number, TEXT("significand"), Significand, At, Error) ||
                !Integer(Number, TEXT("exponent10"), Exponent, At, Error,
                    MinGrantExponent, MaxGrantExponent, true))
                return false;
            if (!Decimal(Significand, Significant, false) ||
                Significant > FOGLargeNumber::MaxSignificand)
                return Fail(Error, At, TEXT("significand must be a nonnegative decimal string of at most 9 digits"));
            Out.StatGrants.Add(FName(*Pair.Key), FOGLargeNumber(Significant, Exponent));
        }
    }
    return true;
}

int32 RarityRank(FName Rarity)
{
    static const FName Order[] = {TEXT("R"), TEXT("SR"), TEXT("SSR"),
        TEXT("UR"), TEXT("LR"), TEXT("MR")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Order); ++Index)
        if (Rarity == Order[Index])
            return Index;
    return INDEX_NONE;
}

bool UniqueIds(const TArray<FOGContentId>& Values, const FString& Path, FString& Error)
{
    TSet<FOGContentId> Seen;
    for (const auto& Value : Values)
    {
        if (!Value.IsValid() || Seen.Contains(Value))
            return Fail(Error, Path, TEXT("invalid or duplicate content ID: ") + Value.Value);
        Seen.Add(Value);
    }
    return true;
}

bool UniqueRoutes(const TArray<FOGDevelopmentRouteRequirement>& Values,
    const FString& Path, FString& Error)
{
    TSet<FString> Seen;
    for (const auto& Value : Values)
    {
        const FString Key = Value.RouteId.Value + TEXT("\n") + Value.NodeId.Value;
        if (!Value.RouteId.IsValid() || !Value.NodeId.IsValid() ||
            !Token(Value.State.ToString()) || Seen.Contains(Key))
            return Fail(Error, Path, TEXT("invalid route/node/state or duplicate route/node"));
        Seen.Add(Key);
    }
    return true;
}

// The DOM otherwise overwrites duplicate object members. Check before loading
// it, and bound nesting to keep hostile input away from recursive DOM parsing.
bool CheckJson(const FString& Json, FString& Error)
{
    struct FScope { bool bObject = false; TSet<FString> Keys; };
    TArray<FScope> Stack;
    const auto Reader = TJsonReaderFactory<>::Create(Json);
    EJsonNotation Notation;
    while (Reader->ReadNext(Notation))
    {
        if (Notation == EJsonNotation::Error)
            return Fail(Error, TEXT("catalog"), Reader->GetErrorMessage());
        if (Notation == EJsonNotation::Number)
        {
            // Every numeric JSON field in v1 is int32. Check its original
            // token so a tiny fraction cannot round to an accepted integer.
            const FString& Raw = Reader->GetValueAsNumberString();
            const bool bNegative = Raw.StartsWith(TEXT("-"));
            int64 Magnitude = 0;
            if (!Decimal(bNegative ? Raw.Mid(1) : Raw, Magnitude, false) ||
                Magnitude > (bNegative ? -static_cast<int64>(MIN_int32) : MAX_int32))
                return Fail(Error, TEXT("catalog.") + Reader->GetIdentifier(),
                    TEXT("expected a plain int32 JSON integer token, without fraction or exponent"));
        }
        const bool bEnd = Notation == EJsonNotation::ObjectEnd || Notation == EJsonNotation::ArrayEnd;
        if (!bEnd && !Stack.IsEmpty() && Stack.Last().bObject)
        {
            const FString Key = Reader->GetIdentifier();
            if (Stack.Last().Keys.Contains(Key))
                return Fail(Error, TEXT("catalog"), TEXT("duplicate JSON field: ") + Key);
            Stack.Last().Keys.Add(Key);
        }
        if (Notation == EJsonNotation::ObjectStart || Notation == EJsonNotation::ArrayStart)
        {
            if (Stack.Num() >= 64)
                return Fail(Error, TEXT("catalog"), TEXT("JSON nesting exceeds 64 levels"));
            FScope Scope;
            Scope.bObject = Notation == EJsonNotation::ObjectStart;
            Stack.Add(MoveTemp(Scope));
        }
        else if (bEnd && !Stack.IsEmpty())
            Stack.Pop();
    }
    return (Reader->GetErrorMessage().IsEmpty() && Stack.IsEmpty()) ||
        Fail(Error, TEXT("catalog"), TEXT("malformed or incomplete JSON: ") + Reader->GetErrorMessage());
}
}

bool FOGCharacterDevelopmentCatalog::LoadInstalled(
    TArray<FOGCharacterDevelopmentDefinition>& Out, FString& Error)
{
    Out.Reset();
    Error.Reset();
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(),
        TEXT("NoWorld/Progression/DevelopmentCatalog.json"));
    FString Json;
    FFileHelper::LoadFileToString(Json, *Path);
    FString Merged;
    if (!UOGDefinitionLibrary::LoadCatalog(TEXT("development"),Json,Merged,Error)) return false;
    Json=MoveTemp(Merged);
    if (!Parse(Json, Out, Error))
    {
        Error = Path + TEXT(": ") + Error;
        return false;
    }
    return true;
}

bool FOGCharacterDevelopmentCatalog::Parse(const FString& Json,
    TArray<FOGCharacterDevelopmentDefinition>& Out, FString& Error)
{
    Out.Reset();
    Error.Reset();
    if (!CheckJson(Json, Error))
        return false;
    FObject Root;
    const auto Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        return Fail(Error, TEXT("catalog"), TEXT("expected a valid JSON object: ") + Reader->GetErrorMessage());
    FString Schema;
    const FArray* Offers = nullptr;
    if (!Fields(Root, {TEXT("schema"), TEXT("offers")}, TEXT("catalog"), Error) ||
        !String(Root, TEXT("schema"), Schema, TEXT("catalog"), Error))
        return false;
    if (!Schema.Equals(TEXT("noworld.character_development.v1"), ESearchCase::CaseSensitive))
        return Fail(Error, TEXT("catalog.schema"), TEXT("expected noworld.character_development.v1"));
    if (!Array(Root, TEXT("offers"), Offers, TEXT("catalog"), Error, true))
        return false;
    TArray<FOGCharacterDevelopmentDefinition> Parsed;
    for (int32 Index = 0; Index < Offers->Num(); ++Index)
    {
        const FString Path = FString::Printf(TEXT("catalog.offers[%d]"), Index);
        FObject Row;
        FOGCharacterDevelopmentDefinition Definition;
        if (!ObjectValue((*Offers)[Index], Row, Path, Error) ||
            !ReadOffer(Row, Definition, Path, Error))
            return false;
        Parsed.Add(MoveTemp(Definition));
    }
    if (!Validate(Parsed, Error))
        return false;
    Out = MoveTemp(Parsed);
    return true;
}

bool FOGCharacterDevelopmentCatalog::ValidateDefinition(
    const FOGCharacterDevelopmentDefinition& Definition, FString& Error)
{
    Error.Reset();
    const auto& D = Definition;
    const FString Path = TEXT("offer[") + D.DevelopmentId.Value + TEXT("]");
    if (!D.DevelopmentId.IsValid() || !D.IdentityId.IsValid() || !D.VersionId.IsValid())
        return Fail(Error, Path, TEXT("development_id, identity_id and version_id must be stable content IDs"));
    if (D.DisplayName.TrimStartAndEnd().IsEmpty())
        return Fail(Error, Path + TEXT(".display_name"), TEXT("required nonempty display name"));
    if (D.MinimumLevel < 1 || D.MinimumLevel > MaxRankLevel ||
        D.ResultLevel < 0 || D.ResultLevel > MaxRankLevel ||
        (D.ResultLevel > 0 && D.ResultLevel < D.MinimumLevel))
        return Fail(Error, Path, TEXT("minimum_level must be 1..100; result_level must be zero or minimum_level..100; breakthroughs use the rank engine"));
    const FName Names[] = {D.RequiredRarity, D.ResultRarity,
        D.RequiredReinforcementState, D.ResultReinforcementState};
    for (FName Value : Names)
        if (!Value.IsNone() && !Token(Value.ToString()))
            return Fail(Error, Path, TEXT("invalid rarity or reinforcement state token"));
    if (!D.ResultRarity.IsNone())
    {
        const int32 Before = RarityRank(D.RequiredRarity);
        const int32 After = RarityRank(D.ResultRarity);
        if (D.RequiredRarity.IsNone() ||
            ((Before == INDEX_NONE || After == INDEX_NONE) ? D.ResultRarity != D.RequiredRarity : After < Before))
            return Fail(Error, Path + TEXT(".result_rarity"),
                TEXT("requires required_rarity; cannot decrease R/SR/SSR/UR/LR/MR or change an unknown rarity"));
    }
    if (!UniqueIds(D.PrerequisiteDevelopmentIds, Path + TEXT(".prerequisite_development_ids"), Error) ||
        !UniqueIds(D.ExcludedDevelopmentIds, Path + TEXT(".excluded_development_ids"), Error) ||
        !UniqueIds(D.GrantedForms, Path + TEXT(".granted_forms"), Error) ||
        !UniqueRoutes(D.RequiredRoutes, Path + TEXT(".required_routes"), Error) ||
        !UniqueRoutes(D.GrantedRoutes, Path + TEXT(".granted_routes"), Error))
        return false;
    if (D.PrerequisiteDevelopmentIds.Contains(D.DevelopmentId) ||
        D.ExcludedDevelopmentIds.Contains(D.DevelopmentId))
        return Fail(Error, Path, TEXT("offer cannot require or exclude itself"));
    for (const auto& Id : D.PrerequisiteDevelopmentIds)
        if (D.ExcludedDevelopmentIds.Contains(Id))
            return Fail(Error, Path, TEXT("same offer cannot be both required and excluded"));
    TSet<FOGContentId> Resources;
    for (const auto& Cost : D.Costs)
    {
        if (!Cost.ResourceId.IsValid() || Cost.DisplayName.TrimStartAndEnd().IsEmpty() ||
            Cost.Amount <= 0 || Resources.Contains(Cost.ResourceId))
            return Fail(Error, Path + TEXT(".costs"),
                TEXT("costs need a unique canonical resource_id, display_name and positive int64 amount"));
        Resources.Add(Cost.ResourceId);
    }
    TSet<FString> ItemKeys;
    for (const auto& Cost : D.ItemCosts)
    {
        const FString Key = Cost.ObjectId.ToString() + TEXT("|") +
            Cost.RankId.ToString() + TEXT("|") + Cost.QualityId.ToString();
        if (!Cost.ObjectId.IsValid() ||
            (!Cost.RankId.IsEmpty() && !Cost.RankId.IsValid()) ||
            (!Cost.QualityId.IsEmpty() && !Cost.QualityId.IsValid()) ||
            Cost.InstanceCount <= 0 || ItemKeys.Contains(Key))
            return Fail(Error, Path + TEXT(".item_costs"),
                TEXT("requires unique object/rank/quality inputs and positive instance_count"));
        ItemKeys.Add(Key);
    }
    TSet<FString> Facts;
    for (const auto& Fact : D.RequiredFacts)
    {
        const FString Key = FString(Fact.bSubjectIsRuler ? TEXT("ruler:") : TEXT("manifestation:")) +
            Fact.FactKey.ToString().ToLower();
        if (!Token(Fact.FactKey.ToString()) || Fact.DisplayName.TrimStartAndEnd().IsEmpty() || Facts.Contains(Key))
            return Fail(Error, Path + TEXT(".required_facts"), TEXT("invalid or duplicate subject/event type requirement"));
        Facts.Add(Key);
    }
    bool bHasOutcome = D.ResultLevel > D.MinimumLevel ||
        (!D.ResultRarity.IsNone() && D.ResultRarity != D.RequiredRarity) ||
        (!D.ResultReinforcementState.IsNone() && D.ResultReinforcementState != D.RequiredReinforcementState) ||
        D.bCompletesReinforcement || !D.GrantedForms.IsEmpty();
    for (const auto& Grant : D.StatGrants)
    {
        if (!ValidGrant(Grant.Key.ToString(), Grant.Value))
            return Fail(Error, Path + TEXT(".stat_grants.") + Grant.Key.ToString(),
                TEXT("unknown stat, negative/imprecise number, unsafe exponent, or nonintegral/out-of-int32 scalar grant"));
        bHasOutcome |= !Grant.Value.IsZero();
    }
    for (const auto& Grant : D.GrantedRoutes)
    {
        bool bUnchanged = false;
        for (const auto& Required : D.RequiredRoutes)
            bUnchanged |= Grant.RouteId == Required.RouteId && Grant.NodeId == Required.NodeId &&
                Grant.State == Required.State;
        bHasOutcome |= !bUnchanged;
    }
    return bHasOutcome || Fail(Error, Path, TEXT("offer must have a nonempty, effective outcome"));
}

bool FOGCharacterDevelopmentCatalog::Validate(
    const TArray<FOGCharacterDevelopmentDefinition>& Definitions, FString& Error)
{
    Error.Reset();
    TMap<FOGContentId, const FOGCharacterDevelopmentDefinition*> ById;
    for (const auto& D : Definitions)
    {
        if (!ValidateDefinition(D, Error))
            return false;
        if (ById.Contains(D.DevelopmentId))
            return Fail(Error, D.DevelopmentId.Value, TEXT("duplicate development_id"));
        ById.Add(D.DevelopmentId, &D);
    }
    TMap<FOGContentId, int32> Pending;
    TMap<FOGContentId, TArray<FOGContentId>> Dependents;
    TArray<FOGContentId> Ready;
    for (const auto& D : Definitions)
    {
        for (const auto* References : {&D.PrerequisiteDevelopmentIds, &D.ExcludedDevelopmentIds})
            for (const auto& Id : *References)
            {
                const auto* Target = ById.Find(Id);
                if (!Target || (*Target)->IdentityId != D.IdentityId || (*Target)->VersionId != D.VersionId)
                    return Fail(Error, D.DevelopmentId.Value,
                        TEXT("reference must resolve in this catalog to the same identity/version: ") + Id.Value);
            }
        Pending.Add(D.DevelopmentId, D.PrerequisiteDevelopmentIds.Num());
        if (D.PrerequisiteDevelopmentIds.IsEmpty())
            Ready.Add(D.DevelopmentId);
        for (const auto& Id : D.PrerequisiteDevelopmentIds)
            Dependents.FindOrAdd(Id).Add(D.DevelopmentId);
    }
    // Iterative topological validation avoids recursion on long authored chains.
    int32 Visited = 0;
    for (int32 Index = 0; Index < Ready.Num(); ++Index)
    {
        ++Visited;
        if (const auto* Children = Dependents.Find(Ready[Index]))
            for (const auto& Child : *Children)
            {
                int32* Count = Pending.Find(Child);
                if (Count && --*Count == 0)
                    Ready.Add(Child);
            }
    }
    if (Visited != Definitions.Num())
        return Fail(Error, TEXT("catalog.offers"), TEXT("prerequisite development cycle detected"));

    for (const auto& D : Definitions)
    {
        TSet<FOGContentId> Closure;
        TArray<const FOGCharacterDevelopmentDefinition*> Ancestors;
        Closure.Add(D.DevelopmentId);
        Ancestors.Add(&D);
        for (int32 Index = 0; Index < Ancestors.Num(); ++Index)
            for (const auto& Id : Ancestors[Index]->PrerequisiteDevelopmentIds)
                if (!Closure.Contains(Id))
                {
                    // All references have already been resolved above.
                    const auto* Target = ById.Find(Id);
                    if (Target)
                    {
                        Closure.Add(Id);
                        Ancestors.Add(*Target);
                    }
                }
        for (const auto* Ancestor : Ancestors)
            for (const auto& Excluded : Ancestor->ExcludedDevelopmentIds)
                if (Closure.Contains(Excluded))
                    return Fail(Error, D.DevelopmentId.Value,
                        TEXT("unreachable prerequisite closure: ") + Ancestor->DevelopmentId.Value +
                        TEXT(" excludes required offer ") + Excluded.Value);
    }
    return true;
}
