#include "UI/OGGate12CharacterVisualCatalog.h"

#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

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
};

// These are the existing content-registry Version IDs, not a second animation
// definition. Motions still bind once to the canonical character identity.
constexpr FOGAcceptedLuminousArt LuminousAcceptedArt[] =
{
    { TEXT("character:luminous_valentine.version_ln_base"), TEXT("01_BASE.png") },
    { TEXT("character:luminous_valentine.version_alter_v1"), TEXT("02_ALTER.png") },
    { TEXT("character:luminous_valentine.version_awakened_v4"), TEXT("03_AWAKENED.png") },
    { TEXT("character:luminous_valentine.version_berserker_v4"), TEXT("04_BERSERKER.png") },
    { TEXT("character:luminous_valentine.version_grand_v4"), TEXT("05_GRAND_CONVERGENCE.png") }
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
        return Declared;

    const TCHAR* AcceptedFile = nullptr;
    const FString RequestedVersion = VersionId.ToString();
    for (const FOGAcceptedLuminousArt& Form : LuminousAcceptedArt)
        if (RequestedVersion == Form.VersionId)
        {
            AcceptedFile = Form.Filename;
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
