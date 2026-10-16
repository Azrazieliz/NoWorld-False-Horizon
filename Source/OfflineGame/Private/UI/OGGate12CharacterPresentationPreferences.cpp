#include "UI/OGGate12CharacterPresentationPreferences.h"

#include "Runtime/OGPlayerProfileSettings.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
FString PreferenceKey(
    const FOGEntityId& RulerId,
    const FOGContentId& IdentityId,
    const FOGEntityId& ManifestationId)
{
    return RulerId.ToString() + TEXT("|") +
        IdentityId.ToString() + TEXT("|") +
        ManifestationId.ToString();
}

bool ReadPreferenceMap(
    const FString& Json,
    TSharedPtr<FJsonObject>& OutRoot,
    FString& OutError)
{
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(Json.IsEmpty() ? TEXT("{}") : Json);
    if (!FJsonSerializer::Deserialize(Reader, OutRoot) || !OutRoot.IsValid())
    {
        OutError = TEXT("Character presentation preference JSON is invalid.");
        return false;
    }
    return true;
}
}

FString FOGGate12CharacterPresentationPreferences::MakeKey(
    const FOGEntityId& RulerId,
    const FOGContentId& IdentityId,
    const FOGEntityId& ManifestationId)
{
    return PreferenceKey(RulerId, IdentityId, ManifestationId);
}

bool FOGGate12CharacterPresentationPreferences::LoadAll(
    const FString& ProfilePath,
    TMap<FString, bool>& OutPreferences,
    FString& OutError)
{
    OutPreferences.Reset();
    OutError.Reset();
    FOGPlayerProfileSettings Settings;
    if (!FOGPlayerProfileSettingsService::Load(ProfilePath, Settings, OutError))
        return false;

    TSharedPtr<FJsonObject> Root;
    if (!ReadPreferenceMap(
        Settings.CharacterPresentationPreferencesJson, Root, OutError))
        return false;
    for (const auto& Entry : Root->Values)
    {
        if (Entry.Value.IsValid() && Entry.Value->Type == EJson::Boolean)
            OutPreferences.Add(Entry.Key, Entry.Value->AsBool());
    }
    return true;
}

bool FOGGate12CharacterPresentationPreferences::Load(
    const FString& ProfilePath,
    const FOGEntityId& RulerId,
    const FOGContentId& IdentityId,
    const FOGEntityId& ManifestationId,
    bool& bOutPrefer3D,
    FString& OutError)
{
    bOutPrefer3D = false;
    OutError.Reset();
    if (!RulerId.IsValid() || !IdentityId.IsValid() ||
        !ManifestationId.IsValid())
    {
        OutError = TEXT("Character presentation requires a valid owned character context.");
        return false;
    }
    FOGPlayerProfileSettings Settings;
    if (!FOGPlayerProfileSettingsService::Load(ProfilePath, Settings, OutError))
        return false;

    TSharedPtr<FJsonObject> Root;
    if (!ReadPreferenceMap(Settings.CharacterPresentationPreferencesJson,
        Root, OutError))
        return false;
    Root->TryGetBoolField(
        PreferenceKey(RulerId, IdentityId, ManifestationId), bOutPrefer3D);
    return true;
}

bool FOGGate12CharacterPresentationPreferences::Save(
    const FString& ProfilePath,
    const FOGEntityId& RulerId,
    const FOGContentId& IdentityId,
    const FOGEntityId& ManifestationId,
    bool bPrefer3D,
    FString& OutError)
{
    OutError.Reset();
    if (!RulerId.IsValid() || !IdentityId.IsValid() ||
        !ManifestationId.IsValid())
    {
        OutError = TEXT("Refusing to store an invalid character presentation preference.");
        return false;
    }
    FOGPlayerProfileSettings Settings;
    if (!FOGPlayerProfileSettingsService::Load(ProfilePath, Settings, OutError))
        return false;
    TSharedPtr<FJsonObject> Root;
    if (!ReadPreferenceMap(Settings.CharacterPresentationPreferencesJson,
        Root, OutError))
        return false;

    Root->SetBoolField(
        PreferenceKey(RulerId, IdentityId, ManifestationId), bPrefer3D);
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer =
        TJsonWriterFactory<>::Create(&Json);
    if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
    {
        OutError = TEXT("Could not serialize character presentation preferences.");
        return false;
    }
    Settings.CharacterPresentationPreferencesJson = MoveTemp(Json);
    // Preserve all existing volume, orientation, accessibility and SFW choices.
    return FOGPlayerProfileSettingsService::Save(ProfilePath, Settings, OutError);
}
