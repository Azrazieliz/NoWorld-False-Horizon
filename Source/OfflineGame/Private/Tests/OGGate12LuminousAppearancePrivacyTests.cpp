#include "Koikatsu/OGLuminousV3PlayableCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Persistence/OGSQLiteWorldStore.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGGate12LuminousAppearanceMatrixTest,
    "OfflineGame.Gate12.Luminous.AppearancePrivacyRenderMatrix",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGGate12LuminousAppearanceMatrixTest::RunTest(const FString&)
{
    using Wardrobe = EOGLuminousV3Wardrobe;
    using Policy = FOGLuminousV3AppearancePolicy;

    const auto Base = Policy::Resolve(Wardrobe::Clothed, false, false, true);
    TestTrue(TEXT("Base dressed appearance renders without adult context"), Base.bRenderable);
    TestEqual(TEXT("Actual base"), Base.ActualWardrobe, Wardrobe::Clothed);
    TestEqual(TEXT("Effective base"), Base.EffectiveRenderedWardrobe, Wardrobe::Clothed);
    TestTrue(TEXT("Default identity appearance"), Base.bDefaultBaseAppearance);

    for (const Wardrobe Actual : {Wardrobe::Underwear, Wardrobe::Nude})
    {
        const auto Normal = Policy::Resolve(Actual, false, true, true);
        TestTrue(TEXT("Authorized adult-context asset renders"), Normal.bRenderable);
        TestEqual(TEXT("Actual explicit state remains selected"), Normal.ActualWardrobe, Actual);
        TestEqual(TEXT("Authorized render follows actual"), Normal.EffectiveRenderedWardrobe, Actual);
        TestFalse(TEXT("Authorized explicit state not privacy-masked"), Normal.bMaskedForPrivacyOrUnavailableAdultContext);

        const auto Masked = Policy::Resolve(Actual, true, true, true);
        TestTrue(TEXT("Privacy permits safe substitute"), Masked.bRenderable);
        TestEqual(TEXT("Mask does not equip clothes"), Masked.ActualWardrobe, Actual);
        TestEqual(TEXT("Mask renders nonexplicit clothing"), Masked.EffectiveRenderedWardrobe, Wardrobe::Clothed);
        TestTrue(TEXT("Read-only substitution is identified"), Masked.bMaskedForPrivacyOrUnavailableAdultContext);

        const auto Unmasked = Policy::Resolve(Masked.ActualWardrobe, false, true, true);
        TestEqual(TEXT("Unmask restores actual explicit rendering"), Unmasked.EffectiveRenderedWardrobe, Actual);
        TestEqual(TEXT("Unmask still does not mutate actual clothing"), Unmasked.ActualWardrobe, Actual);

        const auto Unowned = Policy::Resolve(Actual, false, false, true);
        TestEqual(TEXT("Unapproved adult render falls back nonexplicit"), Unowned.EffectiveRenderedWardrobe, Wardrobe::Clothed);
        TestEqual(TEXT("Unapproved context does not rewrite actual"), Unowned.ActualWardrobe, Actual);
        TestTrue(TEXT("Unapproved adult render is identified as masked"), Unowned.bMaskedForPrivacyOrUnavailableAdultContext);

        const auto NoFallback = Policy::Resolve(Actual, true, true, false);
        TestFalse(TEXT("Never display explicit content if privacy fallback absent"), NoFallback.bRenderable);
    }

    const auto Invalid = Policy::Resolve(static_cast<Wardrobe>(255), false, true, true);
    TestFalse(TEXT("Unknown content cannot render"), Invalid.bRenderable);
    TestFalse(TEXT("Default render without safe base cannot show"), Policy::Resolve(Wardrobe::Clothed, false, true, false).bRenderable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGGate12LuminousPrivacyPreservesSqliteTest,
    "OfflineGame.Gate12.Luminous.PrivacyDoesNotMutateCanonicalOutfitOrOwnership",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGGate12LuminousPrivacyPreservesSqliteTest::RunTest(const FString&)
{
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FOGSQLiteWorldStore Store;
    FString Error;
    bool bOk = Store.Open(FPaths::Combine(Directory, TEXT("privacy.db")), Error);
    const FOGEntityId Ruler = FOGEntityId::NewId();
    const FOGEntityId First = FOGEntityId::NewId();
    const FOGEntityId Second = FOGEntityId::NewId();
    const FOGContentId Identity(TEXT("test:identity.same_canonical_character"));
    const FOGContentId Version(TEXT("test:version.single_base"));
    const FString FirstJson =
        TEXT("{\"luminous_v3_wardrobe\":\"NUDE\",\"injury\":3,\"real_equipment\":\"unchanged\"}");
    const FString SecondJson =
        TEXT("{\"luminous_v3_wardrobe\":\"UNDERWEAR\",\"relationship\":\"preserved\"}");

    if (bOk) bOk = Store.UpsertEntity(Ruler, FName(TEXT("ruler")), 0, TEXT("{}"), Error) &&
        Store.UpsertEntity(First, FName(TEXT("character")), 0, TEXT("{}"), Error) &&
        Store.UpsertEntity(Second, FName(TEXT("character")), 0, TEXT("{}"), Error);
    for (int32 Index = 0; bOk && Index < 2; ++Index)
    {
        FOGCharacterManifestationRecord Manifestation;
        Manifestation.ManifestationId = Index == 0 ? First : Second;
        Manifestation.OwningRulerId = Ruler;
        Manifestation.IdentityId = Identity;
        Manifestation.ActiveVersionId = Version;
        Manifestation.AcquisitionOrdinal = Index;
        bOk = Store.UpsertCharacterManifestation(Manifestation, Index, Error);
        if (!bOk) break;
        FOGManifestationPresentationStateRecord Appearance;
        Appearance.ManifestationId = Manifestation.ManifestationId;
        Appearance.OutfitStateJson = Index == 0 ? FirstJson : SecondJson;
        Appearance.PresentationVariantStateJson = TEXT("{\"non_clothing_world_state\":\"retained\"}");
        bOk = Store.UpsertManifestationPresentationState(Appearance, Error);
    }
    if (!TestTrue(TEXT("Isolated SQLite real-state fixture"), bOk))
    {
        if (!Error.IsEmpty()) AddError(Error);
        Store.Close(); IFileManager::Get().DeleteDirectory(*Directory, false, true);
        return false;
    }

    TArray<FOGCharacterManifestationRecord> OwnedBefore;
    bOk = Store.ListCharacterManifestationsByOwner(Ruler, OwnedBefore, Error);
    TestTrue(TEXT("Roster is real SQLite, not cosmetic duplicates"), bOk);
    TestEqual(TEXT("Two owned instances, one identity and one base version"), OwnedBefore.Num(), 2);
    // Rendering calculation is intentionally read-only. Neither a privacy
    // toggle nor a restored unmasked representation calls any Store mutation.
    const auto MaskFirst = FOGLuminousV3AppearancePolicy::Resolve(
        EOGLuminousV3Wardrobe::Nude, true, true, true);
    const auto MaskSecond = FOGLuminousV3AppearancePolicy::Resolve(
        EOGLuminousV3Wardrobe::Underwear, true, true, true);
    const auto RestoreFirst = FOGLuminousV3AppearancePolicy::Resolve(
        MaskFirst.ActualWardrobe, false, true, true);
    TestEqual(TEXT("First instance remains actually nude while masked"), MaskFirst.ActualWardrobe, EOGLuminousV3Wardrobe::Nude);
    TestEqual(TEXT("Second instance remains actually underwear"), MaskSecond.ActualWardrobe, EOGLuminousV3Wardrobe::Underwear);
    TestEqual(TEXT("Unmasked first returns to its own clothing"), RestoreFirst.EffectiveRenderedWardrobe, EOGLuminousV3Wardrobe::Nude);

    bool bFound = false;
    FOGManifestationPresentationStateRecord FirstAfter;
    FOGManifestationPresentationStateRecord SecondAfter;
    bOk = Store.TryReadManifestationPresentationState(First, bFound, FirstAfter, Error) && bFound;
    TestTrue(TEXT("First owned Manifestation presentation preserved"), bOk);
    if (bOk) TestEqual(TEXT("First original outfit/equipment JSON byte-equal"), FirstAfter.OutfitStateJson, FirstJson);
    bFound = false;
    bOk = Store.TryReadManifestationPresentationState(Second, bFound, SecondAfter, Error) && bFound;
    TestTrue(TEXT("Second owned Manifestation presentation preserved"), bOk);
    if (bOk) TestEqual(TEXT("Second original state JSON byte-equal"), SecondAfter.OutfitStateJson, SecondJson);
    TestEqual(TEXT("Interaction/presentation variant unchanged"),
        SecondAfter.PresentationVariantStateJson, FString(TEXT("{\"non_clothing_world_state\":\"retained\"}")));

    TArray<FOGCharacterManifestationRecord> OwnedAfter;
    bOk = Store.ListCharacterManifestationsByOwner(Ruler, OwnedAfter, Error);
    TestTrue(TEXT("Ownership read after toggling"), bOk);
    TestEqual(TEXT("No added gacha/Manifestation records"), OwnedAfter.Num(), OwnedBefore.Num());
    if (bOk)
    {
        for (const FOGCharacterManifestationRecord& Actual : OwnedAfter)
        {
            TestTrue(TEXT("No identity, base version or owner change"),
                Actual.IdentityId == Identity && Actual.ActiveVersionId == Version &&
                Actual.OwningRulerId == Ruler);
        }
    }

    Store.Close();
    IFileManager::Get().DeleteDirectory(*Directory, false, true);
    return true;
}
#endif
