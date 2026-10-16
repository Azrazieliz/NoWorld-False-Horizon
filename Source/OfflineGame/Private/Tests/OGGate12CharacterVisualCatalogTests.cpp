#include "UI/OGGate12CharacterVisualCatalog.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12CharacterVisualExactVersionTest,
    "OfflineGame.Gate12.Ruler.VisualAssetsExactVersionAndMissing2D",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12CharacterVisualExactVersionTest::RunTest(const FString&)
{
    UOGGate12CharacterVisualCatalog* Catalog =
        NewObject<UOGGate12CharacterVisualCatalog>(GetTransientPackage());
    if (!TestNotNull(TEXT("Transient catalog created without content installation"), Catalog))
        return false;

    const FOGContentId Identity(TEXT("qa:identity"));
    const FOGContentId Base(TEXT("qa:base"));
    const FOGContentId Alter(TEXT("qa:alter"));
    const FOGContentId Other(TEXT("qa:other"));

    FOGGate12CharacterVisualEntry Entry;
    Entry.IdentityId = Identity;
    Entry.VersionId = Base;
    Catalog->Entries.Add(Entry);

    bool bPassed = true;
    bPassed &= TestNotNull(TEXT("Exact Identity and Version match"),
        Catalog->FindExact(Identity, Base));
    bPassed &= TestNull(TEXT("An Alter never resolves to the base artwork"),
        Catalog->FindExact(Identity, Alter));
    bPassed &= TestNull(TEXT("Another Identity never inherits this art"),
        Catalog->FindExact(Other, Base));
    bPassed &= TestNull(TEXT("Invalid content ID fails closed"),
        Catalog->FindExact(FOGContentId(), Base));

    const FOGGate12CharacterVisualEntry* Found =
        Catalog->FindExact(Identity, Base);
    if (Found)
    {
        bPassed &= TestTrue(TEXT("Original 2D art may be absent safely"),
            Found->OriginalArtwork2D.IsNull());
        bPassed &= TestTrue(TEXT("No arbitrary original art used in Privacy"),
            Found->ApprovedSfwArtwork2D.IsNull());
        bPassed &= TestTrue(TEXT("Wardrobe cannot be written without authored options"),
            Found->WardrobeStateJsonField.IsNone() &&
            Found->ApprovedWardrobeChoices.IsEmpty());
        bPassed &= TestTrue(TEXT("Missing 3D asset never substitutes a mannequin"),
            Found->GenuineCharacterPreviewClass.IsNull());
    }

    // Identical keys from two packages must not silently select the first.
    Catalog->Entries.Add(Entry);
    bPassed &= TestNull(TEXT("Duplicate conflicting mapping fails closed"),
        Catalog->FindExact(Identity, Base));
    return bPassed;
}
#endif
