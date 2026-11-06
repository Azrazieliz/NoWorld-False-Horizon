#include "Narrative/OGNarrativeChronicle.h"

#include "Engine/GameInstance.h"
#include "Narrative/OGNarrativeJson.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"

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
    FOGWorldEventCursor Next;
    return ReadMemoryPage(FOGWorldEventCursor(),Out,Next,Error);
}

bool UOGNarrativeSessionSubsystem::ReadMemoryPage(const FOGWorldEventCursor& Before,
    TArray<FOGNarrativeMemoryEntry>& Out, FOGWorldEventCursor& Next, FString& Error) const
{
    using namespace OGNarrative;
    Out.Reset();
    Next = FOGWorldEventCursor();
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

    constexpr int32 PageSize = 12;
    TArray<FOGWorldEvent> Events;
    if (!Store->ListWorldEventsPage(Ruler->BoundRulerId,NarrativeReceiptType,true,PageSize + 1,
        Before,Events,Next,Error)) return false;

    const int64 Now = Core->GetCanonicalWorldTick();
    for (int32 Index = 0; Index < FMath::Min(Events.Num(),PageSize); ++Index)
    {
        const FOGWorldEvent& Event = Events[Index];
        // Personal decisions belong only to the Ruler who committed the receipt.
        if (Event.PrimaryEntity != Ruler->BoundRulerId || Event.WorldTick > Now) continue;

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
    if (Events.Num() > PageSize)
    {
        Next.EventId = Events[PageSize - 1].EventId;
        Next.WorldTick = Events[PageSize - 1].WorldTick;
    }
    return true;
}
