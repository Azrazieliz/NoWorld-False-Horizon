#include "Interaction/OGInteractionParticipantComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Animation/OGSharedMotionControllerComponent.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Persistence/OGSQLiteWorldStore.h"
#include "Runtime/OGCanonicalCharacterPresentation.h"

namespace
{
struct FParticipantFixture
{
    FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits));
    FOGSQLiteWorldStore Store;
    UWorld* World = nullptr;
    FOGEntityId Ruler = FOGEntityId::NewId();
    FOGEntityId Entity = FOGEntityId::NewId();
    FOGCharacterManifestationRecord Manifestation;
    AActor* Actor = nullptr;
    USkeletalMeshComponent* Mesh = nullptr;
    UOGInteractionParticipantComponent* Binding = nullptr;
    FOGContentId Rig = FOGContentId(TEXT("test:rig.participant"));
    FString Error;

    bool Open(bool bManifestation = true)
    {
        IFileManager::Get().MakeDirectory(*Directory, true);
        if (!Store.Open(FPaths::Combine(Directory, TEXT("participants.db")), Error) ||
            !Store.UpsertEntity(Ruler, TEXT("ruler"), 0, TEXT("{}"), Error) ||
            !Store.UpsertEntity(Entity, TEXT("character"), 0, TEXT("{\"history\":\"retained\"}"), Error)) return false;
        if (bManifestation)
        {
            Manifestation.ManifestationId = Entity;
            Manifestation.OwningRulerId = Ruler;
            Manifestation.IdentityId = FOGContentId(TEXT("test:identity.participant"));
            Manifestation.ActiveVersionId = FOGContentId(TEXT("test:version.original"));
            if (!Store.UpsertCharacterManifestation(Manifestation, 0, Error)) return false;
        }
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(EWorldType::Game, true, NAME_None, nullptr,
            true, ERHIFeatureLevel::SM5, &Values, false);
        if (!World) return false;
        Actor = World->SpawnActor<AActor>();
        Mesh = AddPart(Actor);
        Binding = NewObject<UOGInteractionParticipantComponent>(Actor);
        Actor->AddInstanceComponent(Binding);
        Binding->RegisterComponent();
        return true;
    }
    USkeletalMeshComponent* AddPart(AActor* Owner)
    {
        auto* Part = NewObject<USkeletalMeshComponent>(Owner);
        Owner->AddInstanceComponent(Part);
        if (!Owner->GetRootComponent()) Owner->SetRootComponent(Part);
        auto* Body = NewObject<USkeletalMesh>();
        Body->SetSkeleton(NewObject<USkeleton>());
        Part->SetSkeletalMeshAsset(Body);
        Part->RegisterComponent();
        return Part;
    }
    bool Bind() { return Binding->BindFromCanonicalStore(Entity, Rig, Store, nullptr, Error); }
    ~FParticipantFixture()
    {
        if (World) World->DestroyWorld(true);
        Store.Close();
        IFileManager::Get().DeleteDirectory(*Directory, false, true);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGParticipantOwnershipTest,
    "OfflineGame.Foundation.Interaction.Participant.StoreOwnershipAndFreshProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGParticipantOwnershipTest::RunTest(const FString& Parameters)
{
    FParticipantFixture F;
    if (!TestTrue(TEXT("Open real Store and native test actor"), F.Open()) ||
        !TestTrue(TEXT("Native factory binds existing entity"), F.Bind())) return false;
    FOGInteractionParticipant Participant;
    TestTrue(TEXT("Build supplied role"), F.Binding->BuildParticipant(TEXT("npc"), Participant, F.Error));
    TestTrue(TEXT("Actual Owner comes from Manifestation"), Participant.Character.OwnerEntityId == F.Ruler);
    TestTrue(TEXT("Manifestation is exact existing entity"), Participant.Character.ManifestationId == F.Entity);
    TestTrue(TEXT("Actual actor is retained"), Participant.Actor.Get() == F.Actor);
    TestEqual(TEXT("Role is not diagnostic selection"), Participant.Role, FName(TEXT("npc")));
    TestFalse(TEXT("Empty role rejected"), F.Binding->BuildParticipant(NAME_None, Participant, F.Error));
    TestFalse(TEXT("Failed output cannot leak stale actor"), Participant.Actor.IsValid());
    FOGFoundationCharacterProjection P;
    TestTrue(TEXT("Update canonical data outside binding"), F.Store.UpsertEntity(F.Entity,
        TEXT("character"), 7, TEXT("{\"history\":\"changed\",\"injury_severity_bps\":2500}"), F.Error));
    TestTrue(TEXT("Fresh projection reads changed data"), F.Binding->GetCurrentProjection(P, F.Error));
    TestTrue(TEXT("No cached state fallback"), P.EntityStateJson.Contains(TEXT("changed")));
    TestTrue(TEXT("Native regular presentation refresh"), F.Binding->RefreshFromCanonical(F.Error));
    auto* Canonical = F.Actor->FindComponentByClass<UOGCanonicalCharacterPresentation>();
    if (TestNotNull(TEXT("Ordinary adapter installed"), Canonical))
        TestEqual(TEXT("Same current canonical payload reaches regular presentation"), Canonical->GetProjection().EntityStateJson, P.EntityStateJson);
    TestFalse(TEXT("Unknown entity cannot rebind actor"),
        F.Binding->BindFromCanonicalStore(FOGEntityId::NewId(), F.Rig, F.Store, nullptr, F.Error));
    TestTrue(TEXT("Failed bind preserves previous valid selection"), F.Binding->BuildParticipant(TEXT("npc"), Participant, F.Error));
    F.Manifestation.ActiveVersionId = FOGContentId(TEXT("test:version.current"));
    TestTrue(TEXT("Canonical factory changes actual Version"), F.Store.UpsertCharacterManifestation(F.Manifestation, 8, F.Error));
    TestFalse(TEXT("Stale visual Version does not escape as a playable binding"), F.Binding->BuildParticipant(TEXT("npc"), Participant, F.Error));
    TestTrue(TEXT("Refresh adopts exact current Version on regular adapter"), F.Binding->RefreshFromCanonical(F.Error));
    TestTrue(TEXT("Read refreshed canonical Version"), F.Binding->GetCurrentProjection(P, F.Error));
    TestTrue(TEXT("No base-Version substitution"), P.Manifestation.ActiveVersionId == F.Manifestation.ActiveVersionId);
    F.Store.Close();
    TestFalse(TEXT("Closed Store cannot emit cached participant"), F.Binding->BuildParticipant(TEXT("npc"), Participant, F.Error));
    TestFalse(TEXT("Unavailable Store resets actor output"), Participant.Actor.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGParticipantSourceTest,
    "OfflineGame.Foundation.Interaction.Participant.ExactOriginalSourceMaturity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGParticipantSourceTest::RunTest(const FString& Parameters)
{
    FParticipantFixture F;
    if (!TestTrue(TEXT("Open canonical source fixture"), F.Open()) || !TestTrue(TEXT("Bind"), F.Bind())) return false;
    FOGInteractionParticipant Participant;
    TestTrue(TEXT("Missing source preserves ordinary interaction"), F.Binding->BuildParticipant(TEXT("participant"), Participant, F.Error));
    TestFalse(TEXT("Missing lore cannot claim an adult source"), F.Binding->HasCanonicalAdultSource(F.Error));
    FOGFoundationCharacterProjection P;
    if (!F.Binding->GetCurrentProjection(P, F.Error)) return false;
    FOGCharacterIdentityDefinition Identity;
    Identity.IdentityId = F.Manifestation.IdentityId;
    Identity.CanonicalMaturity = EOGCanonicalMaturity::Adult;
    FOGCharacterVersionDefinition Version;
    Version.IdentityId = Identity.IdentityId;
    Version.VersionId = F.Manifestation.ActiveVersionId;
    FOGInteractionParticipantSourceProfile Receipt;
    TestTrue(TEXT("Copy actual original Identity maturity"), FOGInteractionParticipantSourceProfile::FromDefinitions(
        P, Identity, Version, FOGContentId(TEXT("test:source.original_lore")), TEXT("approved-source-v1"),
        TEXT("source-authored age metadata"), Receipt, F.Error));
    TestTrue(TEXT("Enum is copied, not inferred"), Receipt.CanonicalMaturity == Identity.CanonicalMaturity);
    F.Binding->SetSourceProfileResolver([&Receipt](const auto&, auto& Out, FString&) { Out = Receipt; return true; });
    TestTrue(TEXT("Exact native source retained"), F.Binding->HasCanonicalAdultSource(F.Error));
    Receipt.EntityId = F.Ruler;
    TestFalse(TEXT("Source for another entity cannot bleed across"), F.Binding->HasCanonicalAdultSource(F.Error));
    Receipt.EntityId = F.Entity;
    Receipt.VersionId = FOGContentId(TEXT("test:version.other"));
    TestFalse(TEXT("Source for another Version rejected"), F.Binding->HasCanonicalAdultSource(F.Error));
    Receipt.VersionId = Version.VersionId;
    Receipt.IdentityId = FOGContentId(TEXT("test:identity.other"));
    TestFalse(TEXT("Source for another Identity rejected"), F.Binding->HasCanonicalAdultSource(F.Error));
    Receipt.IdentityId = Identity.IdentityId;
    for (EOGCanonicalMaturity Maturity : {EOGCanonicalMaturity::Unknown, EOGCanonicalMaturity::NonAdult})
    {
        Identity.CanonicalMaturity = Maturity;
        TestTrue(TEXT("Copy original source classification without changing it"), FOGInteractionParticipantSourceProfile::FromDefinitions(
            P, Identity, Version, FOGContentId(TEXT("test:source.original_lore")), TEXT("v1"), TEXT(""), Receipt, F.Error));
        TestFalse(TEXT("Unknown and NonAdult remain their actual source classification"), F.Binding->HasCanonicalAdultSource(F.Error));
        TestTrue(TEXT("General mature and ordinary bindings remain available"), F.Binding->BuildParticipant(TEXT("participant"), Participant, F.Error));
    }
    Version.IdentityId = FOGContentId(TEXT("test:identity.other"));
    TestFalse(TEXT("Mismatched original definitions cannot be relabeled"), FOGInteractionParticipantSourceProfile::FromDefinitions(
        P, Identity, Version, FOGContentId(TEXT("test:source.original_lore")), TEXT("v1"), TEXT(""), Receipt, F.Error));
    TestFalse(TEXT("Failure resets source receipt"), Receipt.EntityId.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGParticipantNpcTest,
    "OfflineGame.Foundation.Interaction.Participant.WorldBornNpcAndProjectionMismatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGParticipantNpcTest::RunTest(const FString& Parameters)
{
    FParticipantFixture F;
    if (!TestTrue(TEXT("Open world-born NPC"), F.Open(false)) || !TestTrue(TEXT("Bind NPC"), F.Bind())) return false;
    FOGInteractionParticipant Participant;
    TestTrue(TEXT("Ordinary NPC has native binding"), F.Binding->BuildParticipant(TEXT("npc"), Participant, F.Error));
    TestTrue(TEXT("NPC's actual canonical ownership is itself"), Participant.Character.OwnerEntityId == F.Entity);
    TestFalse(TEXT("No fake Manifestation"), Participant.Character.ManifestationId.IsValid());
    F.Binding->SetSourceProfileResolver([](const auto& P, auto& Out, FString&)
    {
        Out.EntityId = P.Context.EntityId;
        Out.SourceProfileId = FOGContentId(TEXT("test:lore.npc"));
        Out.SourceRevision = TEXT("native-v1");
        Out.CanonicalMaturity = EOGCanonicalMaturity::Adult;
        return true;
    });
    TestTrue(TEXT("Exact authored NPC lore seam"), F.Binding->HasCanonicalAdultSource(F.Error));
    auto* Canonical = F.Actor->FindComponentByClass<UOGCanonicalCharacterPresentation>();
    FOGFoundationCharacterContext Wrong;
    Wrong.EntityId = F.Ruler;
    TestTrue(TEXT("External factory changes actual actor projection"), Canonical->RefreshFromCanonicalStore(Wrong, F.Store, nullptr, F.Error));
    TestFalse(TEXT("Old binding cannot relabel changed actor projection"), F.Binding->BuildParticipant(TEXT("npc"), Participant, F.Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGParticipantPartsTest,
    "OfflineGame.Foundation.Interaction.Participant.MultipartDiscoveryAndSignificanceRestoration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGParticipantPartsTest::RunTest(const FString& Parameters)
{
    FParticipantFixture F;
    if (!TestTrue(TEXT("Open native multipart actor"), F.Open(false))) return false;
    auto* Second = F.AddPart(F.Actor);
    auto* Motion = NewObject<UOGSharedMotionControllerComponent>(F.Actor);
    F.Actor->AddInstanceComponent(Motion);
    Motion->RigProfile = TEXT("test_native_parts");
    Motion->RegisterComponent();
    TestTrue(TEXT("Factory registers actual native geometry"), Motion->RegisterNativeParts({F.Mesh, Second}));
    auto* ObsoleteInheritedBody = F.AddPart(F.Actor);
    ObsoleteInheritedBody->SetVisibility(false);
    ObsoleteInheritedBody->SetHiddenInGame(true);
    if (!TestTrue(TEXT("Bind native multipart actor"), F.Bind())) return false;
    TArray<USkeletalMeshComponent*> Parts;
    TestTrue(TEXT("Read all parts"), F.Binding->GetNativeParts(Parts, F.Error));
    TestEqual(TEXT("Both actual parts retained"), Parts.Num(), 2);
    TestFalse(TEXT("Obsolete inherited body is excluded from actual source parts"), Parts.Contains(ObsoleteInheritedBody));
    TestTrue(TEXT("Generic controller discovered"), F.Binding->GetMotionController() == Motion);
    F.Mesh->bEnableUpdateRateOptimizations = true;
    F.Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    Second->bEnableUpdateRateOptimizations = false;
    Second->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
    TestFalse(TEXT("Ordinary traversal owns presentation before interaction"), F.Binding->IsInInteractionPresentation());
    F.Binding->SetInteractionSignificance(true, false);
    TestTrue(TEXT("Host significance acquires interaction presentation"), F.Binding->IsInInteractionPresentation());
    TestFalse(TEXT("Focus restores full fidelity on first part"), F.Mesh->bEnableUpdateRateOptimizations);
    TestFalse(TEXT("Focus includes independent second part"), Second->bEnableUpdateRateOptimizations);
    F.Binding->SetInteractionSignificance(false, false);
    TestTrue(TEXT("Background relevance still retains interaction presentation"), F.Binding->IsInInteractionPresentation());
    Motion->StopAction();
    TestTrue(TEXT("Finished/stopped clip does not release interaction presentation"), F.Binding->IsInInteractionPresentation());
    TestTrue(TEXT("Background reduces visual policy only"), Second->bEnableUpdateRateOptimizations);
    FOGInteractionParticipant Participant;
    TestTrue(TEXT("Reduced significance retains logical binding"), F.Binding->BuildParticipant(TEXT("background"), Participant, F.Error));
    Second->UnregisterComponent();
    auto* Replacement = F.AddPart(F.Actor);
    Replacement->bEnableUpdateRateOptimizations = false;
    Replacement->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
    TestTrue(TEXT("Factory installs actual replacement part set"), Motion->RegisterNativeParts({F.Mesh, Replacement}));
    TestTrue(TEXT("Refresh rediscovers the real part set"), F.Binding->RefreshFromCanonical(F.Error));
    TestFalse(TEXT("Detached part's exact original rate policy restored"), Second->bEnableUpdateRateOptimizations);
    TestTrue(TEXT("New part receives current background policy"), Replacement->bEnableUpdateRateOptimizations);
    F.Binding->Unbind();
    TestFalse(TEXT("Unbind releases traversal presentation ownership"), F.Binding->IsInInteractionPresentation());
    TestTrue(TEXT("Original first-part rate policy restored"), F.Mesh->bEnableUpdateRateOptimizations);
    TestFalse(TEXT("Original independent second-part rate policy restored"), Second->bEnableUpdateRateOptimizations);
    TestTrue(TEXT("Original second-part tick policy restored"), Second->VisibilityBasedAnimTickOption == EVisibilityBasedAnimTickOption::AlwaysTickPose);
    TestFalse(TEXT("Replacement part's original rate policy restored"), Replacement->bEnableUpdateRateOptimizations);
    TestEqual(TEXT("Discovery never re-registers or drops parts"), Motion->GetNativePartCount(), 2);
    TestFalse(TEXT("Unbound output fails closed"), F.Binding->BuildParticipant(TEXT("npc"), Participant, F.Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGParticipantReplacedRigTest,
    "OfflineGame.Foundation.Interaction.Participant.ReplacedNativeRigRetainsCanonicalBinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGParticipantReplacedRigTest::RunTest(const FString& Parameters)
{
    FParticipantFixture F;
    if (!TestTrue(TEXT("Open original multipart factory fixture"), F.Open(false))) return false;
    // Match the inherited unbound adapter supplied by native playable pawns.
    auto* Inherited = NewObject<UOGCanonicalCharacterPresentation>(F.Actor);
    F.Actor->AddInstanceComponent(Inherited);
    Inherited->RegisterComponent();
    TestFalse(TEXT("Inherited adapter starts unbound"), Inherited->HasProjection());
    auto* Second = F.AddPart(F.Actor);
    auto* Motion = NewObject<UOGSharedMotionControllerComponent>(F.Actor);
    F.Actor->AddInstanceComponent(Motion);
    Motion->RigProfile = TEXT("test_replaced_native_parts");
    Motion->RegisterComponent();
    if (!TestTrue(TEXT("Factory registers exact current source parts"), Motion->RegisterNativeParts({F.Mesh, Second})) ||
        !TestTrue(TEXT("Bind the actual existing canonical entity"), F.Bind())) return false;
    FOGFoundationCharacterProjection Before;
    if (!TestTrue(TEXT("Read source projection before replacement"), F.Binding->GetCurrentProjection(Before, F.Error))) return false;
    F.Binding->SetInteractionSignificance(true, false);
    auto* Replacement = F.AddPart(F.Actor);
    F.Actor->SetRootComponent(Replacement);
    TestTrue(TEXT("Factory publishes exact replacement parts"), Motion->RegisterNativeParts({Replacement, Second}));
    F.Mesh->DestroyComponent();
    TestTrue(TEXT("Refresh survives destruction of the previously configured rig part"), F.Binding->RefreshFromCanonical(F.Error));
    FOGFoundationCharacterProjection After;
    TestTrue(TEXT("Current projection remains available"), F.Binding->GetCurrentProjection(After, F.Error));
    TestTrue(TEXT("Real entity is never replaced or relabeled"), After.Context.EntityId == Before.Context.EntityId);
    TestTrue(TEXT("Actual canonical owner is preserved"), After.Context.OwnerEntityId == Before.Context.OwnerEntityId);
    TestEqual(TEXT("Wardrobe factory cannot rewrite canonical state"), After.EntityStateJson, Before.EntityStateJson);
    TArray<USkeletalMeshComponent*> Parts;
    TestTrue(TEXT("Native source list remains discoverable"), F.Binding->GetNativeParts(Parts, F.Error));
    TestTrue(TEXT("Actual new part is bound"), Parts.Contains(Replacement));
    TestFalse(TEXT("Destroyed part is not reused"), Parts.Contains(F.Mesh));
    TestTrue(TEXT("Interaction ownership survives source replacement"), F.Binding->IsInInteractionPresentation());
    F.Binding->Unbind();
    TestFalse(TEXT("Inherited adapter releases the Store binding installed by this component"), Inherited->HasProjection());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGParticipantRulerBodyTest,
    "OfflineGame.Foundation.Interaction.Participant.RealRulerBodyWhileCompanionPossessed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGParticipantRulerBodyTest::RunTest(const FString& Parameters)
{
    FParticipantFixture F;
    if (!TestTrue(TEXT("Open canonical companion"), F.Open()) || !TestTrue(TEXT("Bind companion"), F.Bind())) return false;
    auto* Companion = F.World->SpawnActor<ACharacter>();
    F.AddPart(Companion);
    auto* Controller = F.World->SpawnActor<APlayerController>();
    Controller->Possess(Companion);
    FOGInteractionParticipant Player;
    TestFalse(TEXT("Owned companion cannot be supplied as canonical Ruler"), UOGInteractionParticipantComponent::ResolveActualRulerParticipant(
        F.World, F.Store, F.Entity, Player, F.Error));
    TestFalse(TEXT("Possessed companion is never a Ruler fallback"), UOGInteractionParticipantComponent::ResolveActualRulerParticipant(
        F.World, F.Store, F.Ruler, Player, F.Error));
    ACharacter* RulerBody = F.World->SpawnActor<ACharacter>();
    auto* RulerMesh = F.AddPart(RulerBody);
    auto* Canonical = NewObject<UOGCanonicalCharacterPresentation>(RulerBody);
    RulerBody->AddInstanceComponent(Canonical);
    Canonical->RegisterComponent();
    FOGFoundationCharacterContext Context;
    Context.EntityId = F.Ruler;
    TestTrue(TEXT("Actual physical Ruler projection installed"), Canonical->ConfigureFromCanonicalStore(
        Context, RulerMesh, TEXT("test_ruler_rig"), F.Store, nullptr, F.Error));
    TestTrue(TEXT("Projection-only native Ruler discovery works"), UOGInteractionParticipantComponent::ResolveActualRulerParticipant(
        F.World, F.Store, F.Ruler, Player, F.Error));
    auto* RulerBinding = NewObject<UOGInteractionParticipantComponent>(RulerBody);
    RulerBody->AddInstanceComponent(RulerBinding);
    RulerBinding->RegisterComponent();
    TestTrue(TEXT("Bind preserved real Ruler pawn before possession switch"), RulerBinding->BindFromCanonicalStore(
        F.Ruler, F.Rig, F.Store, nullptr, F.Error));
    Controller->Possess(RulerBody);
    Controller->Possess(Companion);
    TestTrue(TEXT("Resolve separate real Ruler body"), UOGInteractionParticipantComponent::ResolveActualRulerParticipant(
        F.World, F.Store, F.Ruler, Player, F.Error));
    TestTrue(TEXT("Ruler actor is distinct from possessed companion"), Player.Actor.Get() == RulerBody && Player.Actor.Get() != Controller->GetPawn());
    TestTrue(TEXT("Ruler entity and self ownership preserved"), Player.Character.EntityId == F.Ruler && Player.Character.OwnerEntityId == F.Ruler);
    TestEqual(TEXT("Player role fixed to actual Ruler"), Player.Role, FName(TEXT("player")));
    TestTrue(TEXT("Resolver preserves factory's actual authored rig family"), Player.RigFamilyId == F.Rig);
    AActor* Duplicate = F.World->SpawnActor<AActor>();
    auto* DuplicateMesh = F.AddPart(Duplicate);
    auto* OtherCanonical = NewObject<UOGCanonicalCharacterPresentation>(Duplicate);
    Duplicate->AddInstanceComponent(OtherCanonical);
    OtherCanonical->RegisterComponent();
    TestTrue(TEXT("Prepare conflicting physical projections"), OtherCanonical->ConfigureFromCanonicalStore(
        Context, DuplicateMesh, TEXT("test_ruler_rig"), F.Store, nullptr, F.Error));
    TestFalse(TEXT("Duplicate real bodies are ambiguous, never first-match selected"), UOGInteractionParticipantComponent::ResolveActualRulerParticipant(
        F.World, F.Store, F.Ruler, Player, F.Error));
    TestFalse(TEXT("Ambiguous resolver clears output"), Player.Actor.IsValid());
    return true;
}
#endif
