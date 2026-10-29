#include "Interaction/OGSystemicInteractionRuntime.h"
#include "Persistence/OGWorldStore.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    const FName PlayerRole(TEXT("player"));
    bool Fail(FString& Error, const TCHAR* Message) { Error = Message; return false; }

    // A grammar pass rejects permissive-reader extensions (comments, trailing
    // commas, non-JSON numbers and trailing documents) before deserialization.
    struct FStrictInteractionJson
    {
        const FString& Text;
        int32 At = 0;
        void Space() { while (At < Text.Len() && (Text[At] == ' ' || Text[At] == '\t' || Text[At] == '\r' || Text[At] == '\n')) ++At; }
        bool Take(TCHAR C) { Space(); if (At < Text.Len() && Text[At] == C) { ++At; return true; } return false; }
        bool Digit() const { return At < Text.Len() && Text[At] >= '0' && Text[At] <= '9'; }
        bool String()
        {
            if (!Take('"')) return false;
            while (At < Text.Len())
            {
                const TCHAR C = Text[At++];
                if (C == '"') return true;
                if (C < 0x20) return false;
                if (C != '\\') continue;
                if (At >= Text.Len()) return false;
                const TCHAR Escape = Text[At++];
                if (Escape == 'u')
                {
                    for (int32 I = 0; I < 4; ++I)
                    {
                        if (At >= Text.Len()) return false;
                        const TCHAR H = Text[At++];
                        if (!((H >= '0' && H <= '9') || (H >= 'a' && H <= 'f') || (H >= 'A' && H <= 'F'))) return false;
                    }
                }
                else if (Escape != '"' && Escape != '\\' && Escape != '/' && Escape != 'b' && Escape != 'f' && Escape != 'n' && Escape != 'r' && Escape != 't') return false;
            }
            return false;
        }
        bool Value(int32 Depth = 0)
        {
            Space();
            if (At >= Text.Len() || Depth > 64) return false;
            if (Text[At] == '"') return String();
            if (Text[At] == '{' || Text[At] == '[')
            {
                if (Depth >= 64) return false;
                const bool bObject = Text[At++] == '{';
                const TCHAR End = bObject ? '}' : ']';
                if (Take(End)) return true;
                do
                {
                    if (bObject && (!String() || !Take(':'))) return false;
                    if (!Value(Depth + 1)) return false;
                    if (Take(End)) return true;
                } while (Take(','));
                return false;
            }
            for (const TCHAR* Literal : { TEXT("true"), TEXT("false"), TEXT("null") })
            {
                const int32 Length = FCString::Strlen(Literal);
                if (Text.Mid(At, Length) == Literal) { At += Length; return true; }
            }
            const int32 Start = At;
            if (Text[At] == '-') ++At;
            if (!Digit()) return false;
            if (Text[At] == '0') ++At;
            else while (Digit()) ++At;
            if (At < Text.Len() && Text[At] == '.')
            { ++At; if (!Digit()) return false; while (Digit()) ++At; }
            if (At < Text.Len() && (Text[At] == 'e' || Text[At] == 'E'))
            {
                ++At;
                if (At < Text.Len() && (Text[At] == '+' || Text[At] == '-')) ++At;
                if (!Digit()) return false;
                while (Digit()) ++At;
            }
            double Number = 0;
            return LexTryParseString(Number, *Text.Mid(Start, At - Start)) && FMath::IsFinite(Number);
        }
    };

    bool ReadStateJson(const FString& Json, TSharedPtr<FJsonObject>& Out, FString& Error, bool bPatch = false)
    {
        Out.Reset();
        if (Json.Len() > 8 * 1024 * 1024) return Fail(Error, TEXT("Interaction JSON exceeds 8 Mi characters."));
        FStrictInteractionJson Grammar{Json};
        if (!Grammar.Value()) return Fail(Error, TEXT("Interaction state requires strict JSON."));
        Grammar.Space();
        if (Grammar.At != Json.Len()) return Fail(Error, TEXT("Interaction JSON has trailing data."));
        struct FScope { bool bObject = false; TSet<FString> Keys; };
        TArray<FScope> Stack;
        const auto Reader = TJsonReaderFactory<>::Create(Json);
        EJsonNotation N;
        while (Reader->ReadNext(N))
        {
            if (N == EJsonNotation::Error) return Fail(Error, TEXT("Malformed interaction JSON."));
            const bool bEnd = N == EJsonNotation::ObjectEnd || N == EJsonNotation::ArrayEnd;
            if (!bEnd && !Stack.IsEmpty() && Stack.Last().bObject)
            {
                const FString Key = Reader->GetIdentifier();
                if (Stack.Last().Keys.Contains(Key)) return Fail(Error, TEXT("Duplicate interaction JSON member."));
                Stack.Last().Keys.Add(Key);
                if (bPatch)
                {
                    FString Reserved = Key.ToLower();
                    Reserved.ReplaceInline(TEXT("_"), TEXT("")); Reserved.ReplaceInline(TEXT("-"), TEXT(""));
                    // Schema/authority protection, never a test of a character's
                    // permission or eligibility. These patches only express state.
                    for (const TCHAR* Name : { TEXT("eligible"), TEXT("eligibility"), TEXT("iseligible"), TEXT("beligible"),
                        TEXT("permission"), TEXT("permissions"), TEXT("consent"), TEXT("consentstate"),
                        TEXT("affection"), TEXT("affectionpoints"), TEXT("affectionthreshold"), TEXT("authorizedadult"),
                        TEXT("reward"), TEXT("rewards"), TEXT("currency"), TEXT("grants"),
                        TEXT("manifestationid"), TEXT("identityid"), TEXT("owningrulerid"), TEXT("ownerentityid"),
                        TEXT("versionid"), TEXT("activeversionid"), TEXT("grand"), TEXT("grandstate"),
                        TEXT("isgrand"), TEXT("bgrand"), TEXT("convergence"), TEXT("progressionstate") })
                        if (Reserved == Name) return Fail(Error, TEXT("Interaction patch contains a reserved authority/access/reward field."));
                }
            }
            if (N == EJsonNotation::ObjectStart || N == EJsonNotation::ArrayStart)
            { FScope Scope; Scope.bObject = N == EJsonNotation::ObjectStart; Stack.Add(MoveTemp(Scope)); }
            else if (bEnd && !Stack.IsEmpty()) Stack.Pop();
        }
        if (!Stack.IsEmpty() || !Reader->GetErrorMessage().IsEmpty() ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Out, FJsonSerializer::EFlags::StoreNumbersAsStrings) || !Out.IsValid())
            return Fail(Error, TEXT("Interaction state must be a complete JSON object."));
        return true;
    }

    FString StateJson(const TSharedPtr<FJsonObject>& Object)
    {
        FString Json;
        FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
        return Json;
    }
    void MergeState(const TSharedPtr<FJsonObject>& Target, const TSharedPtr<FJsonObject>& Patch)
    {
        for (const auto& Pair : Patch->Values)
        {
            const TSharedPtr<FJsonObject>* Existing = nullptr;
            if (Pair.Value->Type == EJson::Object && Target->TryGetObjectField(Pair.Key, Existing))
                MergeState(*Existing, Pair.Value->AsObject());
            else Target->SetField(Pair.Key, Pair.Value);
        }
    }
    bool MergeStateJson(FString& Json, const FString& PatchJson, FString& Error)
    {
        TSharedPtr<FJsonObject> Current, Patch;
        if (!ReadStateJson(Json, Current, Error) || !ReadStateJson(PatchJson, Patch, Error, true)) return false;
        if (!Patch->Values.IsEmpty()) { MergeState(Current, Patch); Json = StateJson(Current); }
        return true;
    }
    const FOGInteractionParticipant* UniqueRole(const TArray<FOGInteractionParticipant>& Bindings, FName Role)
    {
        const FOGInteractionParticipant* Result = nullptr;
        if (Role.IsNone()) return nullptr;
        for (const auto& Binding : Bindings) if (Binding.Role == Role)
        { if (Result) return nullptr; Result = &Binding; }
        return Result;
    }
    bool ValidatePatches(const FOGInteractionAuthoredConsequences& Consequences,
        const TArray<FOGInteractionParticipant>& Bindings, FString& Error)
    {
        TSharedPtr<FJsonObject> Object;
        for (const auto& Patch : Consequences.PresentationPatches)
        {
            const auto* Binding = UniqueRole(Bindings, Patch.Role);
            if (!Binding) return Fail(Error, TEXT("Presentation patch role must bind exactly one canonical participant."));
            if (Patch.SelectedSkinId.IsSet() && !Patch.SelectedSkinId.GetValue().IsEmpty() && !Patch.SelectedSkinId.GetValue().IsValid())
                return Fail(Error, TEXT("Presentation patch skin must be an authored content ID or an explicit empty selection."));
            if (!ReadStateJson(Patch.OutfitMergeJson, Object, Error, true) || !ReadStateJson(Patch.VariantMergeJson, Object, Error, true)) return false;
            for (const auto& Record : Consequences.Presentation)
                if (Record.ManifestationId == Binding->Character.EntityId)
                    return Fail(Error, TEXT("Do not mix full presentation replacement and a sparse patch for the same participant."));
        }
        for (const auto& Patch : Consequences.AdultContextPatches)
        {
            if (!UniqueRole(Bindings, Patch.Role)) return Fail(Error, TEXT("Context patch role must bind exactly one canonical participant."));
            if (Patch.CurrentProfileVariantId.IsSet() && !Patch.CurrentProfileVariantId.GetValue().IsEmpty() && !Patch.CurrentProfileVariantId.GetValue().IsValid())
                return Fail(Error, TEXT("Context profile must be an authored content ID or an explicit empty selection."));
            if (!ReadStateJson(Patch.MutableContextMergeJson, Object, Error, true)) return false;
        }
        // Full record consequences remain source compatible and deliberately
        // retain their original explicit-replacement semantics.
        for (const auto& Record : Consequences.Presentation)
            if (!ReadStateJson(Record.OutfitStateJson, Object, Error) || !ReadStateJson(Record.PresentationVariantStateJson, Object, Error)) return false;
        for (const auto& Record : Consequences.Memories)
            if (!ReadStateJson(Record.StateJson, Object, Error)) return false;
        for (const auto& Record : Consequences.Knowledge)
            if (!ReadStateJson(TEXT("{\"value\":") + Record.ValueJson + TEXT("}"), Object, Error)) return false;
        return true;
    }
    void BodySize(AActor* Actor, float& Radius, float& HalfHeight)
    {
        Radius = 35.f; HalfHeight = 90.f;
        if (auto* Character = Cast<ACharacter>(Actor))
        {
            Character->GetCapsuleComponent()->GetScaledCapsuleSize(Radius, HalfHeight);
        }
        else if (Actor)
        {
            FVector Origin, Extent;
            Actor->GetActorBounds(true, Origin, Extent);
            Radius = FMath::Max(35.f, FMath::Max(Extent.X, Extent.Y));
            HalfHeight = FMath::Max(90.f, Extent.Z);
        }
    }
}

FOGSystemicInteractionRuntime::FOGSystemicInteractionRuntime(IOGWorldStore& InStore, const FOGEntityId& InRuler,
    FOGOptionalPackageHost* OptionalPackageHost)
    : Store(InStore), RulerId(InRuler), Characters(InStore), OptionalAssets(InStore, OptionalPackageHost) {}
FOGSystemicInteractionRuntime::~FOGSystemicInteractionRuntime()
{
    if (bActive) { FString Ignored; Exit(FName(TEXT("runtime_teardown")), LastWorldTick, Ignored); }
}

const FOGInteractionActionNode* FOGSystemicInteractionRuntime::FindNode(FName Id) const
{
    return Graph.Nodes.FindByPredicate([Id](const auto& Node) { return Node.NodeId == Id; });
}

TArray<FName> FOGSystemicInteractionRuntime::GetAvailableActions() const
{
    TArray<FName> Result;
    if (!bActive) return Result;
    const FName Previous = CurrentNode.IsNone() ? LastCompletedNode : CurrentNode;
    if (Previous.IsNone()) { Result.Add(Graph.StartNode); return Result; }
    if (const auto* Node = FindNode(Previous))
        for (const FName Next : Node->NextNodes) Result.AddUnique(Next);
    return Result;
}

bool FOGSystemicInteractionRuntime::Enter(const FOGInteractionEntry& Request,
    const FOGInteractionActionGraph& Definition, int64 Tick, FString& Error)
{
    if (bActive) return Fail(Error, TEXT("An interaction is already active."));
    if (!RulerId.IsValid() || Tick < 0 || !Definition.GraphId.IsValid() || Request.Participants.IsEmpty())
        return Fail(Error, TEXT("A canonical Ruler, clock, graph and participants are required."));
    TSet<FOGEntityId> Entities;
    TSet<AActor*> Actors;
    int32 PlayerCount = 0;
    TSet<FName> Roles;
    for (const auto& Participant : Request.Participants)
    {
        if (!Participant.Character.EntityId.IsValid() || Participant.Role.IsNone() || Entities.Contains(Participant.Character.EntityId))
            return Fail(Error, TEXT("Each participant requires a unique persistent entity and bound role."));
        Entities.Add(Participant.Character.EntityId); Roles.Add(Participant.Role);
        if (Participant.Role == PlayerRole)
        {
            ++PlayerCount;
            if (Participant.Character.EntityId != RulerId)
                return Fail(Error, TEXT("The physical player role belongs to the canonical Ruler."));
        }
        else if (Participant.Character.EntityId == RulerId)
            return Fail(Error, TEXT("The canonical Ruler must occupy the player role."));
        if (auto* Actor = Participant.Actor.Get())
        {
            if (!Request.World.IsValid() || Actors.Contains(Actor) || Actor->GetWorld() != Request.World.Get())
                return Fail(Error, TEXT("Embodied participant bindings must be distinct actors in the staging world."));
            Actors.Add(Actor);
        }
    }
    if (PlayerCount != 1) return Fail(Error, TEXT("Exactly one canonical Ruler player-side participant is required."));
    TSet<FName> NodeIds;
    for (const auto& Node : Definition.Nodes)
    {
        if (Node.NodeId.IsNone() || NodeIds.Contains(Node.NodeId) || !Node.ActionId.IsValid() ||
            !FMath::IsFinite(Node.DurationSeconds) || Node.DurationSeconds <= 0)
            return Fail(Error, TEXT("Graph nodes require unique IDs, action IDs and finite positive durations."));
        NodeIds.Add(Node.NodeId);
        if (!ValidatePatches(Node.Consequences, Request.Participants, Error)) return false;
        TSet<FName> PlacedRoles;
        for (const auto& Placement : Node.RolePlacements)
        {
            if (!UniqueRole(Request.Participants, Placement.Role) || PlacedRoles.Contains(Placement.Role) ||
                Placement.RelativeToAnchor.ContainsNaN() || !Placement.RelativeToAnchor.GetRotation().IsNormalized())
                return Fail(Error, TEXT("Authored placements require unique individual roles and finite normalized transforms."));
            PlacedRoles.Add(Placement.Role);
        }
        for (FName Role : Node.RequiredRoles) if (!Roles.Contains(Role))
            return Fail(Error, TEXT("A graph role has no bound participant."));
        for (const auto& Constraint : Node.Constraints)
            if (!Roles.Contains(Constraint.ParticipantRole) || !Roles.Contains(Constraint.TargetRole) ||
                Constraint.TargetOffset.ContainsNaN())
                return Fail(Error, TEXT("Constraints require bound roles and finite target offsets."));
        if (Node.SharedSceneAction.IsNone() != Node.SharedSceneContext.IsNone())
            return Fail(Error, TEXT("A shared scene requires both authored semantic action and context."));
        for (const auto& Binding : Node.AnimationBindings)
            if (!Roles.Contains(Binding.Role))
                return Fail(Error, TEXT("Animation bindings must reference an existing participant role."));
    }
    if (!NodeIds.Contains(Definition.StartNode)) return Fail(Error, TEXT("Graph start node is missing."));
    for (const auto& Node : Definition.Nodes) for (FName Next : Node.NextNodes)
        if (!NodeIds.Contains(Next)) return Fail(Error, TEXT("Graph transition references a missing node."));
    LastWorldTick = Tick;
    Entry = Request; Graph = Definition; SessionId = FOGEntityId::NewId();
    CurrentNode = NAME_None; LastCompletedNode = NAME_None;
    Elapsed = 0; Pacing = 1; bCompletionRefreshPending = false; Sequence.Reset(); Participants.Reset(); Restore.Reset();
    for (const auto& Binding : Entry.Participants)
    {
        FOGInteractionParticipantState State; State.Binding = Binding;
        Participants.Add(MoveTemp(State));
    }
    if (!RefreshCharacters(Error)) { Participants.Reset(); return false; }
    if (!SolveStaging(Entry.PreferredLocation, FindNode(Graph.StartNode), Error))
    { Participants.Reset(); return false; }
    // Persist entry before touching ordinary movement/camera state.
    if (!CommitEvent(FName(TEXT("interaction.entered")), Entry.Context, Tick, nullptr, Error))
    { Participants.Reset(); return false; }
    for (auto& Participant : Participants) if (auto* Actor = Participant.Binding.Actor.Get())
    {
        FActorRestore Saved;
        Saved.Actor = Actor; Saved.Transform = Actor->GetActorTransform(); Saved.bHidden = Actor->IsHidden();
        Saved.bHasPresence = Participant.Character.bHasPresence;
        if (Saved.bHasPresence) Saved.Presence = Participant.Character.Presence;
        if (auto* Character = Cast<ACharacter>(Actor))
        {
            auto* Movement = Character->GetCharacterMovement();
            Saved.Velocity = Movement->Velocity;
            Saved.MovementMode = static_cast<uint8>(Movement->MovementMode);
            Saved.CustomMovementMode = Movement->CustomMovementMode;
            Movement->StopMovementImmediately(); Movement->DisableMovement();
        }
        Restore.Add(Saved);
        Actor->SetActorTransform(Participant.StagingTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }
    if (auto* Controller = Entry.ReturnController.Get())
    {
        ReturnViewTarget = Controller->GetViewTarget(); ReturnControlRotation = Controller->GetControlRotation();
        // SetIgnore* uses a counter; only undo the suppression owned here.
        Controller->SetIgnoreMoveInput(true); Controller->SetIgnoreLookInput(true);
        bMoveSuppressed = true; bLookSuppressed = true; bInputSuppressed = true;
    }
    bActive = true;
    SetPrivacyPresentation(Entry.bPrivacyPresentation);
    if (!StartNode(Graph.StartNode, Error)) { FString Ignored; Exit(FName(TEXT("entry_failed")), Tick, Ignored); return false; }
    return true;
}

bool FOGSystemicInteractionRuntime::RefreshCharacters(FString& Error)
{
    TArray<FOGFoundationCharacterProjection> Projections;
    for (const auto& Participant : Participants)
    {
        FOGFoundationCharacterProjection Projection;
        if (!Characters.Project(Participant.Binding.Character, Projection, Error)) return false;
        Projections.Add(MoveTemp(Projection));
    }
    for (int32 Index = 0; Index < Participants.Num(); ++Index)
    {
        Participants[Index].Character = MoveTemp(Projections[Index]);
        // Entry projection is read-only until staging and entry persistence succeed.
        // StartNode refreshes after activation before presentation takes ownership.
        if (Presentation && bActive) Presentation->ApplyCharacter(Participants[Index]);
    }
    return true;
}

bool FOGSystemicInteractionRuntime::SolveStaging(const FVector& Preferred, const FOGInteractionActionNode* Node, FString& Error)
{
    if (Preferred.ContainsNaN()) return Fail(Error, TEXT("Staging position is not finite."));
    UWorld* World = Entry.World.Get();
    const FName AnchorTag = Node ? Node->EnvironmentAnchorTag : NAME_None;
    TArray<FTransform> Anchors; Anchors.Add(FTransform(FQuat::Identity, Preferred));
    if (World)
    {
        TArray<FTransform> Nearby;
        for (TActorIterator<AActor> It(World); It; ++It)
            if (It->ActorHasTag(FName(TEXT("OGInteractionAnchor"))) &&
                (AnchorTag.IsNone() || It->ActorHasTag(AnchorTag)) && !It->GetActorTransform().ContainsNaN())
                Nearby.Add(FTransform(It->GetActorQuat(), It->GetActorLocation()));
        Nearby.Sort([Preferred](const FTransform& A, const FTransform& B)
            { return FVector::DistSquared(A.GetLocation(), Preferred) < FVector::DistSquared(B.GetLocation(), Preferred); });
        Anchors.Append(Nearby);
        // Bounded geometry search budget does not bound logical participant arity.
        for (int32 Ring = 1; Ring <= 4; ++Ring) for (int32 Sector = 0; Sector < 8; ++Sector)
        {
            const double Angle = Sector * UE_DOUBLE_PI / 4.0;
            Anchors.Add(FTransform(FQuat::Identity, Preferred + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * (Ring * 350.0)));
        }
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OGInteractionStaging), false);
    for (const auto& Participant : Participants) if (Participant.Binding.Actor.IsValid()) Params.AddIgnoredActor(Participant.Binding.Actor.Get());
    float LargestRadius = 35.f;
    for (const auto& Participant : Participants)
    {
        float Radius, Height; BodySize(Participant.Binding.Actor.Get(), Radius, Height);
        LargestRadius = FMath::Max(LargestRadius, Radius);
    }
    const double Spacing = LargestRadius * 2.0 + 35.0;
    double AuthoredExtent = 0;
    if (Node) for (const auto& Placement : Node->RolePlacements)
        AuthoredExtent = FMath::Max(AuthoredExtent, Placement.RelativeToAnchor.GetTranslation().Size2D());
    const double CircleRadius = FMath::Max(AuthoredExtent + Spacing,
        FMath::Max(Spacing, Participants.Num() * Spacing / (2.0 * UE_DOUBLE_PI)));
    for (const FTransform& Anchor : Anchors)
    {
        TArray<FTransform> Placements;
        TArray<bool> Authored;
        TArray<double> FloorHeights;
        bool bFits = true;
        double GroupLift = -TNumericLimits<double>::Max();
        for (int32 Index = 0; Index < Participants.Num(); ++Index)
        {
            const auto& Participant = Participants[Index];
            float Radius, Height; BodySize(Participant.Binding.Actor.Get(), Radius, Height);
            const double Angle = Index * 2.0 * UE_DOUBLE_PI / Participants.Num();
            const auto* Source = Node ? Node->RolePlacements.FindByPredicate([&Participant](const auto& Placement)
                { return Placement.Role == Participant.Binding.Role; }) : nullptr;
            FTransform Placement;
            if (Source) Placement = Source->RelativeToAnchor * Anchor;
            else
            {
                const FVector Position = Anchor.TransformPosition(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * CircleRadius);
                FRotator Facing = (Anchor.GetLocation() - Position).Rotation(); Facing.Pitch = 0; Facing.Roll = 0;
                Placement = FTransform(Facing, Position);
            }
            // Source scale never resizes the current canonical body.
            Placement.SetScale3D(Participant.Binding.Actor.IsValid() ? Participant.Binding.Actor->GetActorScale3D() : FVector::OneVector);
            if (Placement.ContainsNaN()) { bFits = false; break; }
            double FloorHeight = 0;
            if (World && Participant.Binding.Actor.IsValid())
            {
                const FVector Position = Placement.GetLocation();
                FHitResult Floor;
                if (!World->LineTraceSingleByChannel(Floor, Position + FVector(0,0,500), Position - FVector(0,0,1500), ECC_WorldStatic, Params)
                    || Floor.ImpactNormal.Z < 0.5f) { bFits = false; break; }
                FloorHeight = Floor.ImpactPoint.Z + Height + 2.f;
                if (Source) GroupLift = FMath::Max(GroupLift, FloorHeight - Position.Z);
            }
            Placements.Add(Placement); Authored.Add(Source != nullptr); FloorHeights.Add(FloorHeight);
        }
        if (!bFits) continue;
        if (GroupLift == -TNumericLimits<double>::Max()) GroupLift = 0;
        for (int32 Index = 0; Index < Participants.Num(); ++Index)
        {
            const auto& Participant = Participants[Index];
            FVector Position = Placements[Index].GetLocation();
            // One shared vertical correction retains every source-role offset,
            // including relative height. Observers retain independent floor fit.
            if (Authored[Index]) Position.Z += GroupLift;
            else if (World && Participant.Binding.Actor.IsValid()) Position.Z = FloorHeights[Index];
            Placements[Index].SetLocation(Position);
            if (World && Participant.Binding.Actor.IsValid())
            {
                float Radius, Height; BodySize(Participant.Binding.Actor.Get(), Radius, Height);
                FCollisionObjectQueryParams Objects;
                Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic); Objects.AddObjectTypesToQuery(ECC_Pawn);
                if (World->OverlapAnyTestByObjectType(Position, FQuat::Identity, Objects,
                    FCollisionShape::MakeCapsule(Radius, FMath::Max(Height, Radius)), Params)) { bFits = false; break; }
            }
        }
        if (bFits)
        {
            for (int32 Index = 0; Index < Participants.Num(); ++Index) Participants[Index].StagingTransform = Placements[Index];
            return true;
        }
    }
    return Fail(Error, TEXT("No collision-safe nearby staging solution found; ordinary gameplay is preserved."));
}

bool FOGSystemicInteractionRuntime::Restage(const FVector& Location, FString& Error)
{
    if (!bActive) return Fail(Error, TEXT("No active interaction."));
    const auto* Node = FindNode(CurrentNode);
    if (!SolveStaging(Location, Node, Error)) return false;
    Entry.PreferredLocation = Location;
    for (const auto& Participant : Participants) if (auto* Actor = Participant.Binding.Actor.Get())
        Actor->SetActorTransform(Participant.StagingTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (Node && Presentation) for (const auto& Constraint : Node->Constraints) Presentation->ApplyConstraint(Constraint, Participants);
    return true;
}

bool FOGSystemicInteractionRuntime::SetSequence(const TArray<FName>& Requested, FString& Error)
{
    if (!bActive) return Fail(Error, TEXT("No active interaction."));
    FName Previous = CurrentNode.IsNone() ? LastCompletedNode : CurrentNode;
    for (FName Id : Requested)
    {
        if (!FindNode(Id)) return Fail(Error, TEXT("Requested action is not in the current authored graph."));
        if (Previous.IsNone())
        {
            if (Id != Graph.StartNode) return Fail(Error, TEXT("Sequence must begin at the graph start."));
        }
        else if (!FindNode(Previous)->NextNodes.Contains(Id))
            return Fail(Error, TEXT("Requested contextual transition is absent from the graph."));
        Previous = Id;
    }
    if (CurrentNode.IsNone() && !Requested.IsEmpty())
    {
        if (!StartNode(Requested[0], Error)) return false;
        Sequence = Requested;
        Sequence.RemoveAt(0);
    }
    else Sequence = Requested;
    return true;
}

bool FOGSystemicInteractionRuntime::SelectAction(FName Id, FString& Error)
{
    if (!bActive || !CurrentNode.IsNone()) return Fail(Error, TEXT("Finish or interrupt the current action before selecting another."));
    TArray<FName> Requested; Requested.Add(Id); return SetSequence(Requested, Error);
}

bool FOGSystemicInteractionRuntime::StartNode(FName Id, FString& Error)
{
    const auto* Node = FindNode(Id);
    if (!Node || !RefreshCharacters(Error)) return false;
    if (!SolveStaging(Entry.PreferredLocation, Node, Error)) return false;
    bCompletionRefreshPending = false;
    CurrentNode = Id; Elapsed = 0;
    for (const auto& Participant : Participants) if (auto* Actor = Participant.Binding.Actor.Get())
        Actor->SetActorTransform(Participant.StagingTransform, false, nullptr, ETeleportType::TeleportPhysics);
    UpdatePresentation(*Node);
    return true;
}

bool FOGSystemicInteractionRuntime::SetPacing(double Rate, FString& Error)
{
    if (!bActive || !FMath::IsFinite(Rate) || Rate < 0) return Fail(Error, TEXT("Pacing must be finite and nonnegative for an active interaction."));
    Pacing = Rate;
    for (const auto& Pair : PlayedMontages)
        if (auto* Character = Cast<ACharacter>(Pair.Key.Get()))
            if (auto* Montage = Cast<UAnimMontage>(Pair.Value.Get()))
                if (auto* Mesh = Character->GetMesh())
                    if (auto* Anim = Mesh->GetAnimInstance())
                        Anim->Montage_SetPlayRate(Montage, static_cast<float>(FMath::Min(Rate, static_cast<double>(MAX_flt))));
    return true;
}

bool FOGSystemicInteractionRuntime::Advance(double Delta, int64 Tick, FString& Error)
{
    if (!bActive) return true;
    if (!FMath::IsFinite(Delta) || Delta < 0 || Tick < 0 || Tick < LastWorldTick) return Fail(Error, TEXT("Invalid frame duration or backward canonical tick."));
    LastWorldTick = Tick;
    for (const auto& Saved : Restore)
        if (!Saved.Actor.IsValid()) return Exit(FName(TEXT("participant_unloaded")), Tick, Error);
    if (bCompletionRefreshPending)
    {
        if (!RefreshCharacters(Error)) return false;
        bCompletionRefreshPending = false;
    }
    if (CurrentNode.IsNone() && !Sequence.IsEmpty())
    {
        if (!StartNode(Sequence[0], Error)) return false;
        Sequence.RemoveAt(0);
        // This call recovers a previously failed transition only; its frame
        // duration must not be charged to a newly started action.
        return true;
    }
    if (const auto* ActiveNode = FindNode(CurrentNode)) UpdateSignificance(*ActiveNode);
    if (CurrentNode.IsNone() || Pacing == 0) return true;
    const auto* Node = FindNode(CurrentNode);
    if (!Node) return Fail(Error, TEXT("Current action no longer exists."));
    const double Progress = Delta * Pacing;
    if (!FMath::IsFinite(Progress)) return Fail(Error, TEXT("Pacing duration overflow."));
    // At most one completion per call. Surplus duration is discarded; the host
    // continues advancing world time independently of action pacing/progress.
    Elapsed = FMath::Min(Node->DurationSeconds, Elapsed + Progress);
    if (Elapsed < Node->DurationSeconds) return true;
    if (!CommitEvent(FName(TEXT("interaction.action_completed")), Node->NodeId, Tick, &Node->Consequences, Error)) return false;
    LastCompletedNode = CurrentNode; CurrentNode = NAME_None; Elapsed = 0;
    bCompletionRefreshPending = true;
    StopActionPresentation();
    if (!RefreshCharacters(Error)) return false;
    bCompletionRefreshPending = false;
    if (!Sequence.IsEmpty())
    {
        const FName Next = Sequence[0];
        if (!StartNode(Next, Error)) return false;
        Sequence.RemoveAt(0);
    }
    return true;
}

bool FOGSystemicInteractionRuntime::LoadOptional(const FOGOptionalAssetReference& Reference, UObject*& Asset, FString& Reason)
{
    Asset = nullptr;
    if (!Reference.AssetPath.IsValid()) { Reason = TEXT("Optional presentation reference absent."); return false; }
    const FString Key = Reference.PackageId.ToString() + TEXT("|") + Reference.AssetPath.ToString();
    if (const auto* Existing = SessionAssets.Find(Key); Existing && Existing->IsValid())
    { Asset = Existing->Get(); Reason.Reset(); return true; }
    if (!OptionalAssets.ActivateAndLoad(Reference, Asset, Reason)) return false;
    SessionAssets.Add(Key, Asset);
    return true;
}

void FOGSystemicInteractionRuntime::StopActionPresentation()
{
    for (const auto& Pair : PlayedMontages)
        if (auto* Character = Cast<ACharacter>(Pair.Key.Get()))
            if (auto* Montage = Cast<UAnimMontage>(Pair.Value.Get()))
                if (auto* Mesh = Character->GetMesh())
                    if (auto* Anim = Mesh->GetAnimInstance()) Anim->Montage_Stop(0.1f, Montage);
    PlayedMontages.Reset();
    // Never let a previous scene continue into a different authored action
    // (including a privacy-masked or non-scene node).
    if (Presentation) { Presentation->StopSharedScene(); Presentation->EndAction(); }
    for (auto& Participant : Participants)
    {
        Participant.bHasOptionalPresentation = false;
        Participant.bActionRelevant = false;
        if (Presentation && bActive)
            Presentation->SetSignificance(Participant.Binding.Character.EntityId, false, Participant.bCameraRelevant);
    }
}

void FOGSystemicInteractionRuntime::UpdatePresentation(const FOGInteractionActionNode& Node)
{
    StopActionPresentation();
    // Apply the original multi-actor action as a single synchronized native
    // scene. The presentation bridge verifies all compatible source roles,
    // approved clips and active actor rigs BEFORE any participant plays.
    const bool bSharedSceneStarted = Presentation &&
        !Entry.bPrivacyPresentation &&
        !Node.SharedSceneAction.IsNone() &&
        !Node.SharedSceneContext.IsNone() &&
        Presentation->PlaySharedScene(Node, Participants);
    for (auto& Participant : Participants)
    {
        Participant.bAnimationCompatible = false; Participant.bHasOptionalPresentation = false; Participant.PresentationReason.Reset();
        const FOGInteractionAnimationBinding* Binding=nullptr;
        if (!Node.ResolveAnimationBinding(Participant,Binding,Participant.PresentationReason)) continue;
        if (!Binding) { Participant.PresentationReason = TEXT("No compatible optional rig binding; current character state retained."); continue; }
        Participant.bAnimationCompatible = true;
        if (Entry.bPrivacyPresentation) { Participant.PresentationReason = TEXT("Privacy framing uses the same logical action and consequences."); continue; }
        // A declared shared scene is atomic: never fall back to a solo clip
        // for one participant when another role/rig failed to preflight.
        const bool bSceneRole = !Node.SharedSceneAction.IsNone();
        const bool bNativePlayed = bSceneRole ? bSharedSceneStarted :
            (Presentation && !Binding->SharedGameplayAction.IsNone() &&
             !Binding->SharedGameplayContext.IsNone() &&
             Presentation->PlaySharedAction(Participant.Binding.Character.EntityId,
                 Binding->SharedGameplayAction, Binding->SharedGameplayContext));
        if (bNativePlayed) Participant.bHasOptionalPresentation = true;
        // Never play only one participant of a failed synchronized scene.
        // Standalone source clips and existing optional-asset behavior remain
        // available when this node does not request an authored scene.
        UObject* Animation = nullptr;
        if (!bSceneRole && !bNativePlayed &&
            LoadOptional(Binding->Animation, Animation, Participant.PresentationReason))
        {
            if (auto* Character = Cast<ACharacter>(Participant.Binding.Actor.Get()))
            {
                auto* Mesh = Character->GetMesh();
                if (auto* Montage = Cast<UAnimMontage>(Animation))
                {
                    if (Mesh && Mesh->GetAnimInstance() && Mesh->GetSkeletalMeshAsset() && Montage->GetSkeleton() == Mesh->GetSkeletalMeshAsset()->GetSkeleton())
                    {
                        const float MontageRate = static_cast<float>(FMath::Min(Pacing, static_cast<double>(MAX_flt)));
                        if (Mesh->GetAnimInstance()->Montage_Play(Montage, FMath::Max(0.01f, MontageRate)) > 0)
                        {
                            Mesh->GetAnimInstance()->Montage_SetPlayRate(Montage, MontageRate);
                            PlayedMontages.Add(Participant.Binding.Actor, TWeakObjectPtr<UObject>(Montage));
                            Participant.bHasOptionalPresentation = true;
                        }
                    }
                }
                // Sequences are handed to the character's ordinary animation bridge
                // rather than replacing its AnimInstance or its current body.
            }
            if (Presentation) Presentation->ApplyAnimation(Participant.Binding.Character.EntityId, Animation);
        }
        UObject* Expression = nullptr; UObject* Reaction = nullptr; FString Ignored;
        LoadOptional(Binding->Expression, Expression, Ignored);
        LoadOptional(Binding->Reaction, Reaction, Ignored);
        if (Presentation) Presentation->ApplyReaction(Participant.Binding.Character.EntityId, Expression, Reaction);
    }
    UpdateSignificance(Node);
    if (Presentation) for (const auto& Constraint : Node.Constraints) Presentation->ApplyConstraint(Constraint, Participants);
}

void FOGSystemicInteractionRuntime::UpdateSignificance(const FOGInteractionActionNode& Node)
{
    for (auto& Participant : Participants)
    {
        Participant.bActionRelevant = Participant.Binding.Role == PlayerRole || Node.RequiredRoles.Contains(Participant.Binding.Role);
        Participant.bCameraRelevant = false;
        if (auto* Controller = Entry.ReturnController.Get()) if (auto* Actor = Participant.Binding.Actor.Get())
        {
            FVector CameraLocation; FRotator CameraRotation; Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);
            const FVector Direction = (Actor->GetActorLocation() - CameraLocation).GetSafeNormal();
            Participant.bCameraRelevant = FVector::DotProduct(Direction, CameraRotation.Vector()) > 0.4f;
        }
        if (Presentation) Presentation->SetSignificance(Participant.Binding.Character.EntityId,
            Participant.bActionRelevant, Participant.bCameraRelevant);
    }
}

bool FOGSystemicInteractionRuntime::SetCamera(const FTransform& Transform, FString& Error)
{
    if (!bActive || !Entry.World.IsValid() || !Entry.ReturnController.IsValid() || Transform.ContainsNaN())
        return Fail(Error, TEXT("Camera requires an active embodied interaction and finite transform."));
    if (!CameraActor.IsValid())
    {
        auto* Camera = Entry.World->SpawnActor<ACameraActor>(Transform.GetLocation(), Transform.Rotator());
        if (!Camera) return Fail(Error, TEXT("Unable to create interaction camera."));
        CameraActor = Camera;
    }
    CameraActor->SetActorTransform(Transform);
    Entry.ReturnController->SetViewTarget(CameraActor.Get());
    if (const auto* Node = FindNode(CurrentNode)) UpdateSignificance(*Node);
    return true;
}

void FOGSystemicInteractionRuntime::SetPrivacyPresentation(bool Privacy)
{
    Entry.bPrivacyPresentation = Privacy;
    // Mask only presentation. Runtime state, participant binding and consequences
    // are untouched, even when optional assets are missing.
    for (auto& Saved : Restore) if (auto* Actor = Saved.Actor.Get()) Actor->SetActorHiddenInGame(Privacy || Saved.bHidden);
    if (Presentation) Presentation->SetPrivacy(Privacy);
    if (bActive) if (const auto* Node = FindNode(CurrentNode)) UpdatePresentation(*Node);
}

bool FOGSystemicInteractionRuntime::ApplyStatePatches(const FOGInteractionAuthoredConsequences& Consequences,
    int64 Tick, FString& Error)
{
    // Called only inside CommitEvent's transaction. Never derive a replacement
    // from Participants[].Character, which is a presentation cache.
    auto Resolve = [&](FName Role, FOGEntityId& Entity, FName& Kind, FString& Json, bool& bManifestation)
    {
        const auto* Binding = UniqueRole(Entry.Participants, Role);
        if (!Binding) return Fail(Error, TEXT("State patch role is missing or ambiguous."));
        Entity = Binding->Character.EntityId;
        bool Found = false; int64 Revision = 0;
        if (!Store.TryReadEntity(Entity, Found, Kind, Json, Revision, Error)) return false;
        if (!Found) return Fail(Error, TEXT("State patch participant no longer exists canonically."));
        FOGCharacterManifestationRecord Current;
        if (!Store.TryReadCharacterManifestation(Entity, bManifestation, Current, Error)) return false;
        const auto& Context = Binding->Character;
        if (Context.ManifestationId.IsValid() && (Context.ManifestationId != Entity || !bManifestation))
            return Fail(Error, TEXT("State patch canonical Manifestation binding changed."));
        if (Context.OwnerEntityId.IsValid() && Context.OwnerEntityId !=
            (bManifestation ? Current.OwningRulerId : Entity))
            return Fail(Error, TEXT("State patch canonical ownership changed; rebind the participant."));
        return true;
    };
    for (const auto& Patch : Consequences.PresentationPatches)
    {
        FOGEntityId Entity; FName Kind; FString EntityJson; bool bManifestation = false;
        if (!Resolve(Patch.Role, Entity, Kind, EntityJson, bManifestation)) return false;
        FOGManifestationPresentationStateRecord Record;
        Record.ManifestationId = Entity;
        TSharedPtr<FJsonObject> EntityState, PresentationState;
        if (bManifestation)
        {
            bool Found = false;
            if (!Store.TryReadManifestationPresentationState(Entity, Found, Record, Error)) return false;
            Record.ManifestationId = Entity;
        }
        else
        {
            if (!ReadStateJson(EntityJson, EntityState, Error)) return false;
            const auto* Value = EntityState->Values.Find(TEXT("foundation_presentation"));
            if (Value)
            {
                if ((*Value)->Type != EJson::Object) return Fail(Error, TEXT("NPC foundation presentation must be an object."));
                PresentationState = (*Value)->AsObject();
            }
            else PresentationState = MakeShared<FJsonObject>();
            auto ReadString = [&](const TCHAR* Key, FString& Out)
            {
                if (!PresentationState->HasField(Key)) return true;
                const auto* Field = PresentationState->Values.Find(Key);
                return ((*Field)->Type == EJson::String && PresentationState->TryGetStringField(Key, Out)) ||
                    Fail(Error, TEXT("NPC foundation presentation fields must retain their canonical string types."));
            };
            FString Skin, WorldTick;
            if (!ReadString(TEXT("skin_id"), Skin) || !ReadString(TEXT("outfit_state_json"), Record.OutfitStateJson) ||
                !ReadString(TEXT("variant_state_json"), Record.PresentationVariantStateJson) ||
                !ReadString(TEXT("updated_world_tick"), WorldTick)) return false;
            Record.SelectedSkinId = FOGContentId(Skin);
            if (!WorldTick.IsEmpty() && (!LexTryParseString(Record.UpdatedWorldTick, *WorldTick) ||
                Record.UpdatedWorldTick < 0 || LexToString(Record.UpdatedWorldTick) != WorldTick))
                return Fail(Error, TEXT("NPC presentation has an invalid canonical tick."));
        }
        if (Record.UpdatedWorldTick > Tick) return Fail(Error, TEXT("Presentation patch cannot backdate newer canonical state."));
        if (!Record.SelectedSkinId.IsEmpty() && !Record.SelectedSkinId.IsValid())
            return Fail(Error, TEXT("Canonical presentation skin is invalid; preserving state."));
        if (!MergeStateJson(Record.OutfitStateJson, Patch.OutfitMergeJson, Error) ||
            !MergeStateJson(Record.PresentationVariantStateJson, Patch.VariantMergeJson, Error)) return false;
        if (Patch.SelectedSkinId.IsSet()) Record.SelectedSkinId = Patch.SelectedSkinId.GetValue();
        Record.UpdatedWorldTick = Tick;
        if (bManifestation)
        {
            if (!Store.UpsertManifestationPresentationState(Record, Error)) return false;
        }
        else
        {
            // Preserve unknown presentation metadata and every entity sibling.
            PresentationState->SetStringField(TEXT("skin_id"), Record.SelectedSkinId.ToString());
            PresentationState->SetStringField(TEXT("outfit_state_json"), Record.OutfitStateJson);
            PresentationState->SetStringField(TEXT("variant_state_json"), Record.PresentationVariantStateJson);
            PresentationState->SetStringField(TEXT("updated_world_tick"), LexToString(Tick));
            EntityState->SetObjectField(TEXT("foundation_presentation"), PresentationState);
            if (!Store.UpsertEntity(Entity, Kind, Tick, StateJson(EntityState), Error)) return false;
        }
    }
    for (const auto& Patch : Consequences.AdultContextPatches)
    {
        FOGEntityId Entity; FName Kind; FString Json; bool bManifestation = false;
        if (!Resolve(Patch.Role, Entity, Kind, Json, bManifestation)) return false;
        bool Found = false; FOGCharacterAdultRuntimeStateRecord Record;
        if (!Store.TryReadCharacterAdultRuntimeState(Entity, Found, Record, Error)) return false;
        if (Record.UpdatedWorldTick > Tick) return Fail(Error, TEXT("Context patch cannot backdate newer canonical state."));
        if (!Record.CurrentProfileVariantId.IsEmpty() && !Record.CurrentProfileVariantId.IsValid())
            return Fail(Error, TEXT("Canonical context profile is invalid; preserving state."));
        if (!MergeStateJson(Record.MutableContextStateJson, Patch.MutableContextMergeJson, Error)) return false;
        Record.CharacterEntityId = Entity; Record.UpdatedWorldTick = Tick;
        if (Patch.CurrentProfileVariantId.IsSet()) Record.CurrentProfileVariantId = Patch.CurrentProfileVariantId.GetValue();
        if (!Store.UpsertCharacterAdultRuntimeState(Record, Error)) return false;
    }
    return true;
}

bool FOGSystemicInteractionRuntime::CommitEvent(FName Type, FName Reason, int64 Tick,
    const FOGInteractionAuthoredConsequences* Consequences, FString& Error)
{
    if (!Store.BeginTransaction(Error)) return false;
    auto Abort = [&]() { FString RollbackError; if (!Store.RollbackTransaction(RollbackError)) Error += TEXT("; rollback: ") + RollbackError; return false; };
    if (Consequences && !ValidatePatches(*Consequences, Entry.Participants, Error)) return Abort();
    FOGWorldEvent Event;
    Event.EventId = FOGEntityId::NewId(); Event.EventType = Type; Event.WorldTick = Tick; Event.PrimaryEntity = RulerId;
    for (const auto& Participant : Participants) Event.RelatedEntities.Add(Participant.Binding.Character.EntityId);
    auto Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("session_id"), SessionId.ToString());
    Payload->SetStringField(TEXT("graph_id"), Graph.GraphId.ToString());
    Payload->SetStringField(TEXT("context"), Entry.Context.ToString());
    Payload->SetStringField(TEXT("context_event_id"), Entry.ContextEventId.ToString());
    Payload->SetStringField(TEXT("location_id"), Entry.LocationId.ToString());
    Payload->SetStringField(TEXT("reason"), Reason.ToString());
    TArray<TSharedPtr<FJsonValue>> Bindings;
    for (const auto& Participant : Participants)
    {
        auto Binding = MakeShared<FJsonObject>();
        Binding->SetStringField(TEXT("entity_id"), Participant.Binding.Character.EntityId.ToString());
        Binding->SetStringField(TEXT("role"), Participant.Binding.Role.ToString());
        bool Found = false; FOGCharacterManifestationRecord Current;
        if (!Store.TryReadCharacterManifestation(Participant.Binding.Character.EntityId, Found, Current, Error)) return Abort();
        if (Found) Binding->SetStringField(TEXT("manifestation_id"), Current.ManifestationId.ToString());
        Bindings.Add(MakeShared<FJsonValueObject>(Binding));
    }
    Payload->SetArrayField(TEXT("participants"), Bindings);
    // Privacy and installed asset availability deliberately never enter canonical payload.
    if (!FJsonSerializer::Serialize(Payload, TJsonWriterFactory<>::Create(&Event.PayloadJson)))
    { Error = TEXT("Unable to serialize interaction event."); return Abort(); }
    // Persist the causal event before authored consequences that may reference it by
    // source_event_id. The surrounding transaction keeps event + consequences atomic.
    if (!Store.AppendWorldEvent(Event, Error)) return Abort();
    if (Consequences)
    {
        if (!ApplyStatePatches(*Consequences, Tick, Error)) return Abort();
        for (auto Record : Consequences->Knowledge)
        {
            Record.SourceEventId = Event.EventId; Record.UpdatedWorldTick = Tick;
            if (!Store.UpsertKnowledgeFact(Record, Error)) return Abort();
        }
        for (auto Record : Consequences->Memories)
        {
            if (!Record.MemoryId.IsValid()) Record.MemoryId = FOGEntityId::NewId();
            Record.SourceEventId = Event.EventId;
            if (!Store.UpsertSemanticMemory(Record, Tick, Error)) return Abort();
        }
        for (auto Record : Consequences->Presentation)
        {
            Record.UpdatedWorldTick = Tick;
            if (!Store.UpsertManifestationPresentationState(Record, Error)) return Abort();
        }
        for (auto Record : Consequences->Presence)
        {
            Record.UpdatedWorldTick = Tick;
            if (!Store.UpsertWorldPresence(Record, Error)) return Abort();
        }
    }
    if (!Store.CommitTransaction(Error)) return Abort();
    return true;
}

void FOGSystemicInteractionRuntime::RestoreGameplay()
{
    StopActionPresentation();
    for (const auto& Saved : Restore) if (auto* Actor = Saved.Actor.Get())
    {
        FTransform ReturnTransform = Saved.Transform;
        for (const auto& Participant : Participants)
            if (Participant.Binding.Actor == Saved.Actor && Saved.bHasPresence && Participant.Character.bHasPresence &&
                Participant.Character.Presence.LocationId == Saved.Presence.LocationId)
                ReturnTransform.AddToTranslation(Participant.Character.Presence.LocalPosition - Saved.Presence.LocalPosition);
        Actor->SetActorTransform(ReturnTransform, false, nullptr, ETeleportType::TeleportPhysics);
        Actor->SetActorHiddenInGame(Saved.bHidden);
        if (auto* Character = Cast<ACharacter>(Actor))
        {
            auto* Movement = Character->GetCharacterMovement();
            Movement->SetMovementMode(static_cast<EMovementMode>(Saved.MovementMode), Saved.CustomMovementMode);
            Movement->Velocity = Saved.Velocity;
        }
    }
    if (auto* Controller = Entry.ReturnController.Get(); Controller && bInputSuppressed)
    {
        if (bMoveSuppressed) Controller->SetIgnoreMoveInput(false);
        if (bLookSuppressed) Controller->SetIgnoreLookInput(false);
        if (ReturnViewTarget.IsValid()) Controller->SetViewTarget(ReturnViewTarget.Get());
        Controller->SetControlRotation(ReturnControlRotation);
    }
    if (CameraActor.IsValid()) CameraActor->Destroy();
    CameraActor.Reset(); ReturnViewTarget.Reset(); Restore.Reset(); PlayedMontages.Reset();
    bMoveSuppressed = false; bLookSuppressed = false; bInputSuppressed = false;
    if (Presentation && bActive) Presentation->RestoreGameplay();
    // Presentation clears its actual optional clip/material references before residency leases.
    SessionAssets.Reset();
    OptionalAssets.ReleaseAll();
    bActive = false; CurrentNode = NAME_None; bCompletionRefreshPending = false; Sequence.Reset();
}

bool FOGSystemicInteractionRuntime::Exit(FName Reason, int64 Tick, FString& Error)
{
    if (!bActive) return true;
    bool Persisted = Tick >= LastWorldTick && Tick >= 0 && CommitEvent(FName(TEXT("interaction.exited")), Reason, Tick, nullptr, Error);
    if (Tick < 0 || Tick < LastWorldTick) Error = TEXT("Exit restored gameplay but canonical clock was unavailable or stale.");
    // Refresh actual consequences before returning actors to ordinary presentation.
    FString RefreshError;
    if (!RefreshCharacters(RefreshError) && Persisted) { Error = RefreshError; Persisted = false; }
    // Persistence failure must not trap ordinary controls in staged mode.
    RestoreGameplay();
    return Persisted;
}

FOGInteractionActionGraph FOGSystemicInteractionRuntime::MakeNeutralDiagnosticGraph()
{
    FOGInteractionActionGraph Graph;
    Graph.GraphId = FOGContentId(TEXT("foundation:interaction.diagnostic")); Graph.StartNode = FName(TEXT("align"));
    FOGInteractionActionNode Align; Align.NodeId = Graph.StartNode;
    Align.ActionId = FOGContentId(TEXT("foundation:action.align")); Align.DurationSeconds = 1.0;
    Align.RequiredRoles.Add(PlayerRole); Align.NextNodes.Add(FName(TEXT("handoff"))); Align.NextNodes.Add(Align.NodeId);
    FOGInteractionActionNode Handoff; Handoff.NodeId = FName(TEXT("handoff"));
    Handoff.ActionId = FOGContentId(TEXT("foundation:action.handoff")); Handoff.DurationSeconds = 2.0;
    Handoff.RequiredRoles.Add(PlayerRole); Handoff.NextNodes.Add(Align.NodeId); Handoff.NextNodes.Add(Handoff.NodeId);
    Graph.Nodes.Add(Align); Graph.Nodes.Add(Handoff); return Graph;
}

bool FOGInteractionActionNode::ResolveAnimationBinding(const FOGInteractionParticipantState& Participant,
    const FOGInteractionAnimationBinding*& Out,FString& Error) const
{
    Out=nullptr; Error.Reset(); int32 Best=-1; bool Ambiguous=false;
    const auto Manifestation=Participant.Character.bHasManifestation?Participant.Character.Manifestation.ManifestationId:
        Participant.Binding.Character.ManifestationId;
    for (const auto& Binding:AnimationBindings)
    {
        const bool Shared=!Binding.SharedGameplayAction.IsNone() || !Binding.SharedSceneRole.IsNone();
        if (Binding.Role!=Participant.Binding.Role ||
            (Binding.RigFamilyId.IsValid()?Binding.RigFamilyId!=Participant.Binding.RigFamilyId:!Shared) ||
            (Binding.ManifestationId.IsValid() && Binding.ManifestationId!=Manifestation) ||
            (!Binding.VersionId.IsEmpty() && (!Participant.Character.bHasManifestation ||
                Binding.VersionId!=Participant.Character.Manifestation.ActiveVersionId))) continue;
        const int32 Specificity=(Binding.ManifestationId.IsValid()?4:0)+(!Binding.VersionId.IsEmpty()?2:0)+(Binding.RigFamilyId.IsValid()?1:0);
        if (Specificity==Best) Ambiguous=true;
        if (Specificity>Best) { Out=&Binding; Best=Specificity; Ambiguous=false; }
    }
    if (Ambiguous) { Out=nullptr; Error=TEXT("Original motion selectors are ambiguous for this actual role/rig."); return false; }
    return true;
}
