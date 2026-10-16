#include "UI/OGGate12CharacterVisualCatalog.h"

const FOGGate12CharacterVisualEntry* UOGGate12CharacterVisualCatalog::FindExact(
    const FOGContentId& IdentityId, const FOGContentId& VersionId) const
{
    if (!IdentityId.IsValid() || !VersionId.IsValid()) return nullptr;

    const FOGGate12CharacterVisualEntry* Result = nullptr;
    for (const FOGGate12CharacterVisualEntry& Candidate : Entries)
    {
        if (Candidate.IdentityId == IdentityId &&
            Candidate.VersionId == VersionId)
        {
            // Ambiguous assets must never select an arbitrary character.
            if (Result) return nullptr;
            Result = &Candidate;
        }
    }
    return Result;
}
