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
    TestEqual(TEXT("Explicit unique ultimate key is not a skill-chain pose"),
        FOGCombatMotionPresentation::ActionClipId(FName(TEXT("arcane")),0,
            false,true), FName(TEXT("CombatUltimate.arcane")));
    TestEqual(TEXT("Sling sound is not a generic gun event"),
        FOGCombatMotionPresentation::SemanticActionSoundId(
            FName(TEXT("sling")),true),FName(TEXT("gameplay.attack.sling")));
    TestEqual(TEXT("Magic ultimate uses its own semantic SFX event"),
        FOGCombatMotionPresentation::SemanticActionSoundId(
            FName(TEXT("arcane")),false,true),
        FName(TEXT("gameplay.ultimate.arcane")));
    TestTrue(TEXT("No profile cannot invent audio"),
        FOGCombatMotionPresentation::SemanticActionSoundId(
            NAME_None,true).IsNone());
    TestTrue(TEXT("Missing profile is not inferred"),
        FOGCombatMotionPresentation::ParseProfile(TEXT("{}")).IsNone());
    TestTrue(TEXT("Reject invalid path"),
        FOGCombatMotionPresentation::ParseProfile(
            TEXT("{\"combat_motion_profile\":\"../../rifle\"}")).IsNone());
    // Canonical innate unarmed/magic/atypical anatomy may have its own
    // explicitly-authored clip without an invented equipped weapon.
    Empty.EntityStateJson=TEXT("{\"combat_motion_profile\":\"innate_arcane\"}");
    TestEqual(TEXT("Explicit canonical innate profile selected without gear"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty),
        FName(TEXT("innate_arcane")));
    FOGFoundationEquippedItemProjection A;
    A.Item.EvolutionStateJson=TEXT("{\"combat_motion_profile\":\"sling\"}");
    FOGFoundationEquippedItemProjection B;
    B.Item.EvolutionStateJson=TEXT("{\"combat_motion_profile\":\"arcane\"}");
    Empty.Equipment={A};
    TestEqual(TEXT("One real authored equipment motion"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty),
        FName(TEXT("sling")));
    Empty.Equipment.Add(B);
    TestTrue(TEXT("Conflicting equipment cannot fall back to innate"),

        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty).IsNone());
    Empty.Equipment[1].bBroken=true;
    TestEqual(TEXT("Broken equipment does not override compatible motion"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty),
        FName(TEXT("sling")));
    Empty.Equipment.Reset();
    Empty.EntityStateJson=TEXT("{\"combat_motion_profile\":\"unarmed/notsafe\"}");
    TestTrue(TEXT("Malformed innate style does not create an action"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty).IsNone());
    Empty.EntityStateJson=TEXT("{}");
    TestTrue(TEXT("No innate or equipped style remains neutral"),
        FOGCombatMotionPresentation::ResolveAuthoredProfile(Empty).IsNone());
    return true;
}
#endif
