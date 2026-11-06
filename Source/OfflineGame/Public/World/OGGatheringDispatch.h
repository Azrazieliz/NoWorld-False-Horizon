#pragma once

#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "World/OGDispatchFactionWarRecords.h"
#include "World/OGStrategyExpansionRecords.h"
#include "World/OGWorldStateRecords.h"
#include "OGGatheringDispatch.generated.h"

class IOGWorldStore;
class FOGCanonicalClockRuntime;
struct FOGWorldDirectorScheduleRecord;

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGAuthoredDispatchObjective
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId ObjectiveId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bMandatory = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRequireAccessibleTarget = false;
    /** Exact event on the target. By default only evidence after launch and through the deadline counts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName TargetEventType;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAllowPriorEvidence = false;
};

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
    /** LocationId remains the issuing holding. Gathering retains its existing stock collection path. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOGDispatchType DispatchType = EOGDispatchType::Gathering;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGEntityId TargetLocationId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Briefing;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText RiskDescription;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RiskBps = -1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RiskToleranceBps = -1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRequireKnownTarget = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOGContentId RequiredParticipantSkillId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RequiredParticipantSkillState;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FOGAuthoredDispatchObjective> Objectives;
    /** Explicit success effects. Support can transfer existing stock from the holding to the target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bTransferHoldingStock = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRevealLocation = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOGLocationKnowledgeLevel RevealedLocationLevel = EOGLocationKnowledgeLevel::Rumored;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName KnowledgeFactKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText KnowledgeSummary;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName KnowledgeBeliefState;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 KnowledgeConfidenceBps = -1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText SuccessSummary;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};

struct OFFLINEGAME_API FOGGatheringDispatchQuote
{
    FOGContentId DefinitionId,ResourceId;
    FOGEntityId RulerId,ParticipantId,TerritoryId,LocationId;
    FString Name,ResourceName,Token,Reason;
    int64 Amount=0,Duration=0,Available=0;
    bool bCanStart=false;
    bool bVisible=true;
    EOGDispatchType Type=EOGDispatchType::Gathering;
    FOGEntityId TargetLocationId;
    FString Briefing,RiskDescription,AuthoredPlanJson;
    TArray<FString> ObjectiveNames;
    TArray<FOGDispatchObjectiveRecord> Objectives;
    TArray<FOGDispatchConstraintRecord> Constraints;
    int32 Risk=0,RiskTolerance=0;
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
