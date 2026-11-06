#pragma once

#include "World/OGGatheringDispatch.h"

namespace OGDispatchAuthored
{
bool Validate(const UOGGatheringDispatchDefinition& Definition,FString& Error);
bool Prepare(IOGWorldStore& Store,const UOGGatheringDispatchDefinition& Definition,
    FOGGatheringDispatchQuote& Quote,FString& Error);
bool Evaluate(IOGWorldStore& Store,const FOGDispatchRecord& Order,const FOGDispatchResolutionContext& Policy,
    const FOGEntityId& ReceiptId,int64 Tick,bool bOffline,bool bHoldingAccess,bool bParticipantAvailable,
    FOGDispatchResolutionResult& Result,FString& Error);
}
