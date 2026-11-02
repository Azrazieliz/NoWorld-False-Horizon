#pragma once
#include "Narrative/OGNarrativeService.h"

namespace OGNarrative
{
FString Digest(const FString& Text);
void Part(FString& Out, const FString& Value);
FString DefinitionHash(const FOGNarrativeQuestDefinition& Q);
FString DefinitionHash(const FOGNarrativeDialogueDefinition& D);
FString CommandHash(const FOGNarrativeCommand& Command);
}
