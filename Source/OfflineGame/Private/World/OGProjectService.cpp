#include "World/OGProjectService.h"

bool FOGProjectService::StartProject(
    const FOGEntityId& OwnerEntityId,
    const FOGEntityId& LocationId,
    const FOGContentId& ProjectTypeId,
    const TArray<FOGProjectResourceCost>& Costs,
    int64 StartWorldTick,
    int64 ResolveWorldTick,
    const FString& PayloadJson,
    FOGEntityId& OutProjectId,
    FString& OutError)
{
    OutProjectId = FOGEntityId();
    OutError.Reset();

    if (!OwnerEntityId.IsValid() ||
        !LocationId.IsValid() ||
        !ProjectTypeId.IsValid() ||
        ResolveWorldTick < StartWorldTick)
    {
        OutError =
            TEXT("Project definition is invalid.");
        return false;
    }

    if (!Store.BeginTransaction(OutError))
    {
        return false;
    }

    for (const FOGProjectResourceCost& Cost : Costs)
    {
        if (!Cost.ResourceId.IsValid() ||
            Cost.Amount < 0)
        {
            OutError =
                TEXT("Project contains an invalid resource cost.");
            FString RollbackError;
            Store.RollbackTransaction(
                RollbackError);
            return false;
        }

        bool bKnown = false;
        int64 Balance = 0;

        if (!Store.TryReadResourceBalance(
                OwnerEntityId,
                Cost.ResourceId,
                bKnown,
                Balance,
                OutError))
        {
            FString RollbackError;
            Store.RollbackTransaction(
                RollbackError);
            return false;
        }

        if (!bKnown ||
            Balance < Cost.Amount)
        {
            OutError = FString::Printf(
                TEXT("Insufficient resource: %s"),
                *Cost.ResourceId.ToString());
            FString RollbackError;
            Store.RollbackTransaction(
                RollbackError);
            return false;
        }

        if (!Store.SetResourceBalance(
                OwnerEntityId,
                Cost.ResourceId,
                Balance - Cost.Amount,
                OutError))
        {
            FString RollbackError;
            Store.RollbackTransaction(
                RollbackError);
            return false;
        }
    }

    FOGProjectRecord Project;
    Project.ProjectId =
        FOGEntityId::NewId();
    Project.OwnerEntityId =
        OwnerEntityId;
    Project.LocationId =
        LocationId;
    Project.ProjectTypeId =
        ProjectTypeId;
    Project.Status =
        EOGProjectStatus::Active;
    Project.StartWorldTick =
        StartWorldTick;
    Project.ResolveWorldTick =
        ResolveWorldTick;
    Project.ProgressBps =
        ResolveWorldTick ==
            StartWorldTick
            ? 10000
            : 0;
    Project.PayloadJson =
        PayloadJson.IsEmpty()
            ? TEXT("{}")
            : PayloadJson;

    if (Project.ProgressBps == 10000)
    {
        Project.Status =
            EOGProjectStatus::Completed;
    }

    if (!Store.UpsertProject(
            Project,
            StartWorldTick,
            OutError))
    {
        FString RollbackError;
        Store.RollbackTransaction(
            RollbackError);
        return false;
    }

    if (!Store.CommitTransaction(
            OutError))
    {
        FString RollbackError;
        Store.RollbackTransaction(
            RollbackError);
        return false;
    }

    OutProjectId =
        Project.ProjectId;
    return true;
}

bool FOGProjectService::RefreshProject(
    const FOGEntityId& ProjectId,
    int64 CurrentWorldTick,
    FOGProjectRecord& OutProject,
    FString& OutError)
{
    OutProject = FOGProjectRecord();
    OutError.Reset();

    if (CurrentWorldTick < 0)
    {
        OutError = TEXT("Project refresh requires a non-negative world tick.");
        return false;
    }

    bool bFound = false;
    FOGProjectRecord Project;
    if (!Store.TryReadProject(ProjectId, bFound, Project, OutError))
    {
        return false;
    }

    if (!bFound)
    {
        OutError = TEXT("Project does not exist.");
        return false;
    }

    const EOGProjectStatus PreviousStatus = Project.Status;
    const int32 PreviousProgress = Project.ProgressBps;
    if (PreviousStatus == EOGProjectStatus::Active)
    {
        const int64 Duration = Project.ResolveWorldTick - Project.StartWorldTick;
        if (Duration <= 0 || CurrentWorldTick >= Project.ResolveWorldTick)
        {
            Project.ProgressBps = 10000;
            Project.Status = EOGProjectStatus::Completed;
        }
        else if (CurrentWorldTick <= Project.StartWorldTick)
        {
            Project.ProgressBps = 0;
        }
        else
        {
            const int64 Elapsed = CurrentWorldTick - Project.StartWorldTick;
            const double Fraction = static_cast<double>(Elapsed) / static_cast<double>(Duration);
            Project.ProgressBps = FMath::Clamp(FMath::FloorToInt(Fraction * 10000.0), 0, 10000);
        }
    }
    else if (PreviousStatus != EOGProjectStatus::Completed)
    {
        OutProject = MoveTemp(Project);
        return true;
    }

    // Both the live strategic refresh and the due-action resolver enter here.
    // Completed projects can have pending phases in saves from before this binding.
    if (!Store.BeginTransaction(OutError))
    {
        return false;
    }

    TArray<FOGProjectPhaseRecord> Phases;
    const bool bProjectChanged = Project.Status != PreviousStatus || Project.ProgressBps != PreviousProgress;
    if (!RefreshProjectPhases(ProjectId, CurrentWorldTick, Phases, OutError) ||
        (bProjectChanged && !Store.UpsertProject(Project, Project.StartWorldTick, OutError)) ||
        !Store.CommitTransaction(OutError))
    {
        FString RollbackError;
        Store.RollbackTransaction(RollbackError);
        return false;
    }

    OutProject = MoveTemp(Project);
    return true;
}

bool FOGProjectService::SetProjectPhase(
    const FOGProjectPhaseRecord& Phase,
    FString& OutError)
{
    bool bFound = false;
    FOGProjectRecord Project;
    if (!Store.TryReadProject(
            Phase.ProjectId,
            bFound,
            Project,
            OutError))
    {
        return false;
    }

    if (!bFound)
    {
        OutError =
            TEXT("Project phase requires an existing Project.");
        return false;
    }

    return Store.UpsertProjectPhase(
        Phase,
        OutError);
}

bool FOGProjectService::AssignProjectRole(
    const FOGProjectAssignmentRecord& Assignment,
    FString& OutError)
{
    bool bFound = false;
    FOGProjectRecord Project;
    if (!Store.TryReadProject(
            Assignment.ProjectId,
            bFound,
            Project,
            OutError))
    {
        return false;
    }

    if (!bFound)
    {
        OutError =
            TEXT("Project assignment requires an existing Project.");
        return false;
    }

    return Store.UpsertProjectAssignment(
        Assignment,
        OutError);
}

bool FOGProjectService::RefreshProjectPhases(
    const FOGEntityId& ProjectId,
    int64 CurrentWorldTick,
    TArray<FOGProjectPhaseRecord>& OutPhases,
    FString& OutError)
{
    OutPhases.Reset();
    OutError.Reset();

    if (!ProjectId.IsValid() ||
        CurrentWorldTick < 0)
    {
        OutError =
            TEXT("Project phase refresh request is invalid.");
        return false;
    }

    if (!Store.ListProjectPhases(
            ProjectId,
            OutPhases,
            OutError))
    {
        return false;
    }

    for (FOGProjectPhaseRecord& Phase :
         OutPhases)
    {
        if (Phase.Status !=
                FName(TEXT("active")) &&
            Phase.Status !=
                FName(TEXT("planned")))
        {
            continue;
        }

        const FName PreviousStatus = Phase.Status;
        const int32 PreviousProgress = Phase.ProgressBps;
        if (CurrentWorldTick <
            Phase.StartWorldTick)
        {
            Phase.ProgressBps = 0;
            Phase.Status =
                FName(TEXT("planned"));
        }
        else if (CurrentWorldTick >=
                 Phase.ResolveWorldTick)
        {
            Phase.ProgressBps = 10000;
            Phase.Status =
                FName(TEXT("completed"));
        }
        else
        {
            const int64 Duration =
                Phase.ResolveWorldTick -
                Phase.StartWorldTick;

            if (Duration <= 0)
            {
                Phase.ProgressBps = 10000;
                Phase.Status =
                    FName(TEXT("completed"));
            }
            else
            {
                const int64 Elapsed =
                    CurrentWorldTick -
                    Phase.StartWorldTick;
                const double Fraction =
                    static_cast<double>(Elapsed) /
                    static_cast<double>(Duration);
                Phase.ProgressBps =
                    FMath::Clamp(
                        FMath::FloorToInt(
                            Fraction * 10000.0),
                        0,
                        10000);
                Phase.Status =
                    FName(TEXT("active"));
            }
        }

        if ((Phase.Status != PreviousStatus || Phase.ProgressBps != PreviousProgress) &&
            !Store.UpsertProjectPhase(Phase, OutError))
        {
            return false;
        }
    }

    return true;
}
