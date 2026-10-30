#include "Narrative/OGNarrativeJson.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace OGNarrative
{
bool Fail(FString& Error, const FString& At, const FString& Message)
{
    Error = At + TEXT(": ") + Message;
    return false;
}
bool ReadObject(const FString& Json, FObject& Out, FString& Error)
{
    Out.Reset();
    if (Json.Len() > 8 * 1024 * 1024) return Fail(Error, TEXT("JSON"), TEXT("exceeds 8 Mi characters"));
    struct FScope { bool IsObject = false; TSet<FString> Keys; };
    TArray<FScope> Stack;
    auto Reader = TJsonReaderFactory<>::Create(Json);
    EJsonNotation N;
    while (Reader->ReadNext(N))
    {
        if (N == EJsonNotation::Error) return Fail(Error, TEXT("JSON"), Reader->GetErrorMessage());
        const bool End = N == EJsonNotation::ObjectEnd || N == EJsonNotation::ArrayEnd;
        if (!End && !Stack.IsEmpty() && Stack.Last().IsObject)
        {
            const FString Key = Reader->GetIdentifier();
            if (Stack.Last().Keys.Contains(Key)) return Fail(Error, Key, TEXT("duplicate JSON field"));
            Stack.Last().Keys.Add(Key);
        }
        if (N == EJsonNotation::ObjectStart || N == EJsonNotation::ArrayStart)
        {
            if (Stack.Num() >= 64) return Fail(Error, TEXT("JSON"), TEXT("nesting exceeds 64 levels"));
            FScope Scope; Scope.IsObject = N == EJsonNotation::ObjectStart; Stack.Add(MoveTemp(Scope));
        }
        else if (End && !Stack.IsEmpty()) Stack.Pop();
    }
    if (!Reader->GetErrorMessage().IsEmpty() || !Stack.IsEmpty())
        return Fail(Error, TEXT("JSON"), TEXT("incomplete or malformed document"));
    Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Out, FJsonSerializer::EFlags::StoreNumbersAsStrings) || !Out.IsValid())
        return Fail(Error, TEXT("JSON"), TEXT("expected an object: ") + Reader->GetErrorMessage());
    return true;
}
bool Object(const TSharedPtr<FJsonValue>& V, FObject& Out, const FString& At, FString& Error)
{
    if (!V.IsValid() || V->Type != EJson::Object) return Fail(Error, At, TEXT("expected an object"));
    Out = V->AsObject(); // Guarded by the exact value type.
    return Out.IsValid() || Fail(Error, At, TEXT("invalid object"));
}
bool Fields(const FObject& O, std::initializer_list<const TCHAR*> Allowed, const FString& At, FString& Error)
{
    for (const auto& Pair : O->Values)
    {
        bool Found = false;
        for (const TCHAR* Key : Allowed) Found |= FString(*Pair.Key).Equals(Key, ESearchCase::CaseSensitive);
        if (!Found) return Fail(Error, At + TEXT(".") + FString(*Pair.Key), TEXT("unknown field"));
    }
    return true;
}
bool String(const FObject& O, const TCHAR* Key, FString& Out, const FString& At, FString& Error, bool Required, bool AllowEmpty)
{
    const auto* V = O->Values.Find(Key);
    if (!V) return !Required || Fail(Error, At + TEXT(".") + Key, TEXT("required string"));
    return (V->IsValid() && (*V)->Type == EJson::String && (*V)->TryGetString(Out) &&
        (AllowEmpty || !Out.TrimStartAndEnd().IsEmpty())) || Fail(Error, At + TEXT(".") + Key, TEXT("invalid string"));
}
bool Array(const FObject& O, const TCHAR* Key, const FArray*& Out, const FString& At, FString& Error, bool Required, int32 Limit)
{
    static const FArray Empty; Out = &Empty;
    if (!O->HasField(Key)) return !Required || Fail(Error, At + TEXT(".") + Key, TEXT("required array"));
    return (O->TryGetArrayField(Key, Out) && Out && Out->Num() <= Limit) ||
        Fail(Error, At + TEXT(".") + Key, TEXT("expected a bounded array"));
}
bool Decimal(const FString& Text, int64& Out, bool Positive)
{
    if (Text.IsEmpty() || (Text.Len() > 1 && Text[0] == '0')) return false;
    int64 Value = 0;
    for (TCHAR C : Text)
    {
        if (C < '0' || C > '9' || Value > (MAX_int64 - (C - '0')) / 10) return false;
        Value = Value * 10 + C - '0';
    }
    Out = Value;
    return !Positive || Value > 0;
}
bool Integer(const FObject& O, const TCHAR* Key, int32& Out, const FString& At, FString& Error, int32 Min, int32 Max, bool Required)
{
    const auto* V = O->Values.Find(Key);
    if (!V) return !Required || Fail(Error, At + TEXT(".") + Key, TEXT("required integer"));
    FString Raw; int64 Magnitude = 0;
    if (!V->IsValid() || (*V)->Type != EJson::Number || !(*V)->TryGetString(Raw))
        return Fail(Error, At + TEXT(".") + Key, TEXT("expected an integral JSON number"));
    const bool Negative = Raw.StartsWith(TEXT("-"));
    if (!Decimal(Negative ? Raw.Mid(1) : Raw, Magnitude) || Magnitude > static_cast<int64>(MAX_int32) + 1)
        return Fail(Error, At + TEXT(".") + Key, TEXT("expected a plain int32 token without fraction or exponent"));
    const int64 Value = Negative ? -Magnitude : Magnitude;
    if (Value < Min || Value > Max) return Fail(Error, At + TEXT(".") + Key, TEXT("integer outside supported range"));
    Out = static_cast<int32>(Value); return true;
}
bool Boolean(const FObject& O, const TCHAR* Key, bool& Out, const FString& At, FString& Error)
{
    const auto* V = O->Values.Find(Key);
    return !V || (V->IsValid() && (*V)->Type == EJson::Boolean && (*V)->TryGetBool(Out)) ||
        Fail(Error, At + TEXT(".") + Key, TEXT("expected a boolean"));
}
bool Id(const FObject& O, const TCHAR* Key, FOGContentId& Out, const FString& At, FString& Error, bool Required)
{
    if (!String(O, Key, Out.Value, At, Error, Required)) return false;
    return (!Required && !O->HasField(Key)) || Out.IsValid() ||
        Fail(Error, At + TEXT(".") + Key, TEXT("expected a stable lowercase namespace:key ID"));
}
bool Token(const FString& Text)
{
    if (Text.IsEmpty() || Text.Len() >= NAME_SIZE || Text.Equals(TEXT("None"), ESearchCase::IgnoreCase)) return false;
    for (TCHAR C : Text)
        if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') ||
            C == '_' || C == '-' || C == '.' || C == ':' || C == '/')) return false;
    return true;
}
bool Name(const FObject& O, const TCHAR* Key, FName& Out, const FString& At, FString& Error, bool Required)
{
    FString Value;
    if (!String(O, Key, Value, At, Error, Required)) return false;
    if (!O->HasField(Key)) return true;
    if (!Token(Value)) return Fail(Error, At + TEXT(".") + Key, TEXT("invalid stable name token"));
    Out = FName(*Value); return true;
}
bool StringMap(const FObject& O, TMap<FString,FString>& Out, const FString& At, FString& Error)
{
    if (O->Values.Num() > 64) return Fail(Error, At, TEXT("too many string predicates"));
    for (const auto& Pair : O->Values)
    {
        FString Value;
        if (!Token(FString(*Pair.Key)) || !Pair.Value.IsValid() || Pair.Value->Type != EJson::String ||
            !Pair.Value->TryGetString(Value) || Value.IsEmpty()) return Fail(Error, At, TEXT("expected stable keys and nonempty string values"));
        Out.Add(FString(*Pair.Key), Value);
    }
    return true;
}
FString Write(const FObject& O)
{
    FString Text; FJsonSerializer::Serialize(O.ToSharedRef(), TJsonWriterFactory<>::Create(&Text)); return Text;
}
}
