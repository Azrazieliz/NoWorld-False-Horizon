#pragma once

#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "Core/OGEntityId.h"
#include "World/OGDispatchFactionWarRecords.h"
#include "OGFactionDiplomacyPolicy.generated.h"

class IOGWorldStore;

UENUM(BlueprintType)
enum class EOGFactionProposal : uint8
{
    Alliance, Support, Subordination
};

UENUM(BlueprintType)
enum class EOGFactionNegotiator : uint8
{
    RequestingFaction, RespondingFaction, RequestingLeader, RespondingLeader
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGFactionProposalState
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGFactionNegotiator Subject=EOGFactionNegotiator::RespondingFaction;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Field;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString ExactValue;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGFactionProposalLink
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGFactionLinkType Type=EOGFactionLinkType::Alliance;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bFromRespondingFaction=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bMustBeActive=true;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGFactionProposalRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName RuleId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGFactionProposal Proposal=EOGFactionProposal::Alliance;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEntityId ExactRequester;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGFactionProposalState> State;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGFactionProposalLink> Links;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bAccept=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText Reply;
};

/** One authored source of responses for an actual foreign faction. Order is the original decision priority. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGFactionDiplomacyPolicy : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGFactionDiplomacyPolicy() { System=TEXT("faction_diplomacy"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGEntityId RespondingFactionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGFactionProposalRule> Rules;

    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    bool HasProposal(EOGFactionProposal Proposal) const;
    bool Decide(IOGWorldStore& Store,EOGFactionProposal Proposal,const FOGFactionRecord& Requester,
        const FOGFactionRecord& Responder,const FOGFactionProposalRule*& Out,FString& Error) const;
    FString Fingerprint() const;
    static const UOGFactionDiplomacyPolicy* Find(const FOGEntityId& Responder,EOGFactionProposal Proposal,FString& Error);
};
