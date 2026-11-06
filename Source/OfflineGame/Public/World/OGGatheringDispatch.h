#pragma once

#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "World/OGDispatchFactionWarRecords.h"
#include "OGGatheringDispatch.generated.h"

class IOGWorldStore;
class FOGCanonicalClockRuntime;
struct FOGWorldDirectorScheduleRecord;

UCLASS(BlueprintType)
class OFFLINEGAME_API UOGGatheringDispatchDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGGatheringDispatchDefinition() { System=TEXT("gathering_dispatch"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    /** The existing production holding location and its actual resource balance. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEntityId LocationId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ResourceId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText ResourceName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 Amount=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 DurationTicks=0;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};

struct OFFLINEGAME_API FOGGatheringDispatchQuote
{
    FOGContentId DefinitionId,ResourceId;
    FOGEntityId RulerId,ParticipantId,TerritoryId,LocationId;
    FString Name,ResourceName,Token,Reason;
    int64 Amount=0,Duration=0,Available=0;
    bool bCanStart=false;
};

class OFFLINEGAME_API FOGGatheringDispatch
{
public:
    explicit FOGGatheringDispatch(IOGWorldStore& InStore):Store(InStore) {}
    bool Preview(const UOGGatheringDispatchDefinition& Definition,const FOGEntityId& Ruler,
        const FOGEntityId& Territory,const FOGEntityId& Participant,int64 Tick,FOGGatheringDispatchQuote& Out,FString& Error) const;
    bool Start(const FOGGatheringDispatchQuote& Quote,const FOGEntityId& Request,
        FOGCanonicalClockRuntime& Clock,FString& Error);
    bool ReadOrders(const FOGEntityId& Ruler,TArray<FOGDispatchRecord>& Out,FString& Error) const;
    static bool ReadSummary(const FOGDispatchRecord& Order,FString& Name,FString& Outcome);
    static const UOGGatheringDispatchDefinition* FindDefinition(const FOGContentId& Id);
    static bool AttachClock(IOGWorldStore& Store,FOGCanonicalClockRuntime& Clock,FString& Error);
    static bool IsAssigned(IOGWorldStore& Store,const FOGEntityId& Ruler,const FOGEntityId& Participant,
        bool& Assigned,FString& Error);
private:
    static bool Resolve(IOGWorldStore& Store,const FOGWorldDirectorScheduleRecord& Schedule,int64 Tick,
        bool bOffline,FString& Outcome,FString& Error);
    IOGWorldStore& Store;
};
