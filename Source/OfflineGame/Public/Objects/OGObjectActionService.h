#pragma once
#include "CoreMinimal.h"
#include "Objects/OGObjectActionDefinition.h"
#include "Persistence/OGWorldStore.h"

/** One canonical transaction for use, crafting, repair and stocked exchanges.
 * No UI-provided prices/grants and no alternate item database or wallet. */
class OFFLINEGAME_API FOGObjectActionService
{
public:
    explicit FOGObjectActionService(IOGWorldStore& InStore):Store(InStore) {}
    bool Preview(const UOGObjectActionDefinition& Definition,const FOGObjectActionContext& Context,
        int64 WorldTick,FOGObjectActionPreview& Out,FString& Error) const;
    bool Apply(const UOGObjectActionDefinition& Definition,const FOGObjectActionPreview& Reviewed,
        const FOGEntityId& RequestId,int64 WorldTick,bool& AlreadyApplied,TArray<FOGEntityId>& CreatedItems,FString& Error);
    /** Native shop/world producer publishes access, never creates stock/currency or an NPC.
     * RemainingUses=-1 is an explicit unlimited offer; zero closes it. */
    bool PublishOffer(const UOGObjectActionDefinition& Definition,const FOGEntityId& Counterparty,
        const FOGEntityId& AllowedRuler,int64 RemainingUses,int64 OpensTick,int64 ClosesTick,
        const FOGEntityId& RequestId,int64 WorldTick,FString& Error);
private:
    IOGWorldStore& Store;
};
