#include "Combat/OGCombatMotionPresentation.h"
#include "Runtime/OGFoundationCharacterRuntime.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FOGCombatMotionProfilesNoWeaponAssumption,
    "OfflineGame.Gate12.Combat.MotionProfiles.NoUniversalRifleOrSword",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FOGCombatMotionProfilesNoWeaponAssumption::RunTest(const FString&)
{
    FOGFoundationCharacterProjection Empty;
    TestTrue(TEXT("No actual equipped item does not invent a weapon"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty).IsNone());
    for (const TCHAR* Profile : {
             TEXT("fist"), TEXT("sling"), TEXT("longbow"), TEXT("arcane"),
             TEXT("two_handed_blade"), TEXT("energy_cannon"), TEXT("thrown")})
    {
        const FString State=FString::Printf(
            TEXT("{\"combat_motion_profile\":\"%s\"}"),Profile);
        const FName Resolved=FOGCombatMotionPresentation::ParseProfile(State);
        TestFalse(TEXT("Explicit authored profile accepted"),Resolved.IsNone());
        const FName Action=FOGCombatMotionPresentation::ActionClipId(Resolved, 1, true);
        TestTrue(TEXT("Clip uses exact authored style, no firearm substitution"),
            Action.ToString().Contains(Profile));
    }
    TestTrue(TEXT("Missing profile is not inferred"),
        FOGCombatMotionPresentation::ParseProfile(TEXT("{}")).IsNone());
    TestTrue(TEXT("Reject invalid path"),
        FOGCombatMotionPresentation::ParseProfile(
            TEXT("{\"combat_motion_profile\":\"../../rifle\"}")).IsNone());
    FOGFoundationEquippedItemProjection A;
    A.Item.EvolutionStateJson=TEXT("{\"combat_motion_profile\":\"sling\"}");
    FOGFoundationEquippedItemProjection B;
    B.Item.EvolutionStateJson=TEXT("{\"combat_motion_profile\":\"arcane\"}");
    Empty.Equipment={A};
    TestEqual(TEXT("One real authored equipment motion"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty),
        FName(TEXT("sling")));
    Empty.Equipment.Add(B);
    TestTrue(TEXT("Conflicting profiles do not choose a guessed gun pose"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty).IsNone());
    Empty.Equipment[1].bBroken=true;
    TestEqual(TEXT("Broken equipment does not override compatible motion"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty),
        FName(TEXT("sling")));
    return true;
}
#endif
