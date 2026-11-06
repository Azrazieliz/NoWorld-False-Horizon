#pragma once

#include "Narrative/OGNarrativeService.h"

namespace OGNarrative
{
bool IsNarrativeDiscoveryId(const FOGContentId& Id);
bool ReadDiscoveryFact(IOGWorldStore& Store, const FOGEntityId& Ruler, const FOGContentId& Id,
    bool& bFound, FOGKnowledgeFactRecord& Out, FString& Error);
bool ValidateDiscoveries(const TArray<FOGNarrativeDiscovery>& Discoveries,
    const FString& At, FString& Error);

/** Called inside the same WorldStore transaction as the source receipt. */
bool RecordDiscoveries(IOGWorldStore& Store, const TArray<FOGNarrativeDiscovery>& Discoveries,
    const FOGEntityId& Ruler, const FOGEntityId& Source, const FOGEntityId& Receipt, int64 Tick, FString& Error);
}
