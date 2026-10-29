#include "Interaction/OGInteractionDefinition.h"

namespace
{
bool InvalidInteraction(FString& Error, const TCHAR* Text) { Error = Text; return false; }
}

bool UOGInteractionDefinition::BuildGraph(FOGInteractionActionGraph& Out, FString& Error) const
{
    Out = {}; Error.Reset();
    if (!GraphId.IsValid() || SourceRevision.IsEmpty() || DisplayName.IsEmpty() ||
        Roles.IsEmpty() || Nodes.IsEmpty() || StartNode.IsNone())
        return InvalidInteraction(Error, TEXT("Interaction requires original source revision, ID, label, roles and nodes."));
    TSet<FName> RoleSet, NodeSet;
    for (FName Role : Roles)
    {
        if (Role.IsNone() || RoleSet.Contains(Role))
            return InvalidInteraction(Error, TEXT("Original source roles must be nonempty and unique."));
        RoleSet.Add(Role);
    }
    if (!RoleSet.Contains(TEXT("player")))
        return InvalidInteraction(Error, TEXT("Interaction must bind the real Ruler player role."));
    FOGInteractionActionGraph Candidate;
    Candidate.GraphId = GraphId; Candidate.StartNode = StartNode;
    Candidate.DisplayName = DisplayName.ToString(); Candidate.Description = Description.ToString();
    Candidate.bMaturePresentation = bMaturePresentation;
    for (const auto& Source : Nodes)
    {
        if (Source.NodeId.IsNone() || NodeSet.Contains(Source.NodeId) || !Source.ActionId.IsValid() ||
            !FMath::IsFinite(Source.DurationSeconds) || Source.DurationSeconds <= 0)
            return InvalidInteraction(Error, TEXT("Interaction node ID, action or duration is invalid."));
        NodeSet.Add(Source.NodeId);
        FOGInteractionActionNode& Node = Candidate.Nodes.AddDefaulted_GetRef();
        Node.NodeId = Source.NodeId; Node.ActionId = Source.ActionId;
        Node.DisplayName = Source.DisplayName.ToString(); Node.DurationSeconds = Source.DurationSeconds;
        Node.RequiredRoles = Source.RequiredRoles; Node.NextNodes = Source.NextNodes;
        Node.SharedSceneAction = Source.SceneAction; Node.SharedSceneContext = Source.SceneContext;
        Node.EnvironmentAnchorTag = Source.EnvironmentAnchorTag;
        if (Node.SharedSceneAction.IsNone() != Node.SharedSceneContext.IsNone())
            return InvalidInteraction(Error, TEXT("Source scene action and context must be supplied together."));
        TSet<FName> Required, Placements;
        for (FName Role : Source.RequiredRoles)
        {
            if (!RoleSet.Contains(Role) || Required.Contains(Role))
                return InvalidInteraction(Error, TEXT("Required role is missing or repeated."));
            Required.Add(Role);
        }
        TSet<FString> TrackKeys;
        for (const auto& Track : Source.Tracks)
        {
            if (!RoleSet.Contains(Track.Role) || Track.Action.IsNone() != Track.Context.IsNone())
                return InvalidInteraction(Error, TEXT("Motion track needs an existing role and paired action/context."));
            const FString Key = Track.Role.ToString() + TEXT("|") + Track.RigFamilyId.ToString() + TEXT("|") +
                Track.ManifestationId.ToString() + TEXT("|") + Track.VersionId.ToString();
            if (TrackKeys.Contains(Key)) return InvalidInteraction(Error, TEXT("Duplicate source motion selector."));
            TrackKeys.Add(Key);
            if (!Node.SharedSceneAction.IsNone() && Required.Contains(Track.Role) && Track.SourceSceneRole.IsNone())
                return InvalidInteraction(Error, TEXT("Synchronized active roles need genuine original scene tracks."));
            auto& Binding = Node.AnimationBindings.AddDefaulted_GetRef();
            Binding.Role = Track.Role; Binding.RigFamilyId = Track.RigFamilyId;
            Binding.ManifestationId = Track.ManifestationId; Binding.VersionId = Track.VersionId;
            Binding.SharedGameplayAction = Track.Action; Binding.SharedGameplayContext = Track.Context;
            Binding.SharedSceneRole = Track.SourceSceneRole;
            Binding.ExpressionTracks = Track.ExpressionTracks;
            if (!Binding.ValidateExpressionTracks(Node.DurationSeconds,Error)) return false;
        }
        if (!Node.SharedSceneAction.IsNone())
        {
            if (Required.Num() < 2) return InvalidInteraction(Error, TEXT("A synchronized scene needs at least two active source roles."));
            for (FName Role : Required)
                if (!Source.Tracks.ContainsByPredicate([Role](const auto& T) { return T.Role == Role && !T.SourceSceneRole.IsNone(); }))
                    return InvalidInteraction(Error, TEXT("Synchronized scene is missing an active source role."));
        }
        for (const auto& Placement : Source.Placements)
        {
            if (!RoleSet.Contains(Placement.Role) || Placements.Contains(Placement.Role) ||
                Placement.RelativeToAnchor.ContainsNaN() || !Placement.RelativeToAnchor.GetRotation().IsNormalized())
                return InvalidInteraction(Error, TEXT("Source role placement is invalid or duplicated."));
            Placements.Add(Placement.Role);
            FOGInteractionRolePlacement Native; Native.Role = Placement.Role;
            Native.RelativeToAnchor = Placement.RelativeToAnchor; Node.RolePlacements.Add(Native);
        }
        if (!Node.SharedSceneAction.IsNone())
            for (FName Role:Required)
                if (!Placements.Contains(Role))
                    return InvalidInteraction(Error,TEXT("Synchronized source tracks require their original active-role placements."));
        for (const auto& Change : Source.StateChanges)
        {
            if (!RoleSet.Contains(Change.Role)) return InvalidInteraction(Error, TEXT("State change names an unknown role."));
            FOGInteractionPresentationPatch Patch; Patch.Role = Change.Role;
            if (Change.bChangeSkin) Patch.SelectedSkinId = Change.SkinId;
            Patch.OutfitMergeJson = Change.OutfitMergeJson; Patch.VariantMergeJson = Change.VariantMergeJson;
            if (Change.bChangeSkin || Change.OutfitMergeJson != TEXT("{}") || Change.VariantMergeJson != TEXT("{}"))
                Node.Consequences.PresentationPatches.Add(Patch);
            if (Change.bChangeContextProfile || Change.ContextMergeJson != TEXT("{}"))
            {
                FOGInteractionAdultContextPatch Context; Context.Role = Change.Role;
                if (Change.bChangeContextProfile) Context.CurrentProfileVariantId = Change.ContextProfileId;
                Context.MutableContextMergeJson = Change.ContextMergeJson;
                Node.Consequences.AdultContextPatches.Add(Context);
            }
        }
    }
    if (!NodeSet.Contains(StartNode)) return InvalidInteraction(Error, TEXT("Interaction start node does not exist."));
    for (const auto& Node : Candidate.Nodes)
        for (FName Next : Node.NextNodes)
            if (!NodeSet.Contains(Next)) return InvalidInteraction(Error, TEXT("Interaction transition targets a missing node."));
    TSet<FName> Reached; TArray<FName> Pending{StartNode};
    while (!Pending.IsEmpty())
    {
        const FName Id = Pending.Pop();
        if (Reached.Contains(Id)) continue;
        Reached.Add(Id);
        const auto* Node = Candidate.Nodes.FindByPredicate([Id](const auto& N) { return N.NodeId == Id; });
        Pending.Append(Node->NextNodes);
    }
    if (Reached.Num() != Candidate.Nodes.Num()) return InvalidInteraction(Error, TEXT("Interaction contains unreachable nodes."));
    Out = MoveTemp(Candidate); return true;
}

bool UOGInteractionDefinition::BindRoles(const TArray<FOGInteractionParticipant>& Selection,
    TArray<FOGInteractionParticipant>& Out, FString& Error) const
{
    Out.Reset(); FOGInteractionActionGraph Checked;
    if (!BuildGraph(Checked, Error)) return false;
    if (Selection.Num() < Roles.Num())
        return InvalidInteraction(Error, TEXT("Select enough actual characters for the original source roles."));
    const auto* Ruler = Selection.FindByPredicate([](const auto& P) { return P.Role == TEXT("player"); });
    if (!Ruler) return InvalidInteraction(Error, TEXT("The real Ruler physical participant is required."));
    TSet<FOGEntityId> Entities; TSet<TWeakObjectPtr<AActor>> Bodies;
    for (const auto& P : Selection)
    {
        if (!P.Character.EntityId.IsValid() || !P.Actor.IsValid() || Entities.Contains(P.Character.EntityId) || Bodies.Contains(P.Actor))
            return InvalidInteraction(Error, TEXT("Each selected participant needs a distinct real canonical body."));
        Entities.Add(P.Character.EntityId); Bodies.Add(P.Actor);
    }
    const auto IsExplicit=[&](const FOGInteractionParticipant& P)
    { return Roles.Contains(P.Role) && (P.Role!=TEXT("participant") || P.bExplicitSourceRole); };
    TSet<FName> ExplicitRoles;
    for (const auto& Participant:Selection)
        if (IsExplicit(Participant))
        {
            if (ExplicitRoles.Contains(Participant.Role))
                return InvalidInteraction(Error,TEXT("An original source role was explicitly assigned more than once."));
            ExplicitRoles.Add(Participant.Role);
        }
    TArray<FOGInteractionParticipant> Candidate; Candidate.Add(*Ruler);
    Candidate[0].Role=TEXT("player");
    TSet<FName> UsedRoles=ExplicitRoles;
    for (const auto& Participant:Selection)
    {
        if (&Participant==Ruler) continue;
        auto Copy=Participant;
        if (!IsExplicit(Copy))
        {
            FName Available;
            for (FName Role:Roles) if (!UsedRoles.Contains(Role)) { Available=Role; break; }
            if (!Available.IsNone()) Copy.Role=Available;
            else
            {
                int32 Suffix=Candidate.Num();
                do { Copy.Role=FName(*FString::Printf(TEXT("observer_%d"),Suffix++)); }
                while (UsedRoles.Contains(Copy.Role) || Roles.Contains(Copy.Role));
            }
        }
        UsedRoles.Add(Copy.Role); Candidate.Add(MoveTemp(Copy));
    }
    Out = MoveTemp(Candidate); return true;
}
