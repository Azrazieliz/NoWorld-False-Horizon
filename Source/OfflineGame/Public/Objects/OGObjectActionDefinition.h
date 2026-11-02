#pragma once
#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "World/OGItemKnowledgeCharacterRecords.h"
#include "OGObjectActionDefinition.generated.h"

UENUM(BlueprintType)
enum class EOGObjectActionKind : uint8 { Use, Craft, Repair, Exchange };
UENUM(BlueprintType)
enum class EOGObjectActionParty : uint8 { Ruler, Counterparty };
UENUM(BlueprintType)
enum class EOGObjectInputDisposition : uint8 { Retain, Consume, Transfer };

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectActionInput
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ObjectId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId RankId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId QualityId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 InstanceCount=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGObjectActionParty From=EOGObjectActionParty::Ruler;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGObjectInputDisposition Disposition=EOGObjectInputDisposition::Consume;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectActionOutput
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ObjectId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 InstanceCount=1;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGObjectActionParty To=EOGObjectActionParty::Ruler;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectActionResourceChange
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ResourceId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGObjectActionParty Account=EOGObjectActionParty::Ruler;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int64 Amount=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
};
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGObjectDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGObjectDefinition() { System=TEXT("object"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId InitialRankId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId InitialQualityId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(MultiLine=true)) FString InitialDurabilityJson=TEXT("{}");
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(MultiLine=true)) FString InitialEvolutionJson=TEXT("{}");
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
};
/** Shared executable recipe. Instances, wallets, stock and repair condition stay canonical in WorldStore. */
UCLASS(BlueprintType)
class OFFLINEGAME_API UOGObjectActionDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGObjectActionDefinition() { System=TEXT("object_action"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EOGObjectActionKind Kind=EOGObjectActionKind::Use;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(MultiLine=true)) FText Description;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bExplicitNoCost=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGObjectActionInput> Inputs;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGObjectActionOutput> Outputs;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGObjectActionResourceChange> Resources;
    /** Required for repair. The selected owned instance is preserved, including its modifiers/history. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId SubjectObjectId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 RepairToConditionBps=0;
    /** Any counterparty access requires a real, explicitly published canonical offer. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName CounterpartyEntityKind;
    bool UsesCounterparty() const;
    FString Fingerprint() const;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    virtual void CollectRequiredDefinitions(TArray<FOGDefinitionReference>& Out) const override;
    virtual bool ValidateResolvedReferences(const FOGDefinitionResolver& Resolve,FString& Error) const override;
};

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectActionContext
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FOGEntityId RulerId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FOGEntityId CounterpartyId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FOGEntityId SelectedItemId;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectActionSelectedInput
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGItemInstanceRecord Item;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) EOGObjectInputDisposition Disposition=EOGObjectInputDisposition::Consume;
    UPROPERTY(BlueprintReadOnly) FOGEntityId NewOwnerId;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectActionWalletPreview
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGEntityId OwnerId;
    UPROPERTY(BlueprintReadOnly) FOGContentId ResourceId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) int64 Before=0;
    UPROPERTY(BlueprintReadOnly) int64 After=0;
};
USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGObjectActionPreview
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FOGContentId ActionId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) FText Description;
    UPROPERTY(BlueprintReadOnly) FOGObjectActionContext Context;
    UPROPERTY(BlueprintReadOnly) TArray<FOGObjectActionSelectedInput> Inputs;
    UPROPERTY(BlueprintReadOnly) TArray<FOGObjectActionOutput> Outputs;
    UPROPERTY(BlueprintReadOnly) TArray<FOGObjectActionWalletPreview> Wallets;
    UPROPERTY(BlueprintReadOnly) bool bRepairsSubject=false;
    UPROPERTY(BlueprintReadOnly) int32 RepairBeforeConditionBps=0;
    UPROPERTY(BlueprintReadOnly) int32 RepairAfterConditionBps=0;
    UPROPERTY(BlueprintReadOnly) bool bSubjectWasBroken=false;
    UPROPERTY(BlueprintReadOnly) FOGItemInstanceRecord SubjectBefore;
    UPROPERTY(BlueprintReadOnly) FString SubjectDurabilityAfter;
    UPROPERTY(BlueprintReadOnly) bool bCanApply=false;
    UPROPERTY(BlueprintReadOnly) TArray<FString> MissingRequirements;
    UPROPERTY(BlueprintReadOnly) FString ConfirmationToken;
    /** Internal canonical offer snapshot, included in the confirmation fingerprint. */
    FOGEntityId OfferId;
    FString OfferStateJson;
    int64 OfferRemaining=0;
};
