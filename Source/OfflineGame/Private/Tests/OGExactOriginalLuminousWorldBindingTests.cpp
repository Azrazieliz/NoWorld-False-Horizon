#include "Koikatsu/OGExactOriginalLuminousParts.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"
#include "Koikatsu/OGLuminousExactOriginalPreviewActor.h"
#include "UI/OGGate12CharacterPreviewConfiguration.h"
#include "UI/OGGate12OwnedWorldCharacterBinding.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGGate12TrueOriginalLuminousModelsTest,
    "OfflineGame.Gate12.Luminous.TrueOriginalModelsAndFullWorldBinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGGate12TrueOriginalLuminousModelsTest::RunTest(const FString&)
{
    bool Passed = true;
    for (const TPair<FString, int32>& Form :
        { TPair<FString,int32>(TEXT("character:luminous_valentine.version_awakened_v4"),1073),
          TPair<FString,int32>(TEXT("character:luminous_valentine.version_berserker_v4"),882) })
    {
        TArray<FSoftObjectPath> Paths;
        int32 Bones = 0;
        Passed &= TestTrue(TEXT("Exact approved-card source form installed in native registry"),
            FOGExactOriginalLuminousParts::GetClothed(Form.Key,Paths,Bones));
        Passed &= TestEqual(TEXT("Nine exact source parts for entire clothed character"),Paths.Num(),9);
        Passed &= TestEqual(TEXT("Actual original-card Blender source skeleton recorded"),Bones,Form.Value);
        TSet<FString> Unique;
        for (const FSoftObjectPath& Path : Paths)
        {
            const FString Full = Path.ToString();
            Passed &= TestTrue(TEXT("Authentic form-specific source, not historical candidate"),
                Full.StartsWith(Form.Value==1073
                    ? TEXT("/Game/NoWorld/Characters/Luminous/Awakened/ExactOriginalClothed/")
                    : TEXT("/Game/NoWorld/Characters/Luminous/Berserker/ExactOriginalClothed/")));
            Passed &= TestFalse(TEXT("No duplicate source part attached"),Unique.Contains(Full));
            Unique.Add(Full);
        }
    }
    TArray<FSoftObjectPath> Rejected;
    int32 RejectedBones = -1;
    Passed &= TestFalse(TEXT("Grand may not borrow another native source"),
        FOGExactOriginalLuminousParts::GetClothed(
            TEXT("character:luminous_valentine.version_grand_v4"),Rejected,RejectedBones));
    Passed &= TestTrue(TEXT("Grand has no substitute native parts"),Rejected.IsEmpty());
    Passed &= TestTrue(TEXT("Awakened World pawn retains real canonical binding"),
        AOGLuminousAwakenedOriginalWorldCharacter::StaticClass()->ImplementsInterface(
            UOGGate12OwnedWorldCharacterBinding::StaticClass()));
    Passed &= TestTrue(TEXT("Berserker World pawn retains real canonical binding"),
        AOGLuminousBerserkerOriginalWorldCharacter::StaticClass()->ImplementsInterface(
            UOGGate12OwnedWorldCharacterBinding::StaticClass()));
    Passed &= TestTrue(TEXT("Roster 3D preview is non-pawn, separate from World"),
        !AOGLuminousExactOriginalPreviewActor::StaticClass()->IsChildOf(
            AOGLuminousV3PlayableCharacter::StaticClass()));
    Passed &= TestTrue(TEXT("Roster 3D preview validates existing owned Manifestation"),
        AOGLuminousExactOriginalPreviewActor::StaticClass()->ImplementsInterface(
            UOGGate12CharacterPreviewConfiguration::StaticClass()));
    return Passed;
}
#endif
