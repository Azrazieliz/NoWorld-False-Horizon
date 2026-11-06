#include "World/OGTerritoryProjectDefinition.h"

#include "Dom/JsonObject.h"
#include "Runtime/OGFoundationStrategicRuntime.h"
#include "Serialization/JsonSerializer.h"
#include "World/OGProjectService.h"

namespace
{
FString SaveProjectLabel(const FText& Label, const FString* Source = nullptr)
{
    auto Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("display_name"), Label.ToString());
    if (Source)
        Payload->SetStringField(TEXT("original_source_reference"), *Source);
    FString Json;
    FJsonSerializer::Serialize(Payload, TJsonWriterFactory<>::Create(&Json));
    return Json;
}
}

bool UOGTerritoryProjectDefinition::ValidateDefinition_Implementation(FString& Error) const
{
    if (!Super::ValidateDefinition_Implementation(Error))
        return false;
    if (System != DefinitionSystem() || DisplayName.ToString().TrimStartAndEnd().IsEmpty() ||
        OriginalSourceReference.TrimStartAndEnd().IsEmpty() || !LocationId.IsValid() ||
        DurationWorldTicks < 0 || bExplicitNoCost != ResourceCosts.IsEmpty())
    {
        Error = TEXT("A territory project requires its authored source, name, location, duration and costs.");
        return false;
    }
    TSet<FOGContentId> Resources;
    for (const auto& Cost : ResourceCosts)
    {
        if (!Cost.ResourceId.IsValid() || Cost.Amount <= 0 || Resources.Contains(Cost.ResourceId))
        {
            Error = TEXT("Project costs require distinct resource IDs and positive amounts.");
            return false;
        }
        Resources.Add(Cost.ResourceId);
    }
    TSet<FOGContentId> PhaseIds;
    for (const auto& Phase : Phases)
    {
        if (!Phase.PhaseId.IsValid() || PhaseIds.Contains(Phase.PhaseId) ||
            Phase.DisplayName.ToString().TrimStartAndEnd().IsEmpty() || Phase.StartOffsetWorldTicks < 0 ||
            Phase.ResolveOffsetWorldTicks < Phase.StartOffsetWorldTicks ||
            Phase.ResolveOffsetWorldTicks > DurationWorldTicks)
        {
            Error = TEXT("Project phases require distinct IDs, names and dates within the project duration.");
            return false;
        }
        PhaseIds.Add(Phase.PhaseId);
    }
    return true;
}

bool UOGTerritoryProjectDefinition::Issue(IOGWorldStore& Store, FOGFoundationStrategicRuntime& Runtime,
    const FOGEntityId& RulerId, const FOGEntityId& TerritoryId, int64 WorldTick,
    FOGEntityId& OutProjectId, FOGEntityId& OutScheduleId, FString& Error) const
{
    OutProjectId = FOGEntityId();
    OutScheduleId = FOGEntityId();
    Error.Reset();
    auto* Library = UOGDefinitionLibrary::Get();
    if (!Library || Library->Resolve(DefinitionSystem(), DefinitionId) != this || !ValidateDefinition(Error))
    {
        if (Error.IsEmpty()) Error = TEXT("This authored project is no longer installed.");
        return false;
    }
    if (!RulerId.IsValid() || !TerritoryId.IsValid() || WorldTick < 0 || DurationWorldTicks > MAX_int64 - WorldTick)
    {
        Error = TEXT("The selected holding or project timing is unavailable.");
        return false;
    }
    if (!Store.BeginTransaction(Error))
        return false;
    const auto Rollback = [&Store]()
    {
        FString Ignored;
        Store.RollbackTransaction(Ignored);
        return false;
    };

    FOGTerritoryRecord Holding;
    bool bFound = false;
    if (!Store.TryReadTerritory(TerritoryId, bFound, Holding, Error))
        return Rollback();
    if (!bFound || Holding.RootLocationId != LocationId)
    {
        Error = TEXT("The authored project belongs to a different holding.");
        return Rollback();
    }

    FOGFoundationStrategicContext Context;
    Context.OwnerId = RulerId;
    Context.TerritoryId = TerritoryId;
    Context.LocationId = LocationId;
    FOGFoundationStrategicCommand Command;
    Command.Action = EOGFoundationStrategicAction::StartProject;
    Command.DefinitionId = DefinitionId;
    Command.WorldTick = WorldTick;
    Command.DueWorldTick = WorldTick + DurationWorldTicks;
    Command.ResourceCosts = ResourceCosts;
    Command.PayloadJson = SaveProjectLabel(DisplayName, &OriginalSourceReference);
    FOGFoundationStrategicCommandResult Result;
    if (!Runtime.Execute(Context, Command, Result, Error))
        return Rollback();

    FOGProjectService Projects(Store);
    for (int32 Index = 0; Index < Phases.Num(); ++Index)
    {
        const auto& Authored = Phases[Index];
        FOGProjectPhaseRecord Phase;
        Phase.ProjectId = Result.EntityId;
        Phase.PhaseId = Authored.PhaseId;
        Phase.Sequence = Index;
        Phase.StartWorldTick = WorldTick + Authored.StartOffsetWorldTicks;
        Phase.ResolveWorldTick = WorldTick + Authored.ResolveOffsetWorldTicks;
        Phase.PayloadJson = SaveProjectLabel(Authored.DisplayName);
        if (!Projects.SetProjectPhase(Phase, Error))
            return Rollback();
    }
    FOGProjectRecord Project;
    if (!Phases.IsEmpty() && !Projects.RefreshProject(Result.EntityId, WorldTick, Project, Error))
        return Rollback();

    FOGWorldDirectorScheduleRecord Schedule;
    if (DurationWorldTicks > 0 &&
        !Runtime.ScheduleDueAction(Context, Result.EntityId, FName(TEXT("project")),
            WorldTick, Command.DueWorldTick, Schedule, Error))
        return Rollback();
    if (!Store.CommitTransaction(Error))
        return Rollback();

    OutProjectId = Result.EntityId;
    OutScheduleId = Schedule.ScheduleId;
    return true;
}
