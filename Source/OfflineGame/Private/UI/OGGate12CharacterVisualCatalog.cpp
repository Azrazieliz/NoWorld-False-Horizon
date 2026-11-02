#include "UI/OGGate12CharacterVisualCatalog.h"

#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Characters/OGCharacterAppearanceDefinition.h"
#include "Characters/OGContentCharacterActors.h"

// Gate01's five 2026-10-20 user-accepted PNG files are original immutable
// 2D art, not separate characters, newly invented character models, or
// replacements for the approved live 3D classes. One identity, five forms.
// The original package manifest and artwork files are shipped unchanged.
namespace
{
struct FOGAcceptedLuminousArt
{
    const TCHAR* VersionId;
    const TCHAR* Filename;
    // Existing installed native actor only; nullptr until the genuine model
    // has actually been imported. This does NOT create a motion library.
    const TCHAR* InstalledNativePreviewClass;
};

// One identity, one shared semantic motion bank, five separate source forms.
// Only Base/Alter/Berserker have native Blueprint packages in this checkout;
// preserve their exact packages and never manufacture the missing ones.
constexpr FOGAcceptedLuminousArt LuminousAcceptedArt[] =
{
    { TEXT("character:luminous_valentine.version_ln_base"), TEXT("01_BASE.png"),
      TEXT("/Game/NoWorld/Characters/Luminous/Base/BP_Luminous_Base_Production.BP_Luminous_Base_Production_C") },
    { TEXT("character:luminous_valentine.version_alter_v1"), TEXT("02_ALTER.png"),
      TEXT("/Game/NoWorld/Characters/Luminous/Alter/BP_Luminous_Alter_Production.BP_Luminous_Alter_Production_C") },
    { TEXT("character:luminous_valentine.version_awakened_v4"), TEXT("03_AWAKENED.png"), nullptr },
    { TEXT("character:luminous_valentine.version_berserker_v4"), TEXT("04_BERSERKER.png"),
      TEXT("/Game/NoWorld/Characters/Luminous/Berserker/BP_Luminous_Berserker_OriginalSourceNative.BP_Luminous_Berserker_OriginalSourceNative_C") },
    { TEXT("character:luminous_valentine.version_grand_v4"), TEXT("05_GRAND_CONVERGENCE.png"), nullptr }
};
}

const FOGGate12CharacterVisualEntry* UOGGate12CharacterVisualCatalog::FindExact(
    const FOGContentId& IdentityId, const FOGContentId& VersionId) const
{
    if (!IdentityId.IsValid() || !VersionId.IsValid()) return nullptr;

    const FOGGate12CharacterVisualEntry* Declared = nullptr;
    for (const FOGGate12CharacterVisualEntry& Candidate : Entries)
    {
        if (Candidate.IdentityId == IdentityId &&
            Candidate.VersionId == VersionId)
        {
            // Never choose arbitrary duplicate definitions.
            if (Declared) return nullptr;
            Declared = &Candidate;
        }
    }

    if (IdentityId != FOGContentId(TEXT("character:luminous_valentine.identity")))
    {
        // Only new exact content-factory registrations are discovered here.
        // Existing user-authored visual catalog entries always take priority.
        if (Declared) return Declared;
        static TMap<FString, FOGGate12CharacterVisualEntry> FactoryEntries;
        const FString Key=IdentityId.ToString()+TEXT("|")+VersionId.ToString();
        if (const FOGGate12CharacterVisualEntry* Cached=FactoryEntries.Find(Key))
            return Cached;
        FString Manifest;
        const FString Source=FPaths::Combine(FPaths::ProjectContentDir(),
            TEXT("NoWorld/ContentFactory/Characters.json"));
        if (!FFileHelper::LoadFileToString(Manifest,*Source)) return nullptr;
        TSharedPtr<FJsonObject> Root;
        if (!FJsonSerializer::Deserialize(
                TJsonReaderFactory<>::Create(Manifest),Root) || !Root) return nullptr;
        FString Schema;
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        if (!Root->TryGetStringField(TEXT("schema"),Schema) ||
            Schema!=TEXT("noworld.content_factory.index.v1") ||
            !Root->TryGetArrayField(TEXT("characters"),Rows) || !Rows) return nullptr;
        TSharedPtr<FJsonObject> Exact;
        for (const auto& Value:*Rows)
        {
            const auto Row=Value.IsValid()?Value->AsObject():nullptr;
            FString Id,Version;
            if(!Row || !Row->TryGetStringField(TEXT("identity"),Id) ||
               !Row->TryGetStringField(TEXT("version"),Version) ||
               Id!=IdentityId.ToString() || Version!=VersionId.ToString()) continue;
            if(Exact) return nullptr; // Reject ambiguous runtime source registry.
            Exact=Row;
        }
        FString Asset;
        if(!Exact || !Exact->TryGetStringField(TEXT("appearance"),Asset) ||
           !Asset.StartsWith(TEXT("/Game/NoWorld/ContentFactory/Appearances/")))
            return nullptr;
        auto* Native=LoadObject<UOGCharacterAppearanceDefinition>(nullptr,*Asset);
        if(!Native || Native->GetPathName()!=Asset ||
           Native->IdentityId!=IdentityId || Native->VersionId!=VersionId)
            return nullptr;
        FString Error;
        if(!Native->ValidateContract(Error)) return nullptr;
        FOGGate12CharacterVisualEntry Entry;
        Entry.IdentityId=IdentityId;
        Entry.VersionId=VersionId;
        Entry.WardrobeStateJsonField=Native->WardrobeStateJsonField;
        for(const auto& Row:Native->Wardrobes)
            Entry.ApprovedWardrobeChoices.Add(Row.WardrobeId);
        FString Artwork;
        if(Exact->TryGetStringField(TEXT("artwork"),Artwork) &&
           Artwork.StartsWith(TEXT("/Game/NoWorld/ContentFactory/Art/")))
            Entry.OriginalArtwork2D=
                TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Artwork));
        // Existing real native generic pawn projects exact owned source
        // meshes and uses the game's canonical actor/battle bindings.
        Entry.ApprovedWorldPlayableClass=
            TSoftClassPtr<AOGWorldPrototypeCharacter>(
                AOGContentPlayableCharacter::StaticClass());
        // Do not fabricate a preview actor, fake ownership, or assume the
        // source 2D artwork is eligible for a privacy/SFW presentation.
        return &FactoryEntries.Add(Key,MoveTemp(Entry));
    }

    const TCHAR* AcceptedFile = nullptr;
    const TCHAR* InstalledNativePreview = nullptr;
    const FString RequestedVersion = VersionId.ToString();
    for (const FOGAcceptedLuminousArt& Form : LuminousAcceptedArt)
        if (RequestedVersion == Form.VersionId)
        {
            AcceptedFile = Form.Filename;
            InstalledNativePreview = Form.InstalledNativePreviewClass;
            break;
        }
    if (!AcceptedFile) return Declared;

    // Keep all pre-existing genuine 3D, wardrobe and world-playable class
    // bindings. Only replace the 2D artwork slots with the exact accepted
    // original files; the five source PNGs are staged unchanged as UFS.
    static TMap<FString, FOGGate12CharacterVisualEntry> AcceptedByVersion;
    if (FOGGate12CharacterVisualEntry* Cached = AcceptedByVersion.Find(RequestedVersion))
        return Cached;

    FOGGate12CharacterVisualEntry Exact = Declared
        ? *Declared : FOGGate12CharacterVisualEntry();
    Exact.IdentityId = IdentityId;
    Exact.VersionId = VersionId;
    Exact.OriginalArtwork2D.Reset();
    Exact.ApprovedSfwArtwork2D.Reset();
    // Bind only genuinely installed preview Blueprints. Original Registry
    // entries take precedence, and World possession still requires a separately
    // verified playable pawn. No substitute mesh and no per-Version animation.
    if (InstalledNativePreview && Exact.GenuineCharacterPreviewClass.IsNull())
        Exact.GenuineCharacterPreviewClass =
            TSoftClassPtr<AActor>(FSoftObjectPath(InstalledNativePreview));
    // Bind original Koikatsu-card-native form sources through one shared
    // character motion system. Grand is an exceptional final manifestation,
    // not a fifth ordinary lineage, and uses its complete source-renderer
    // set rather than the earlier unrelated Grand candidate model.
    if (RequestedVersion ==
            TEXT("character:luminous_valentine.version_awakened_v4") ||
        RequestedVersion ==
            TEXT("character:luminous_valentine.version_berserker_v4") ||
        RequestedVersion ==
            TEXT("character:luminous_valentine.version_grand_v4"))
    {
        Exact.GenuineCharacterPreviewClass =
            TSoftClassPtr<AActor>(FSoftObjectPath(
                TEXT("/Script/OfflineGame.OGLuminousExactOriginalPreviewActor")));
        const TCHAR* NativeWorld = RequestedVersion ==
            TEXT("character:luminous_valentine.version_awakened_v4")
                ? TEXT("/Script/OfflineGame.OGLuminousAwakenedOriginalWorldCharacter")
                : RequestedVersion == TEXT("character:luminous_valentine.version_berserker_v4")
                ? TEXT("/Script/OfflineGame.OGLuminousBerserkerOriginalWorldCharacter")
                : TEXT("/Script/OfflineGame.OGLuminousGrandOriginalWorldCharacter");
        Exact.ApprovedWorldPlayableClass =
            TSoftClassPtr<AOGWorldPrototypeCharacter>(
                FSoftObjectPath(NativeWorld));
    }

    const FString File = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("NoWorld/Characters/Luminous/Art/Accepted2D"), AcceptedFile);
    UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(File);
    if (Texture)
    {
        // A runtime imported transient texture cannot be discovered by
        // Unreal's cooker. The source PNG is deliberately staged as UFS.
        // Keep the texture alive across UMG/Slate widget reconstruction.
        Texture->AddToRoot();
        Exact.OriginalArtwork2D = TSoftObjectPtr<UTexture2D>(Texture);
        // All five accepted compositions are clothed; the same original
        // illustration is valid in both existing presentation settings.
        // This is only a render choice, never an authorization decision.
        Exact.ApprovedSfwArtwork2D = TSoftObjectPtr<UTexture2D>(Texture);
    }
    // Even if staging fails, never silently display rejected earlier art.
    return &AcceptedByVersion.Add(RequestedVersion, MoveTemp(Exact));
}
