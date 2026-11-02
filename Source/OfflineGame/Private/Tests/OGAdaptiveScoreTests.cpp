#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"
#include "Runtime/OGAdaptiveScoreComponent.h"
#include "Sound/SoundWave.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGAdaptiveScoreSelectionTest,
    "OfflineGame.Audio.AdaptiveScore.AuthoredPriorityAndCanonicalNamespace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOGAdaptiveScoreSelectionTest::RunTest(const FString& Parameters)
{
    UOGAdaptiveScoreDefinition* Score = NewObject<UOGAdaptiveScoreDefinition>();
    FOGAdaptiveScoreRule World;
    World.RuleId = TEXT("world");
    World.RequiredTags = { FName(TEXT("mode.world")) };
    World.Audio.Category = EOGSemanticAudioCategory::Music;
    World.Audio.Sound = NewObject<USoundWave>(); // isolated validator fixture, never admitted as game content
    FOGAdaptiveScoreRule Combat = World;
    Combat.RuleId = TEXT("combat");
    Combat.Priority = 10;
    Combat.RequiredTags.Add(TEXT("battle.running"));
    FOGAdaptiveScoreRule Local = World;
    Local.RuleId = TEXT("biome");
    Local.Priority = 10;
    Local.RequiredTags.Add(TEXT("biome.test"));
    Local.ExcludedTags.Add(TEXT("battle.running"));
    Score->Rules = { Combat, Local, World };
    FString Error;
    TestTrue(TEXT("Authored real music rules validate"), Score->Validate(Error));
    const FOGAdaptiveScoreRule* Chosen = Score->Select({ FName(TEXT("mode.world")) });
    TestTrue(TEXT("World context chooses matching world score"), Chosen && Chosen->RuleId == World.RuleId);
    Chosen = Score->Select({ FName(TEXT("mode.world")), FName(TEXT("biome.test")) });
    TestTrue(TEXT("Specific authored biome priority wins"), Chosen && Chosen->RuleId == Local.RuleId);
    Chosen = Score->Select({ FName(TEXT("mode.world")), FName(TEXT("biome.test")), FName(TEXT("battle.running")) });
    TestTrue(TEXT("Actual battle context vetoes excluded biome"), Chosen && Chosen->RuleId == Combat.RuleId);
    TestNull(TEXT("No rule invents Ruler music"), Score->Select({ FName(TEXT("mode.ruler")) }));
    Score->Rules[1].ExcludedTags.Reset();
    Chosen = Score->Select({ FName(TEXT("mode.world")), FName(TEXT("biome.test")), FName(TEXT("battle.running")) });
    TestTrue(TEXT("Ties resolve by lexical RuleId"), Chosen && Chosen->RuleId == Local.RuleId);
    Algo::Reverse(Score->Rules);
    Chosen = Score->Select({ FName(TEXT("mode.world")), FName(TEXT("biome.test")), FName(TEXT("battle.running")) });
    TestTrue(TEXT("Reordering content does not change score selection"), Chosen && Chosen->RuleId == Local.RuleId);
    Score->Rules[0].ExcludedTags.Add(TEXT("mode.world"));
    TestFalse(TEXT("Contradictory authored rule rejected"), Score->Validate(Error));
    FOGAdaptiveScoreRule Ruler = World;
    Ruler.RuleId = TEXT("ruler");
    Ruler.RequiredTags = { FName(TEXT("mode.ruler")) };
    Ruler.Audio.bWorldOnly = false;
    FOGAdaptiveScoreRule WorldOnly = World;
    WorldOnly.RequiredTags.Reset();
    WorldOnly.Priority = 100;
    Score->Rules = { WorldOnly, Ruler };
    Chosen = Score->Select({ FName(TEXT("mode.ruler")) }, false);
    TestTrue(TEXT("World-only rule is filtered before priority, retaining valid Ruler music"),
        Chosen && Chosen->RuleId == Ruler.RuleId);

    UOGAdaptiveScoreComponent* Director = NewObject<UOGAdaptiveScoreComponent>();
    TestFalse(TEXT("Presentation provider cannot fabricate battle state"),
        Director->SetContextTags(TEXT("weather"), { FName(TEXT("battle.running")) }));
    TestFalse(TEXT("Presentation provider cannot fabricate mode state"),
        Director->SetContextTags(TEXT("weather"), { FName(TEXT("Mode.Ruler")) }));
    TestTrue(TEXT("Actual provider may publish its own semantic context"),
        Director->SetContextTags(TEXT("weather"), { FName(TEXT("weather.rain")) }));
    return true;
}

#endif
