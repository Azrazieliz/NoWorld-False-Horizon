#include "Content/OGDefinitionLibrary.h"
#include "World/OGOwnedEquipmentService.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <initializer_list>

namespace
{
bool EquipmentCatalogError(FString& Error, const FString& Detail)
{
    Error = TEXT("Equipment catalog: ") + Detail;
    return false;
}
bool EquipmentFields(const TSharedPtr<FJsonObject>& Object,
    std::initializer_list<const TCHAR*> Allowed, FString& Error)
{
    if (!Object.IsValid()) return EquipmentCatalogError(Error, TEXT("expected an object."));
    for (const auto& Field : Object->Values)
    {
        bool bKnown = false;
        for (const TCHAR* Name : Allowed) bKnown |= Field.Key == Name;
        if (!bKnown) return EquipmentCatalogError(Error, TEXT("unknown field ") + Field.Key);
    }
    return true;
}
bool EquipmentString(const TSharedPtr<FJsonObject>& O, const TCHAR* Key,
    FString& Out, FString& Error, bool bEmptyAllowed = false)
{
    if (!O->TryGetStringField(Key, Out) || (!bEmptyAllowed && Out.TrimStartAndEnd().IsEmpty()))
        return EquipmentCatalogError(Error, FString(TEXT("expected string ")) + Key);
    return true;
}
bool EquipmentId(const TSharedPtr<FJsonObject>& O, const TCHAR* Key,
    FOGContentId& Out, FString& Error, bool bEmptyAllowed = false)
{
    return EquipmentString(O, Key, Out.Value, Error, bEmptyAllowed) &&
        ((bEmptyAllowed && Out.IsEmpty()) || Out.IsValid() ||
         EquipmentCatalogError(Error, FString(TEXT("invalid content ID ")) + Key));
}
bool EquipmentJsonShape(const FString& Json, FString& Error)
{
    struct FScope { bool bObject = false; TSet<FString> Keys; };
    TArray<FScope> Stack;
    const auto Reader = TJsonReaderFactory<>::Create(Json);
    EJsonNotation N;
    while (Reader->ReadNext(N))
    {
        if (N == EJsonNotation::Error) return EquipmentCatalogError(Error, Reader->GetErrorMessage());
        const bool bEnd = N == EJsonNotation::ObjectEnd || N == EJsonNotation::ArrayEnd;
        if (!bEnd && !Stack.IsEmpty() && Stack.Last().bObject)
        {
            const FString Key = Reader->GetIdentifier();
            if (Stack.Last().Keys.Contains(Key))
                return EquipmentCatalogError(Error, TEXT("duplicate field ") + Key);
            Stack.Last().Keys.Add(Key);
        }
        if (N == EJsonNotation::Number)
        {
            const FString Raw = Reader->GetValueAsNumberString();
            int32 Value = 0;
            if (!LexTryParseString(Value, *Raw) || LexToString(Value) != Raw)
                return EquipmentCatalogError(Error, TEXT("numeric fields must be plain int32 tokens."));
        }
        if (N == EJsonNotation::ObjectStart || N == EJsonNotation::ArrayStart)
        {
            if (Stack.Num() >= 32) return EquipmentCatalogError(Error, TEXT("excessive nesting."));
            FScope Scope;
            Scope.bObject = N == EJsonNotation::ObjectStart;
            Stack.Add(MoveTemp(Scope));
        }
        else if (bEnd && !Stack.IsEmpty()) Stack.Pop();
    }
    return (Stack.IsEmpty() && Reader->GetErrorMessage().IsEmpty()) ||
        EquipmentCatalogError(Error, TEXT("invalid JSON."));
}
FString EquipmentSlotKey(const FOGContentId& Identity, const FOGContentId& Version,
    const FOGContentId& Slot)
{
    return Identity.Value + TEXT("|") + Version.Value + TEXT("|") + Slot.Value;
}
bool ValidEquipmentBonus(FName Key, const FOGLargeNumber& Value)
{
    if (Value.GetSign() < 0 || Value.Significand > FOGLargeNumber::MaxSignificand ||
        Value.Exponent10 < -100000 || Value.Exponent10 > 100000) return false;
    if (Key == TEXT("max_hp") || Key == TEXT("attack") || Key == TEXT("defense")) return true;
    if (Key != TEXT("speed")) return false;
    FOGLargeNumber Integral = Value;
    int64 Number = Integral.Significand;
    if (Number == 0) return true;
    if (Integral.Exponent10 < -9 || Integral.Exponent10 > 9) return false;
    for (int32 I = 0; I < Integral.Exponent10; ++I)
    {
        if (Number > MAX_int32 / 10) return false;
        Number *= 10;
    }
    for (int32 I = 0; I > Integral.Exponent10; --I)
    {
        if (Number % 10) return false;
        Number /= 10;
    }
    return Number <= MAX_int32;
}
void EquipmentFingerprintPart(FString& Out, const FString& Part)
{
    Out += LexToString(Part.Len()) + TEXT(":") + Part;
}
}

bool FOGEquipmentCatalog::LoadInstalled(FOGEquipmentCatalog& Out, FString& Error)
{
    Out = FOGEquipmentCatalog();
    const FString Path = FPaths::ProjectContentDir() / TEXT("NoWorld/Equipment/EquipmentCatalog.json");
    FString Json;
    FFileHelper::LoadFileToString(Json, *Path);
    FString Merged;
    if (!UOGDefinitionLibrary::LoadCatalog(TEXT("equipment"),Json,Merged,Error)) return false;
    return Parse(Merged,Out,Error);
}

bool FOGEquipmentCatalog::Parse(const FString& Json, FOGEquipmentCatalog& Out, FString& Error)
{
    Out = FOGEquipmentCatalog();
    Error.Reset();
    if (!EquipmentJsonShape(Json, Error)) return false;
    TSharedPtr<FJsonObject> Root;
    FString Schema;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) ||
        !EquipmentFields(Root, {TEXT("schema"), TEXT("slots"), TEXT("items"), TEXT("rules")}, Error) ||
        !EquipmentString(Root, TEXT("schema"), Schema, Error) ||
        !Schema.Equals(TEXT("noworld.equipment.v1"), ESearchCase::CaseSensitive))
        return EquipmentCatalogError(Error, TEXT("expected schema noworld.equipment.v1."));
    FOGEquipmentCatalog Parsed;
    for (const TCHAR* Key : {TEXT("slots"), TEXT("items"), TEXT("rules")})
    {
        const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
        if (!Root->TryGetArrayField(Key, Rows))
            return EquipmentCatalogError(Error, FString(TEXT("required array ")) + Key);
        for (const auto& Value : *Rows)
        {
            const TSharedPtr<FJsonObject>* RowPtr = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(RowPtr) || !RowPtr)
                return EquipmentCatalogError(Error, TEXT("array entry must be an object."));
            const TSharedPtr<FJsonObject>& Row = *RowPtr;
            if (FString(Key) == TEXT("slots"))
            {
                FOGAuthoredEquipmentSlot S;
                if (!EquipmentFields(Row, {TEXT("identity_id"), TEXT("version_id"), TEXT("slot_id"), TEXT("display_name")}, Error) ||
                    !EquipmentId(Row, TEXT("identity_id"), S.IdentityId, Error) ||
                    !EquipmentId(Row, TEXT("version_id"), S.VersionId, Error) ||
                    !EquipmentId(Row, TEXT("slot_id"), S.SlotId, Error) ||
                    !EquipmentString(Row, TEXT("display_name"), S.DisplayName, Error)) return false;
                Parsed.Slots.Add(MoveTemp(S));
            }
            else if (FString(Key) == TEXT("items"))
            {
                FOGAuthoredEquipmentItem I;
                if (!EquipmentFields(Row, {TEXT("definition_id"), TEXT("display_name")}, Error) ||
                    !EquipmentId(Row, TEXT("definition_id"), I.DefinitionId, Error) ||
                    !EquipmentString(Row, TEXT("display_name"), I.DisplayName, Error)) return false;
                Parsed.Items.Add(MoveTemp(I));
            }
            else
            {
                FOGAuthoredEquipmentRule R;
                if (!EquipmentFields(Row, {TEXT("identity_id"), TEXT("version_id"), TEXT("slot_id"),
                        TEXT("definition_id"), TEXT("rank_id"), TEXT("quality_id"), TEXT("stat_bonuses")}, Error) ||
                    !EquipmentId(Row, TEXT("identity_id"), R.IdentityId, Error) ||
                    !EquipmentId(Row, TEXT("version_id"), R.VersionId, Error) ||
                    !EquipmentId(Row, TEXT("slot_id"), R.SlotId, Error) ||
                    !EquipmentId(Row, TEXT("definition_id"), R.DefinitionId, Error) ||
                    !EquipmentId(Row, TEXT("rank_id"), R.RankId, Error, true) ||
                    !EquipmentId(Row, TEXT("quality_id"), R.QualityId, Error, true)) return false;
                const TSharedPtr<FJsonObject>* Bonuses = nullptr;
                if (!Row->TryGetObjectField(TEXT("stat_bonuses"), Bonuses))
                    return EquipmentCatalogError(Error, TEXT("stat_bonuses must be an explicit object."));
                for (const auto& Pair : (*Bonuses)->Values)
                {
                    const TSharedPtr<FJsonObject>* Number = nullptr;
                    FString Digits;
                    int64 Significand = 0;
                    int32 Exponent = 0;
                    if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(Number) || !Number ||
                        !EquipmentFields(*Number, {TEXT("significand"), TEXT("exponent10")}, Error) ||
                        !EquipmentString(*Number, TEXT("significand"), Digits, Error) ||
                        !LexTryParseString(Significand, *Digits) || LexToString(Significand) != Digits ||
                        !(*Number)->TryGetNumberField(TEXT("exponent10"), Exponent) ||
                        Significand < 0 || Significand > FOGLargeNumber::MaxSignificand ||
                        Exponent < -100000 || Exponent > 100000 ||
                        Pair.Key != Pair.Key.ToLower())
                        return EquipmentCatalogError(Error, TEXT("invalid stat contribution."));
                    R.StatBonuses.Add(FName(*Pair.Key), FOGLargeNumber(Significand, Exponent));
                }
                Parsed.Rules.Add(MoveTemp(R));
            }
        }
    }
    if (!Parsed.Validate(Error)) return false;
    Out = MoveTemp(Parsed);
    return true;
}


bool FOGEquipmentCatalog::Validate(FString& Error) const
{
    Error.Reset();
    TSet<FString> SlotKeys, RuleKeys;
    TSet<FOGContentId> ItemIds;
    for (const auto& S : Slots)
    {
        const FString Key = EquipmentSlotKey(S.IdentityId, S.VersionId, S.SlotId);
        if (!S.IdentityId.IsValid() || !S.VersionId.IsValid() || !S.SlotId.IsValid() ||
            S.DisplayName.TrimStartAndEnd().IsEmpty() || SlotKeys.Contains(Key))
            return EquipmentCatalogError(Error, TEXT("invalid or duplicate exact slot."));
        SlotKeys.Add(Key);
    }
    for (const auto& I : Items)
    {
        if (!I.DefinitionId.IsValid() || I.DisplayName.TrimStartAndEnd().IsEmpty() || ItemIds.Contains(I.DefinitionId))
            return EquipmentCatalogError(Error, TEXT("invalid or duplicate item definition."));
        ItemIds.Add(I.DefinitionId);
    }
    for (const auto& R : Rules)
    {
        const FString Slot = EquipmentSlotKey(R.IdentityId, R.VersionId, R.SlotId);
        const FString Key = Slot + TEXT("|") + R.DefinitionId.Value + TEXT("|") +
            R.RankId.Value + TEXT("|") + R.QualityId.Value;
        if (!SlotKeys.Contains(Slot) || !ItemIds.Contains(R.DefinitionId) ||
            (!R.RankId.IsEmpty() && !R.RankId.IsValid()) ||
            (!R.QualityId.IsEmpty() && !R.QualityId.IsValid()) || RuleKeys.Contains(Key))
            return EquipmentCatalogError(Error, TEXT("rule must uniquely reference an authored slot and item."));
        RuleKeys.Add(Key);
        for (const auto& Bonus : R.StatBonuses)
            if (!ValidEquipmentBonus(Bonus.Key, Bonus.Value))
                return EquipmentCatalogError(Error, TEXT("unsupported, negative or unsafe stat contribution."));
    }
    return true;
}

FString FOGEquipmentCatalog::Fingerprint() const
{
    FString Text;
    for (const auto& S : Slots)
        for (const auto& Part : {S.IdentityId.Value, S.VersionId.Value, S.SlotId.Value, S.DisplayName})
            EquipmentFingerprintPart(Text, Part);
    EquipmentFingerprintPart(Text, TEXT("items"));
    for (const auto& I : Items)
    {
        EquipmentFingerprintPart(Text, I.DefinitionId.Value);
        EquipmentFingerprintPart(Text, I.DisplayName);
    }
    EquipmentFingerprintPart(Text, TEXT("rules"));
    for (const auto& R : Rules)
    {
        for (const auto& Part : {R.IdentityId.Value, R.VersionId.Value, R.SlotId.Value,
                R.DefinitionId.Value, R.RankId.Value, R.QualityId.Value})
            EquipmentFingerprintPart(Text, Part);
        TArray<FName> Keys;
        R.StatBonuses.GetKeys(Keys);
        Keys.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });
        for (FName Key : Keys)
        {
            EquipmentFingerprintPart(Text, Key.ToString());
            EquipmentFingerprintPart(Text, R.StatBonuses.FindChecked(Key).ToDebugString());
        }
        EquipmentFingerprintPart(Text, TEXT("end-rule"));
    }
    return FMD5::HashAnsiString(*Text);
}

FString FOGEquipmentCatalog::ItemName(const FOGContentId& DefinitionId) const
{
    const auto* Item = Items.FindByPredicate([&](const auto& I) { return I.DefinitionId == DefinitionId; });
    return Item ? Item->DisplayName : DefinitionId.ToString();
}

const FOGAuthoredEquipmentRule* FOGEquipmentCatalog::FindRule(
    const FOGCharacterManifestationRecord& M, const FOGContentId& Slot,
    const FOGItemInstanceRecord& Item) const
{
    return Rules.FindByPredicate([&](const auto& R)
    {
        return R.IdentityId == M.IdentityId && R.VersionId == M.ActiveVersionId &&
            R.SlotId == Slot && R.DefinitionId == Item.DefinitionId &&
            R.RankId == Item.CurrentRankId && R.QualityId == Item.QualityId;
    });
}
