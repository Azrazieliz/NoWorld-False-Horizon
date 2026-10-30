#if WITH_DEV_AUTOMATION_TESTS
#include "World/OGWorldSimulationReceiver.h"
#include "Misc/AutomationTest.h"

struct FOGWorldSimulationBodyTestAccess
{
    static bool Owns(uint32 Request,bool Arrival,uint32 Completed,uint32 Current,bool Valid,bool Idle)
    { return UOGWorldSimulationReceiver::OwnsCheckpointRequest(Request,Arrival,Completed,Current,Valid,Idle); }
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGWorldBodyCompletedRequestFence,
    "OfflineGame.World.NativeBody.CompletedRequestOwnership",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGWorldBodyCompletedRequestFence::RunTest(const FString&)
{
    TestTrue(TEXT("Active accepted native request can checkpoint"),
        FOGWorldSimulationBodyTestAccess::Owns(17,false,0,17,true,false));
    TestFalse(TEXT("Active foreign request cannot checkpoint"),
        FOGWorldSimulationBodyTestAccess::Owns(17,false,0,18,true,false));
    TestTrue(TEXT("Completed accepted request may checkpoint while follower retains its ID"),
        FOGWorldSimulationBodyTestAccess::Owns(0,true,17,17,true,true));
    TestTrue(TEXT("Completed accepted request may checkpoint after follower clears its ID"),
        FOGWorldSimulationBodyTestAccess::Owns(0,true,17,0,false,true));
    TestFalse(TEXT("A later running or paused owner cannot donate its position"),
        FOGWorldSimulationBodyTestAccess::Owns(0,true,17,18,true,false));
    TestFalse(TEXT("A later completed owner cannot donate its position"),
        FOGWorldSimulationBodyTestAccess::Owns(0,true,17,18,true,true));
    TestFalse(TEXT("No completion evidence cannot checkpoint an idle body"),
        FOGWorldSimulationBodyTestAccess::Owns(0,false,17,0,false,true));
    TestFalse(TEXT("Unknown completion cannot claim another retained native request"),
        FOGWorldSimulationBodyTestAccess::Owns(0,true,0,18,true,true));
    return true;
}
#endif
