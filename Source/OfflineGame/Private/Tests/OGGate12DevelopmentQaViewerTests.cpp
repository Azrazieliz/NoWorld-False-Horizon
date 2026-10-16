#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "UI/OGGate12CharacterPreviewStage.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12DevelopmentSourceViewerIsolationTest,
    "OfflineGame.Gate12.Ruler.DevelopmentSourceViewerIsExactAndNoncanonical",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12DevelopmentSourceViewerIsolationTest::RunTest(const FString&)
{
    bool bPassed = true;
    bPassed &= TestFalse(TEXT("Null QA preview class is rejected"),
        AOGGate12CharacterPreviewStage::IsDevelopmentQaClassAllowed(nullptr));
    bPassed &= TestFalse(TEXT("Arbitrary Actor is not accepted as QA source"),
        AOGGate12CharacterPreviewStage::IsDevelopmentQaClassAllowed(
            AActor::StaticClass()));
    bPassed &= TestFalse(TEXT("Native Luminous class is not a specific approved QA package"),
        AOGGate12CharacterPreviewStage::IsDevelopmentQaClassAllowed(
            AOGLuminousV3PlayableCharacter::StaticClass()));

    UClass* AuthoredQa = LoadClass<AActor>(nullptr,
        TEXT("/Game/Experimental/LuminousValentineV3/RuntimeQA_20261011/"
             "BP_LuminousValentineV3_CLOTHED_RuntimeQA."
             "BP_LuminousValentineV3_CLOTHED_RuntimeQA_C"));
#if UE_BUILD_SHIPPING
    bPassed &= TestFalse(TEXT("Noncanonical QA source cannot be shown in shipping"),
        AOGGate12CharacterPreviewStage::IsDevelopmentQaClassAllowed(AuthoredQa));
#else
    bPassed &= TestNotNull(TEXT("Existing real 3D QA Blueprint source is installed"), AuthoredQa);
    if (AuthoredQa)
        bPassed &= TestTrue(TEXT("Only exact development source class is allowed"),
            AOGGate12CharacterPreviewStage::IsDevelopmentQaClassAllowed(AuthoredQa));
#endif
    // Preview stage initialization never receives a canonical Manifestation
    // and does not call any Registry, roster or SQLite ownership grant.
    return bPassed;
}
#endif
