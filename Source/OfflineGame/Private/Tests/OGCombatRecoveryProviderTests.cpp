#if WITH_DEV_AUTOMATION_TESTS
#include "Runtime/OGGameCoreSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGCombatRecoveryProviderRegistrationTest,
    "OfflineGame.Combat.Recovery.NativeFactoryRegistration",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGCombatRecoveryProviderRegistrationTest::RunTest(const FString&)
{
    auto* Game=NewObject<UGameInstance>(); auto* Core=NewObject<UOGGameCoreSubsystem>(Game); FString Error;
    FOGCombatRecoveryProvider Provider; Provider.Id=TEXT("test.callback_factory"); Provider.Version=2;
    Provider.Capture=[](const auto&,auto&,const auto&,FString& Payload,FString&){Payload=TEXT("{}");return true;};
    Provider.Restore=[](auto&,const auto&,int32,const FString&,auto&,FString&){return true;};
    auto Invalid=Provider; Invalid.MinimumReadableVersion=3;
    TestFalse(TEXT("Factory cannot claim an inverted migration range"),Core->RegisterCombatRecoveryProvider(Invalid,Error));
    Invalid=Provider; Invalid.Id=TEXT("core.luminous");
    TestFalse(TEXT("Extensions cannot replace the canonical built-in factory"),Core->RegisterCombatRecoveryProvider(Invalid,Error));
    TestTrue(TEXT("Installed native extension registers once"),Core->RegisterCombatRecoveryProvider(Provider,Error));
    TestFalse(TEXT("A duplicate factory cannot replace live native code"),Core->RegisterCombatRecoveryProvider(Provider,Error));
    TestTrue(TEXT("An unused extension can release its native factory"),Core->UnregisterCombatRecoveryProvider(Provider.Id,Error));
    return true;
}
#endif
