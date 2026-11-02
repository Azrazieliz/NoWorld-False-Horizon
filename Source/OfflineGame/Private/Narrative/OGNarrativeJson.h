#pragma once

#include "Dom/JsonObject.h"
#include "Content/OGContentId.h"
#include <initializer_list>

namespace OGNarrative
{
using FObject = TSharedPtr<FJsonObject>;
using FArray = TArray<TSharedPtr<FJsonValue>>;
bool Fail(FString& Error, const FString& At, const FString& Message);
bool ReadObject(const FString& Json, FObject& Out, FString& Error);
bool Object(const TSharedPtr<FJsonValue>& Value, FObject& Out, const FString& At, FString& Error);
bool Fields(const FObject& O, std::initializer_list<const TCHAR*> Allowed, const FString& At, FString& Error);
bool String(const FObject& O, const TCHAR* Key, FString& Out, const FString& At, FString& Error,
    bool Required = true, bool AllowEmpty = false);
bool Array(const FObject& O, const TCHAR* Key, const FArray*& Out, const FString& At, FString& Error,
    bool Required = false, int32 Limit = 1000);
bool Integer(const FObject& O, const TCHAR* Key, int32& Out, const FString& At, FString& Error,
    int32 Min, int32 Max, bool Required = false);
bool Boolean(const FObject& O, const TCHAR* Key, bool& Out, const FString& At, FString& Error);
bool Id(const FObject& O, const TCHAR* Key, FOGContentId& Out, const FString& At, FString& Error,
    bool Required = true);
bool Name(const FObject& O, const TCHAR* Key, FName& Out, const FString& At, FString& Error, bool Required = true);
bool Token(const FString& Text);
bool Decimal(const FString& Text, int64& Out, bool Positive = false);
bool StringMap(const FObject& O, TMap<FString,FString>& Out, const FString& At, FString& Error);
FString Write(const FObject& O);
}
