#include "UI/OGGate12CharacterPresentationPreferences.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Runtime/OGPlayerProfileSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12CharacterPresentationPersistenceTest,
    "OfflineGame.Gate12.Ruler.CharacterPresentationPreferencePersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12CharacterPresentationPersistenceTest::RunTest(const FString&)
{
    const FString Dir = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Dir, true);
    const FString Path = FPaths::Combine(Dir, TEXT("Profile.json"));

    const FOGEntityId Owner = FOGEntityId::NewId();
    const FOGEntityId OtherOwner = FOGEntityId::NewId();
    const FOGEntityId Instance = FOGEntityId::NewId();
    const FOGEntityId OtherInstance = FOGEntityId::NewId();
    const FOGContentId Identity(TEXT("qa:base_identity"));
    const FOGContentId OtherIdentity(TEXT("qa:other_identity"));
    FString Error;
    bool bPassed = true;
    FOGPlayerProfileSettings Initial;
    Initial.MasterVolume = 0.42f;
    Initial.bSfwPresentation = true;
    bPassed &= TestTrue(TEXT("Create isolated profile"),
        FOGPlayerProfileSettingsService::Save(Path, Initial, Error));

    bool b3D = true;
    bPassed &= TestTrue(TEXT("Read missing preference"),
        FOGGate12CharacterPresentationPreferences::Load(
            Path, Owner, Identity, Instance, b3D, Error));
    bPassed &= TestFalse(TEXT("Default is 2D"), b3D);
    bPassed &= TestTrue(TEXT("Save owned per-copy 3D preference"),
        FOGGate12CharacterPresentationPreferences::Save(
            Path, Owner, Identity, Instance, true, Error));
    b3D = false;
    bPassed &= TestTrue(TEXT("Reload persisted 3D preference"),
        FOGGate12CharacterPresentationPreferences::Load(
            Path, Owner, Identity, Instance, b3D, Error));
    bPassed &= TestTrue(TEXT("3D persisted"), b3D);

    b3D = true;
    bPassed &= TestTrue(TEXT("Another independently owned copy reads"),
        FOGGate12CharacterPresentationPreferences::Load(
            Path, Owner, Identity, OtherInstance, b3D, Error));
    bPassed &= TestFalse(TEXT("Another copy is independently 2D"), b3D);
    b3D = true;
    bPassed &= TestTrue(TEXT("Different owner reads"),
        FOGGate12CharacterPresentationPreferences::Load(
            Path, OtherOwner, Identity, Instance, b3D, Error));
    bPassed &= TestFalse(TEXT("Other owner never inherits 3D"), b3D);
    b3D = true;
    bPassed &= TestTrue(TEXT("Different Identity reads"),
        FOGGate12CharacterPresentationPreferences::Load(
            Path, Owner, OtherIdentity, Instance, b3D, Error));
    bPassed &= TestFalse(TEXT("Other identity never inherits 3D"), b3D);

    FOGPlayerProfileSettings Preserved;
    bPassed &= TestTrue(TEXT("Reload full profile without clobbering"),
        FOGPlayerProfileSettingsService::Load(Path, Preserved, Error));
    bPassed &= TestTrue(TEXT("SFW setting remains physically unchanged"),
        Preserved.bSfwPresentation);
    bPassed &= TestTrue(TEXT("Audio preference retained"),
        FMath::IsNearlyEqual(Preserved.MasterVolume, 0.42f, 0.001f));
    bPassed &= TestFalse(TEXT("Invalid Manifestation preference refused"),
        FOGGate12CharacterPresentationPreferences::Save(
            Path, Owner, Identity, FOGEntityId(), true, Error));

    IFileManager::Get().DeleteDirectory(*Dir, false, true);
    return bPassed;
}
#endif
