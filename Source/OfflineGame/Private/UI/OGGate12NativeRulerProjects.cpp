#include "UI/OGGate12NativeRulerWidget.h"

#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "World/OGTerritoryControlService.h"
#include "World/OGTerritoryProjectDefinition.h"

namespace
{
const FLinearColor ProjectGold(.72f, .62f, .44f, 1.f);
const FLinearColor ProjectWarning(.88f, .57f, .43f, 1.f);

struct FProjectOrdersRuntime
{
    IOGWorldStore* Store = nullptr;
    FOGFoundationStrategicRuntime* Strategy = nullptr;
    FOGCanonicalClockRuntime* Clock = nullptr;
    int64 Tick = -1;

    explicit FProjectOrdersRuntime(UGameInstance* Game)
    {
        auto* Core = Game ? Game->GetSubsystem<UOGGameCoreSubsystem>() : nullptr;
        if (!Core || !Core->IsCoreReady()) return;
        Store = Core->GetWorldStore();
        Strategy = Core->GetFoundationStrategicRuntime();
        Clock = Core->GetCanonicalClockRuntime();
        Tick = Core->GetCanonicalWorldTick();
    }
    bool IsReady() const { return Store && Store->IsOpen() && Strategy && Clock && Tick >= 0; }
};

FString SavedOrderLabel(const FString& Json, const FOGContentId& Fallback)
{
    TSharedPtr<FJsonObject> Payload;
    FString Label;
    if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Payload) && Payload &&
        Payload->TryGetStringField(TEXT("display_name"), Label) && !Label.IsEmpty())
        return Label;
    return Fallback.ToString();
}

const TCHAR* ProjectStatus(EOGProjectStatus Status)
{
    switch (Status)
    {
    case EOGProjectStatus::Planned: return TEXT("Planned");
    case EOGProjectStatus::Active: return TEXT("Active");
    case EOGProjectStatus::Completed: return TEXT("Completed");
    case EOGProjectStatus::Failed: return TEXT("Failed");
    case EOGProjectStatus::Cancelled: return TEXT("Cancelled");
    }
    return TEXT("Unknown");
}

bool ReadProjectOrders(IOGWorldStore& Store, const FOGEntityId& RulerId, const FOGEntityId& LocationId,
    TArray<FOGProjectRecord>& Out, FString& Error)
{
    Out.Reset();
    FOGWorldEventCursor Before;
    TSet<FOGEntityId> Seen;
    do
    {
        TArray<FOGWorldEvent> Events;
        FOGWorldEventCursor Next;
        if (!Store.ListWorldEventsPage(RulerId, FName(TEXT("foundation.strategy.project_started")),
                true, 64, Before, Events, Next, Error))
            return false;
        for (const FOGWorldEvent& Event : Events)
        {
            if (Event.PrimaryEntity != RulerId || Event.RelatedEntities.IsEmpty())
                continue;
            const FOGEntityId& ProjectId = Event.RelatedEntities[0];
            if (!ProjectId.IsValid() || Seen.Contains(ProjectId))
                continue;
            Seen.Add(ProjectId);
            bool bFound = false;
            FOGProjectRecord Project;
            if (!Store.TryReadProject(ProjectId, bFound, Project, Error))
                return false;
            if (bFound && Project.OwnerEntityId == RulerId &&
                (!LocationId.IsValid() || Project.LocationId == LocationId))
                Out.Add(MoveTemp(Project));
        }
        Before = Next;
    } while (Before.EventId.IsValid());
    return true;
}

bool CanIssueProject(const FProjectOrdersRuntime& Runtime, const FOGEntityId& RulerId,
    const FOGTerritorySummaryViewModel& Holding, FString& Error)
{
    FOGTerritoryEffectiveControlResult Control;
    if (!FOGTerritoryControlService(*Runtime.Store).EvaluateEffectiveControl(Holding.TerritoryId,
            Runtime.Tick, Control, Error))
        return false;
    if (Control.EffectiveRulerIds.Contains(RulerId))
    {
        TArray<FOGTerritoryClaimRecord> Claims;
        if (!Runtime.Store->ListActiveClaimsForLocation(Holding.RootLocationId, Claims, Error))
            return false;
        if (Claims.ContainsByPredicate([&](const FOGTerritoryClaimRecord& Claim)
            { return Claim.TerritoryId == Holding.TerritoryId && Claim.RulerId == RulerId; }))
            return true;
    }
    Error = TEXT("You need effective control of this location to issue an order.");
    return false;
}

bool RegisterProjectDeadline(FProjectOrdersRuntime& Runtime, FOGFoundationStrategicContext& Context,
    const FOGProjectRecord& Project, FString& Error)
{
    if (Project.Status != EOGProjectStatus::Active) return true;
    FOGWorldDirectorScheduleRecord Schedule;
    return Runtime.Strategy->ScheduleDueAction(Context, Project.ProjectId, FName(TEXT("project")),
            Runtime.Tick, Project.ResolveWorldTick, Schedule, Error) &&
        Runtime.Clock->RegisterDueAction(Schedule.ScheduleId, FOGFoundationStrategicRuntime::DueResolverKey(),
            Context.OwnerId, 0, 0, false, false, Error);
}
}

void UOGGate12NativeRulerWidget::ClearTerritoryProjectReview()
{
    PendingProjectDefinitionId = FOGContentId();
    PendingProjectReviewId.Invalidate();
    PendingProjectLibraryRevision = -1;
}

void UOGGate12NativeRulerWidget::ResetTerritoryProjectOrders()
{
    ClearTerritoryProjectReview();
    ProjectOrdersRulerId = FOGEntityId();
    ProjectOrdersTerritoryId = FOGEntityId();
    bProjectOrdersOpen = false;
    ProjectOrderFeedback.Reset();
}

void UOGGate12NativeRulerWidget::RestoreTerritoryProjectDeadlines(const UOGGate12RulerSessionSubsystem& Session)
{
    if (bProjectOrderInFlight) return;
    TGuardValue<bool> InFlight(bProjectOrderInFlight, true);
    FProjectOrdersRuntime Runtime(GetGameInstance());
    if (!Runtime.IsReady()) return;

    FString Error;
    TArray<FOGProjectRecord> Orders;
    if (!ReadProjectOrders(*Runtime.Store, Session.BoundRulerId, FOGEntityId(), Orders, Error))
    {
        UE_LOG(LogTemp, Warning, TEXT("Territory project restore: %s"), *Error);
        return;
    }

    FOGFoundationStrategicContext Context;
    Context.OwnerId = Session.BoundRulerId;
    for (const FOGProjectRecord& Order : Orders)
    {
        if (Order.Status != EOGProjectStatus::Active) continue;
        if (Runtime.Tick < Order.ResolveWorldTick)
        {
            if (RegisterProjectDeadline(Runtime, Context, Order, Error)) continue;
        }
        else
        {
            FOGFoundationStrategicCommand Command;
            Command.Action = EOGFoundationStrategicAction::RefreshProject;
            Command.SubjectId = Order.ProjectId;
            Command.WorldTick = Runtime.Tick;
            FOGFoundationStrategicCommandResult Result;
            if (Runtime.Strategy->Execute(Context, Command, Result, Error)) continue;
        }
        UE_LOG(LogTemp, Warning, TEXT("Territory project restore: %s"), *Error);
    }
}

void UOGGate12NativeRulerWidget::BuildTerritoryProjectOrders(const UOGGate12RulerSessionSubsystem& Session,
    const FOGTerritorySummaryViewModel& Holding)
{
    if (ProjectOrdersRulerId != Session.BoundRulerId || ProjectOrdersTerritoryId != Holding.TerritoryId)
    {
        ResetTerritoryProjectOrders();
        ProjectOrdersRulerId = Session.BoundRulerId;
        ProjectOrdersTerritoryId = Holding.TerritoryId;
    }

    AddAction(Content, bProjectOrdersOpen ? TEXT("CLOSE PROJECT ORDERS") : TEXT("PROJECT ORDERS"),
        TEXT("project-orders/toggle"));
    if (!bProjectOrdersOpen) return;

    if (!ProjectOrderFeedback.IsEmpty())
        AddMessage(Content, ProjectOrderFeedback, 16);
    FProjectOrdersRuntime Runtime(GetGameInstance());
    if (!Runtime.IsReady() || !Holding.RootLocationId.IsValid())
    {
        ClearTerritoryProjectReview();
        AddMessage(Content, TEXT("Project orders are unavailable for this location."), 16, ProjectWarning);
        return;
    }

    AddAction(Content, TEXT("REFRESH PROJECT PROGRESS"), TEXT("project-orders/refresh"));
    FString Error;
    TArray<FOGProjectRecord> Orders;
    if (ReadProjectOrders(*Runtime.Store, Session.BoundRulerId, Holding.RootLocationId, Orders, Error))
    {
        AddMessage(Content, TEXT("ISSUED ORDERS"), 18, ProjectGold);
        if (Orders.IsEmpty()) AddMessage(Content, TEXT("No orders have been issued here."), 16);
        for (const FOGProjectRecord& Project : Orders)
        {
            AddMessage(Content, FString::Printf(TEXT("%s / %s / %.2f%%"),
                *SavedOrderLabel(Project.PayloadJson, Project.ProjectTypeId),
                ProjectStatus(Project.Status), Project.ProgressBps / 100.0), 17);
            TArray<FOGProjectPhaseRecord> Phases;
            if (!Runtime.Store->ListProjectPhases(Project.ProjectId, Phases, Error))
            {
                AddMessage(Content, Error, 15, ProjectWarning);
                continue;
            }
            for (const FOGProjectPhaseRecord& Phase : Phases)
            {
                AddMessage(Content, FString::Printf(TEXT("%s / %s / %.2f%%"),
                    *SavedOrderLabel(Phase.PayloadJson, Phase.PhaseId), *Phase.Status.ToString(),
                    Phase.ProgressBps / 100.0), 15);
            }
        }
    }
    else AddMessage(Content, Error, 16, ProjectWarning);

    AddMessage(Content, TEXT("AVAILABLE ORDERS"), 18, ProjectGold);
    Error.Reset();
    if (!CanIssueProject(Runtime, Session.BoundRulerId, Holding, Error))
    {
        ClearTerritoryProjectReview();
        AddMessage(Content, Error, 16, ProjectWarning);
        return;
    }

    auto* Library = UOGDefinitionLibrary::Get();
    TArray<UDataAsset*> Assets;
    if (Library)
        Library->List(UOGTerritoryProjectDefinition::DefinitionSystem(),
            UOGTerritoryProjectDefinition::StaticClass(), Assets);
    if (PendingProjectReviewId.IsValid() &&
        (!Library || PendingProjectLibraryRevision != Library->GetRevision()))
    {
        ClearTerritoryProjectReview();
        AddMessage(Content, TEXT("Installed project definitions changed. Review the order again."), 16, ProjectWarning);
    }

    bool bHasOrder = false;
    bool bHasReviewedOrder = false;
    for (UDataAsset* Asset : Assets)
    {
        const auto* Definition = Cast<UOGTerritoryProjectDefinition>(Asset);
        if (!Definition || Definition->LocationId != Holding.RootLocationId) continue;
        bHasOrder = true;
        const bool bReviewed = PendingProjectReviewId.IsValid() && PendingProjectDefinitionId == Definition->DefinitionId;
        bHasReviewedOrder |= bReviewed;
        AddMessage(Content, Definition->DisplayName.ToString(), 17);
        if (!bReviewed)
        {
            AddAction(Content, TEXT("REVIEW ORDER"), TEXT("project-orders/review/") + Definition->DefinitionId.ToString());
            continue;
        }

        AddMessage(Content, FString::Printf(TEXT("Duration / %lld world ticks"),
            static_cast<long long>(Definition->DurationWorldTicks)), 15);
        bool bAffordable = true;
        for (const auto& Cost : Definition->ResourceCosts)
        {
            bool bFound = false;
            int64 Balance = 0;
            if (!Runtime.Store->TryReadResourceBalance(Session.BoundRulerId, Cost.ResourceId, bFound, Balance, Error))
            {
                bAffordable = false;
                AddMessage(Content, Error, 15, ProjectWarning);
                continue;
            }
            const bool bCovered = bFound && Balance >= Cost.Amount;
            bAffordable &= bCovered;
            AddMessage(Content, FString::Printf(TEXT("%s / cost %lld / available %lld"),
                *Cost.ResourceId.ToString(), static_cast<long long>(Cost.Amount),
                static_cast<long long>(bFound ? Balance : 0)), 15, bCovered ? ProjectGold : ProjectWarning);
        }
        if (Definition->bExplicitNoCost)
            AddMessage(Content, TEXT("No resource cost."), 15);
        for (const auto& Phase : Definition->Phases)
        {
            AddMessage(Content, FString::Printf(TEXT("%s / ticks %lld–%lld after issuance"),
                *Phase.DisplayName.ToString(), static_cast<long long>(Phase.StartOffsetWorldTicks),
                static_cast<long long>(Phase.ResolveOffsetWorldTicks)), 15);
        }
        AddAction(Content, TEXT("ISSUE ORDER"), TEXT("project-orders/confirm/") + PendingProjectReviewId.ToString(), bAffordable);
        AddAction(Content, TEXT("CANCEL REVIEW"), TEXT("project-orders/cancel"));
    }
    if (PendingProjectReviewId.IsValid() && !bHasReviewedOrder) ClearTerritoryProjectReview();
    if (!bHasOrder)
        AddMessage(Content, TEXT("No authored project orders are installed for this location."), 16);
}

void UOGGate12NativeRulerWidget::HandleTerritoryProjectAction(const FString& Action,
    UOGGate12RulerSessionSubsystem& Session)
{
    if (bProjectOrderInFlight || !Session.IsBoundToStoredRuler() ||
        Session.CurrentDestination != EOGRulerPrimaryDestination::Territory ||
        Session.BoundRulerId != ProjectOrdersRulerId || SelectedTerritoryId != ProjectOrdersTerritoryId)
        return;
    const auto* Holding = Session.TerritoryProjection.Territories.FindByPredicate(
        [&](const auto& Entry) { return Entry.TerritoryId == ProjectOrdersTerritoryId; });
    if (!Holding) return;

    const FString ConfirmPrefix = TEXT("project-orders/confirm/");
    const bool bConfirm = Action.StartsWith(ConfirmPrefix);
    if (bConfirm && (!PendingProjectReviewId.IsValid() ||
        Action != ConfirmPrefix + PendingProjectReviewId.ToString()))
        return;

    TGuardValue<bool> InFlight(bProjectOrderInFlight, true);
    if (Action == TEXT("project-orders/toggle"))
    {
        bProjectOrdersOpen = !bProjectOrdersOpen;
        ClearTerritoryProjectReview();
        ProjectOrderFeedback.Reset();
        if (!bProjectOrdersOpen) { BuildCurrentPage(); return; }
    }
    else if (!bProjectOrdersOpen) return;

    ProjectOrderFeedback.Reset();
    FString Error;
    FProjectOrdersRuntime Runtime(GetGameInstance());
    if (!Runtime.IsReady() || !Holding->RootLocationId.IsValid())
    {
        ClearTerritoryProjectReview();
        ProjectOrderFeedback = TEXT("Project orders are unavailable for this location.");
        BuildCurrentPage();
        return;
    }

    FOGFoundationStrategicContext Context;
    Context.OwnerId = Session.BoundRulerId;
    Context.TerritoryId = Holding->TerritoryId;
    Context.LocationId = Holding->RootLocationId;
    const FString ReviewPrefix = TEXT("project-orders/review/");
    auto* Library = UOGDefinitionLibrary::Get();
    if (Action == TEXT("project-orders/cancel"))
    {
        ClearTerritoryProjectReview();
    }
    else if (Action.StartsWith(ReviewPrefix))
    {
        ClearTerritoryProjectReview();
        const FOGContentId Id(Action.Mid(ReviewPrefix.Len()));
        const auto* Definition = Library ? Cast<UOGTerritoryProjectDefinition>(
            Library->Resolve(UOGTerritoryProjectDefinition::DefinitionSystem(), Id)) : nullptr;
        if (Definition && Definition->LocationId == Context.LocationId &&
            CanIssueProject(Runtime, Context.OwnerId, *Holding, Error))
        {
            PendingProjectDefinitionId = Id;
            PendingProjectReviewId = FGuid::NewGuid();
            PendingProjectLibraryRevision = Library->GetRevision();
        }
        else if (Error.IsEmpty()) Error = TEXT("This authored order is unavailable at the selected location.");
    }
    else if (bConfirm)
    {
        const FOGContentId Id = PendingProjectDefinitionId;
        const int64 Revision = PendingProjectLibraryRevision;
        ClearTerritoryProjectReview();
        const auto* Definition = Library ? Cast<UOGTerritoryProjectDefinition>(
            Library->Resolve(UOGTerritoryProjectDefinition::DefinitionSystem(), Id)) : nullptr;
        if (!Definition || Library->GetRevision() != Revision || Definition->LocationId != Context.LocationId)
        {
            Error = TEXT("The authored order changed. Review it again.");
        }
        else
        {
            const FString Name = Definition->DisplayName.ToString();
            FOGEntityId ProjectId, ScheduleId;
            if (Definition->Issue(*Runtime.Store, *Runtime.Strategy, Context.OwnerId, Context.TerritoryId,
                    Runtime.Tick, ProjectId, ScheduleId, Error))
            {
                ProjectOrderFeedback = Name + TEXT(" order saved.");
                if (ScheduleId.IsValid() &&
                    !Runtime.Clock->RegisterDueAction(ScheduleId, FOGFoundationStrategicRuntime::DueResolverKey(),
                        Context.OwnerId, 0, 0, false, false, Error))
                {
                    ProjectOrderFeedback += TEXT(" Automatic completion is pending. Refresh project progress to retry.");
                    if (!Error.IsEmpty()) ProjectOrderFeedback += TEXT(" ") + Error;
                    Error.Reset();
                }
            }
        }
    }
    else if (Action == TEXT("project-orders/refresh") || Action == TEXT("project-orders/toggle"))
    {
        ClearTerritoryProjectReview();
        TArray<FOGProjectRecord> Orders;
        if (ReadProjectOrders(*Runtime.Store, Context.OwnerId, Context.LocationId, Orders, Error))
        {
            TArray<FString> Warnings;
            for (const FOGProjectRecord& Order : Orders)
            {
                if (Order.Status != EOGProjectStatus::Active && Order.Status != EOGProjectStatus::Completed)
                    continue;
                FString Warning;
                FOGFoundationStrategicCommand Command;
                Command.Action = EOGFoundationStrategicAction::RefreshProject;
                Command.SubjectId = Order.ProjectId;
                Command.WorldTick = Runtime.Tick;
                FOGFoundationStrategicCommandResult Result;
                FOGProjectRecord Current;
                bool bFound = false;
                if (!Runtime.Strategy->Execute(Context, Command, Result, Warning) ||
                    !Runtime.Store->TryReadProject(Order.ProjectId, bFound, Current, Warning) || !bFound ||
                    !RegisterProjectDeadline(Runtime, Context, Current, Warning))
                {
                    Warnings.Add(Warning.IsEmpty() ? TEXT("A saved order could not refresh.") : Warning);
                }
            }
            ProjectOrderFeedback = FString::Join(Warnings, TEXT(" "));
        }
    }
    if (!Error.IsEmpty()) ProjectOrderFeedback = Error;
    BuildCurrentPage();
}
