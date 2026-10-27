#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/OGInteractionDefinition.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionDefinitionContractTest,
    "OfflineGame.Interaction.NativeDefinition.SourceRolesAndTransitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGInteractionDefinitionContractTest::RunTest(const FString& Parameters)
{
    auto* Definition = NewObject<UOGInteractionDefinition>();
    Definition->GraphId = FOGContentId(TEXT("test:interaction.definition"));
    Definition->SourceRevision = TEXT("test-fixture-only");
    Definition->DisplayName = FText::FromString(TEXT("Source fixture"));
    Definition->Roles = {TEXT("player"), TEXT("partner")};
    Definition->StartNode = TEXT("first");
    FOGInteractionNativeNode First; First.NodeId = TEXT("first");
    First.ActionId = FOGContentId(TEXT("test:interaction.first"));
    First.NextNodes = {TEXT("second")};
    FOGInteractionNativeNode Second; Second.NodeId = TEXT("second");
    Second.ActionId = FOGContentId(TEXT("test:interaction.second"));
    Second.RequiredRoles = {TEXT("player"), TEXT("partner")};
    Second.SceneAction = TEXT("Test.Scene"); Second.SceneContext = TEXT("Test");
    for (FName Role : Second.RequiredRoles)
    {
        FOGInteractionNativeTrack Track; Track.Role = Role; Track.SourceSceneRole = Role;
        Second.Tracks.Add(Track);
    }
    Definition->Nodes = {First, Second};
    FOGInteractionActionGraph Graph; FString Error;
    TestTrue(TEXT("Original role tracks and linked nodes compile"), Definition->BuildGraph(Graph, Error));
    TestEqual(TEXT("No fabricated consequences"), Graph.Nodes[0].Consequences.PresentationPatches.Num(), 0);
    Definition->Nodes[1].Tracks.RemoveAt(1);
    TestFalse(TEXT("Missing original scene role rejected"), Definition->BuildGraph(Graph, Error));
    TestEqual(TEXT("Invalid definition publishes no partial graph"), Graph.Nodes.Num(), 0);
    Definition->Nodes[1] = Second;
    Definition->Nodes[0].NextNodes = {TEXT("missing")};
    TestFalse(TEXT("Dangling transition rejected"), Definition->BuildGraph(Graph, Error));
    Definition->Nodes[0].NextNodes.Reset();
    TestFalse(TEXT("Unreachable production node rejected"), Definition->BuildGraph(Graph, Error));
    Definition->Nodes[0] = First;
    Definition->Nodes[1].Tracks.Add(Definition->Nodes[1].Tracks[0]);
    TestFalse(TEXT("Duplicate source selector rejected"), Definition->BuildGraph(Graph, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionDefinitionArityTest,
    "OfflineGame.Interaction.NativeDefinition.NoFixedParticipantCeiling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGInteractionDefinitionArityTest::RunTest(const FString& Parameters)
{
    auto* Definition = NewObject<UOGInteractionDefinition>();
    Definition->GraphId = FOGContentId(TEXT("test:interaction.large"));
    Definition->SourceRevision = TEXT("test-fixture-only");
    Definition->DisplayName = FText::FromString(TEXT("Large source fixture"));
    Definition->Roles.Add(TEXT("player")); Definition->StartNode = TEXT("start");
    FOGInteractionNativeNode Node; Node.NodeId = TEXT("start");
    Node.ActionId = FOGContentId(TEXT("test:interaction.start"));
    for (int32 I = 1; I < 32; ++I) Definition->Roles.Add(FName(*FString::Printf(TEXT("role_%d"), I)));
    Node.RequiredRoles = Definition->Roles; Definition->Nodes.Add(Node);
    FOGInteractionActionGraph Graph; FString Error;
    TestTrue(TEXT("Thirty-two genuine source roles are architecturally supported"), Definition->BuildGraph(Graph, Error));
    TestEqual(TEXT("No logical participant roles dropped"), Graph.Nodes[0].RequiredRoles.Num(), 32);
    return true;
}

#endif
