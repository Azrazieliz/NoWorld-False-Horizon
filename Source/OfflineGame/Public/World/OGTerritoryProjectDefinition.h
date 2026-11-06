#pragma once

#include "Content/OGDefinitionLibrary.h"
#include "World/OGTerritoryStateRecords.h"
#include "OGTerritoryProjectDefinition.generated.h"

class FOGFoundationStrategicRuntime;
class IOGWorldStore;

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGTerritoryProjectPhaseDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FOGContentId PhaseId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int64 StartOffsetWorldTicks = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int64 ResolveOffsetWorldTicks = 0;
};

/** A source-backed order for an exact canonical location. The shared definition
 * library installs the specification, while WorldStore owns each issued order. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGTerritoryProjectDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()

public:
    UOGTerritoryProjectDefinition() { System = DefinitionSystem(); }
    static FName DefinitionSystem() { return FName(TEXT("territory_project")); }

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString OriginalSourceReference;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FOGEntityId LocationId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int64 DurationWorldTicks = -1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bExplicitNoCost = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGProjectResourceCost> ResourceCosts;

    /** Array order is the saved display sequence. Dates are authored independently. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FOGTerritoryProjectPhaseDefinition> Phases;

    virtual bool ValidateDefinition_Implementation(FString& Error) const override;

    /** Atomically spend, issue, save phases and persist the Director schedule.
     * The caller registers the returned schedule with the loaded canonical clock. */
    bool Issue(IOGWorldStore& Store, FOGFoundationStrategicRuntime& Runtime,
        const FOGEntityId& RulerId, const FOGEntityId& TerritoryId, int64 WorldTick,
        FOGEntityId& OutProjectId, FOGEntityId& OutScheduleId, FString& Error) const;
};
