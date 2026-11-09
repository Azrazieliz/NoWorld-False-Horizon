#pragma once

#include "CoreMinimal.h"
#include "Content/OGDefinitionLibrary.h"
#include "World/OGItemKnowledgeCharacterRecords.h"
#include "OGInventoryStorageService.generated.h"

class IOGWorldStore;

USTRUCT(BlueprintType)
struct OFFLINEGAME_API FOGStorageItemFootprint
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FOGContentId ItemDefinitionId;
    /** Exact burden of one unit in the dimensions used by this storage type. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<FName,int64> Burden;
};

UCLASS(BlueprintType)
class OFFLINEGAME_API UOGInventoryStorageDefinition : public UOGReusableDefinition
{
    GENERATED_BODY()
public:
    UOGInventoryStorageDefinition() { System=TEXT("inventory_storage"); }
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FString OriginalSourceReference;
    /** -1 denotes an explicitly unlimited dimension. Units are authored by the storage type. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<FName,int64> Capacity;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FOGStorageItemFootprint> ItemFootprints;
    virtual bool ValidateDefinition_Implementation(FString& Error) const override;
    const FOGStorageItemFootprint* FindFootprint(const FOGContentId& Item) const;
};

struct OFFLINEGAME_API FOGInventoryStorageMove
{
    FOGEntityId RulerId,ItemId,OwnerId,From,Destination;
    FString FromName,DestinationName,Reason,ConfirmationToken;
    int64 Amount=1;
    bool bCanMove=false;
    TMap<FName,int64> Before,After,Capacity;
};

class OFFLINEGAME_API FOGInventoryStorageService
{
public:
    explicit FOGInventoryStorageService(IOGWorldStore& InStore):Store(InStore) {}
    bool ReadDestinations(const FOGEntityId& Ruler,const FOGEntityId& Item,
        TArray<FOGInventoryStorageMove>& Out,FString& Error) const;
    bool Review(const FOGEntityId& Ruler,const FOGEntityId& Item,const FOGEntityId& Destination,
        FOGInventoryStorageMove& Out,FString& Error) const;
    bool Move(const FOGInventoryStorageMove& Reviewed,const FOGEntityId& Request,int64 Tick,FString& Error);
private:
    IOGWorldStore& Store;
};
