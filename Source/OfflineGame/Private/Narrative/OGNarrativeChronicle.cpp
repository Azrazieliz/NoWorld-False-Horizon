#include "Narrative/OGNarrativeChronicle.h"

#include "Engine/GameInstance.h"
#include "Narrative/OGNarrativeJson.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "UI/OGUiViewModelService.h"

namespace
{
const TCHAR* GetMemoryAction(const FString& Kind)
{
    const int32 Value = FCString::Atoi(*Kind);
    if (Kind != LexToString(Value)) return nullptr;

    switch (Value)
    {
    case static_cast<int32>(EOGNarrativeCommand::AcceptQuest): return TEXT("Quest accepted");
    case static_cast<int32>(EOGNarrativeCommand::AdvanceQuest): return TEXT("Quest progress recorded");
    case static_cast<int32>(EOGNarrativeCommand::CompleteQuest): return TEXT("Quest completed");
    case static_cast<int32>(EOGNarrativeCommand::StartDialogue): return TEXT("Conversation started");
    case static_cast<int32>(EOGNarrativeCommand::ChooseDialogue): return TEXT("Dialogue choice made");
    default: return nullptr;
    }
}
}

bool UOGNarrativeSessionSubsystem::ReadRecentMemories(
    TArray<FOGNarrativeMemoryEntry>& Out, FString& Error) const
{
    using namespace OGNarrative;
    Out.Reset();
    Error.Reset();
    const UGameInstance* Game = GetGameInstance();
    const UOGGameCoreSubsystem* Core = BoundCore.Get();
    const auto* Ruler = Game ? Game->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (!Core || !Core->IsCoreReady() || !Store || !Store->IsOpen() ||
        Store->IsWriteTransactionActive() || !Ruler || !Ruler->IsBoundToStoredRuler())
    {
        Error = TEXT("Canonical narrative history is unavailable.");
        return false;
    }

    FOGChronicleFilter Filter;
    Filter.EventTypes.Add(NarrativeReceiptType);
    Filter.Limit = 12;
    TArray<FOGChronicleEntryViewModel> Events;
    if (!FOGUiViewModelService(*Store).BuildChronicleFiltered(Ruler->BoundRulerId, Filter, Events, Error))
        return false;

    const int64 Now = Core->GetCanonicalWorldTick();
    for (const FOGChronicleEntryViewModel& Event : Events)
    {
        // Related-entity matches are useful for the general Chronicle. Personal
        // decisions belong only to the Ruler who committed the receipt.
        if (Event.PrimaryEntityId != Ruler->BoundRulerId || Event.WorldTick > Now) continue;

        FObject Receipt;
        FString ParseError, Schema, Kind;
        if (!ReadObject(Event.PayloadJson, Receipt, ParseError) ||
            !Receipt->TryGetStringField(TEXT("schema"), Schema) || Schema != NarrativeReceiptSchema ||
            !Receipt->TryGetStringField(TEXT("kind"), Kind)) continue;

        const TCHAR* Action = GetMemoryAction(Kind);
        if (!Action) continue;

        FOGNarrativeMemoryEntry Entry;
        Entry.WorldTick = Event.WorldTick;
        Entry.Summary = Action;
        FString DisplayName;
        if (Receipt->TryGetStringField(TEXT("display_name"), DisplayName) && !DisplayName.IsEmpty())
            Entry.Summary += FString(TEXT(" / ")) + DisplayName;

        FString ChoiceText;
        if (Kind == LexToString(static_cast<int32>(EOGNarrativeCommand::ChooseDialogue)) &&
            Receipt->TryGetStringField(TEXT("choice_text"), ChoiceText) && !ChoiceText.IsEmpty())
            Entry.Summary += FString(TEXT(" / ")) + ChoiceText;

        Out.Add(MoveTemp(Entry));
    }
    return true;
}
