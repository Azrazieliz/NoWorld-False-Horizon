#if WITH_DEV_AUTOMATION_TESTS
#include "Animation/OGSharedAnimationRoutes.h"
#include "Interaction/OGSystemicInteractionRuntime.h"
#include "Misc/AutomationTest.h"

namespace
{
UOGSharedAnimationRoutes* RoutesFixture()
{
    auto* D=NewObject<UOGSharedAnimationRoutes>(); D->DefinitionId=FOGContentId(TEXT("test:animation.routes"));
    D->OriginalSourceReference=TEXT("isolated route fixture");
    for (FName Event:{FName(TEXT("walk")),FName(TEXT("rise")),FName(TEXT("idle"))})
    {
        FOGSharedSystemAnimationRoute R; R.SourceSystem=TEXT("movement"); R.Event=Event;
        R.Action=FName(*(TEXT("Traversal.")+Event.ToString())); R.Context=TEXT("Traversal"); D->Routes.Add(R);
    }
    return D;
}
FOGSharedAutomaticAnimationRule AutoRule(FName Id,FName Event,int32 Priority,EOGSharedMovementCondition Condition)
{
    FOGSharedAutomaticAnimationRule R; R.RuleId=Id; R.SourceSystem=TEXT("movement"); R.Event=Event;
    R.Priority=Priority; R.Condition=Condition; return R;
}
FOGSharedMotionEntry MotionFixture()
{
    FOGSharedMotionEntry M; M.MotionId=TEXT("test.motion"); M.SourceClipId=TEXT("test.original");
    M.GameplayAction=TEXT("Test.Action"); M.GameplayContext=TEXT("Test"); M.CompatibleRigProfile=TEXT("test.rig");
    M.NativeSequence=TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Tests/Source.Source")));
    M.bApprovedForGameplay=true; return M; // Metadata fixture; does not load or claim a playable asset.
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedAutomaticPriorityTest,
    "OfflineGame.Animation.Systems.AutomaticPriorityIndependentOfDefinitionOrder",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedAutomaticPriorityTest::RunTest(const FString&)
{
    auto* D=RoutesFixture(); D->AutomaticRules={
        AutoRule(TEXT("low_a"),TEXT("walk"),1,EOGSharedMovementCondition::Any),
        AutoRule(TEXT("low_b"),TEXT("idle"),1,EOGSharedMovementCondition::Any),
        AutoRule(TEXT("rising"),TEXT("rise"),10,EOGSharedMovementCondition::Rising)};
    FString Error; if (!D->ValidateDefinition(Error)) return false;
    FOGSharedMotionEnvironment E; E.bFalling=true; E.VerticalSpeed=20; int32 Index=INDEX_NONE;
    TestTrue(TEXT("Higher priority resolves lower-priority overlap"),D->SelectAutomatic(E,Index,Error));
    TestEqual(TEXT("Actual rising motion selected"),Index,1);
    E.bFalling=false;
    TestFalse(TEXT("An unresolved highest-priority tie is reported"),D->SelectAutomatic(E,Index,Error));
    TestEqual(TEXT("A failed selection publishes no route"),Index,INDEX_NONE);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedMotionCompositionTest,
    "OfflineGame.Animation.Systems.SharedLibrariesComposeAtomically",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedMotionCompositionTest::RunTest(const FString&)
{
    auto* Base=NewObject<UOGSharedMotionLibrary>(); auto* Extra=NewObject<UOGSharedMotionLibrary>();
    Base->Motions={MotionFixture()}; Extra->Motions={MotionFixture()};
    TArray<FOGSharedMotionEntry> Out; FString Error;
    TestTrue(TEXT("Identical shared source entry deduplicates"),FOGSharedMotionResolver::ComposeLibraries(*Base,{Extra},Out,Error));
    TestEqual(TEXT("No per-character copy was made"),Out.Num(),1);
    Extra->Motions[0].SourceClipId=TEXT("test.different_original");
    TestFalse(TEXT("Conflicting source identity cannot override a motion"),FOGSharedMotionResolver::ComposeLibraries(*Base,{Extra},Out,Error));
    TestEqual(TEXT("Failed composition publishes no partial library"),Out.Num(),0);
    TestEqual(TEXT("Original source layer remains intact"),Base->Motions[0].SourceClipId,FName(TEXT("test.original")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGSharedExpressionContractTest,
    "OfflineGame.Animation.Systems.ExpressionCurvesPreserveSourceClock",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGSharedExpressionContractTest::RunTest(const FString&)
{
    auto* D=RoutesFixture(); FOGSharedSystemAnimationRoute R; R.SourceSystem=TEXT("expression"); R.Event=TEXT("source");
    R.Kind=EOGSharedAnimationKind::Expression; R.DurationSeconds=2;
    FOGSharedMorphTrack T; T.Layer=TEXT("source.face"); T.Morph=TEXT("source_morph");
    FOGSharedMorphKey A,B; A.TimeSeconds=0; B.TimeSeconds=2; B.Weight=1; T.Keys={A,B}; R.MorphTracks={T};
    D->Routes.Add(R); FString Error;
    TestTrue(TEXT("Source expression curve is accepted independently of a body motion"),D->ValidateDefinition(Error));
    D->Routes.Last().MorphTracks[0].Keys[1].TimeSeconds=0;
    TestFalse(TEXT("Duplicate source times cannot create undefined interpolation"),D->ValidateDefinition(Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInteractionSharedSelectorTest,
    "OfflineGame.Interaction.Animation.OneSelectorForSoloAndSynchronizedTracks",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FOGInteractionSharedSelectorTest::RunTest(const FString&)
{
    FOGInteractionParticipantState P; P.Binding.Role=TEXT("partner"); P.Binding.RigFamilyId=FOGContentId(TEXT("test:rig"));
    P.Character.bHasManifestation=true; P.Character.Manifestation.ManifestationId=FOGEntityId::NewId();
    P.Character.Manifestation.ActiveVersionId=FOGContentId(TEXT("test:version"));
    FOGInteractionActionNode N; FOGInteractionAnimationBinding Shared; Shared.Role=P.Binding.Role;
    Shared.SharedGameplayAction=TEXT("Test.Shared"); Shared.SharedGameplayContext=TEXT("Test");
    N.AnimationBindings={Shared}; auto Specific=Shared; Specific.VersionId=P.Character.Manifestation.ActiveVersionId;
    Specific.SharedGameplayAction=TEXT("Test.Specific"); N.AnimationBindings.Add(Specific);
    const FOGInteractionAnimationBinding* Selected=nullptr; FString Error;
    TestTrue(TEXT("Source selector resolves the actual Version override"),N.ResolveAnimationBinding(P,Selected,Error));
    TestEqual(TEXT("Same result is available to solo and scene consumers"),Selected?Selected->SharedGameplayAction:NAME_None,FName(TEXT("Test.Specific")));
    N.AnimationBindings.Add(Specific);
    TestFalse(TEXT("Ambiguous source selectors are not chosen by array order"),N.ResolveAnimationBinding(P,Selected,Error));
    TestTrue(TEXT("Ambiguous result returns no source track"),Selected==nullptr);
    return true;
}
#endif
