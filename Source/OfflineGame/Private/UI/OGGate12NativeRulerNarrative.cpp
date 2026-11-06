#include "UI/OGGate12NativeRulerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "UI/OGGate12GothicChromeWidget.h"
#include "UI/OGUiViewModelService.h"

namespace
{
const FLinearColor JournalSilver(.82f, .79f, .89f, 1.f);
const FLinearColor JournalMuted(.57f, .56f, .63f, 1.f);
const FLinearColor JournalGold(.76f, .65f, .46f, 1.f);
const FLinearColor JournalWarning(.88f, .58f, .43f, 1.f);
const TCHAR* AllCodexCategories = TEXT("All categories");
}

void UOGGate12NativeRulerWidget::ClearNarrativeReview()
{
    PendingNarrativePreview = FOGNarrativePreview();
    PendingNarrativeRequestId = FOGEntityId();
    if (UOGNarrativeSessionSubsystem* Narrative = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr) Narrative->ClearReview();
}

void UOGGate12NativeRulerWidget::ResetNarrativePage()
{
    ClearNarrativeReview();
    if (UOGNarrativeSessionSubsystem* Narrative = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr) Narrative->CloseSubject();
    bNarrativeJournalOpen = false;
    bNarrativeCodexOpen = false;
    NarrativeCodexSearch.Reset();
    NarrativeCodexCategory = NAME_None;
    NarrativeCodexRows.Reset();
    NarrativeMemoryBefore = FOGWorldEventCursor();
    NarrativeMemoryNext = FOGWorldEventCursor();
    OpenNarrativeQuestId = FOGContentId();
    OpenNarrativeDialogueId = FOGContentId();
    NarrativePageRulerId = FOGEntityId();
    NarrativeFeedback.Reset();
}

void UOGGate12NativeRulerWidget::BuildNarrativeJournal()
{
    if (!Content || !WidgetTree) return;
    UOGNarrativeSessionSubsystem* Narrative = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    AddAction(Content, TEXT("BACK TO RECORDS"), TEXT("narrative/close"));
    if (!Narrative) { AddMessage(Content, TEXT("The journal is unavailable."), 18, JournalMuted); return; }
    if (const UOGGate12RulerSessionSubsystem* Ruler = GetGameInstance()->GetSubsystem<UOGGate12RulerSessionSubsystem>())
        NarrativePageRulerId = Ruler->BoundRulerId;
    UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
    UBorder* Surface = WidgetTree->ConstructWidget<UBorder>();
    Surface->SetBrushColor(FLinearColor(.010f, .008f, .018f, .97f));
    Surface->SetPadding(FMargin(16.f, 22.f));
    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>();
    Surface->AddChild(Body);
    UOverlaySlot* SurfaceSlot = Layers->AddChildToOverlay(Surface);
    SurfaceSlot->SetHorizontalAlignment(HAlign_Fill);
    SurfaceSlot->SetVerticalAlignment(VAlign_Fill);
    UOGGate12GothicChromeWidget* Chrome = WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>();
    Chrome->Configure(JournalSilver, INDEX_NONE, false);
    Chrome->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* ChromeSlot = Layers->AddChildToOverlay(Chrome);
    ChromeSlot->SetHorizontalAlignment(HAlign_Fill);
    ChromeSlot->SetVerticalAlignment(VAlign_Fill);
    Content->AddChildToVerticalBox(Layers)->SetPadding(FMargin(4.f, 8.f));
    if (bNarrativeCodexOpen)
    {
        AddAction(Body, TEXT("BACK TO JOURNAL"), TEXT("narrative/list"));
        AddMessage(Body, TEXT("CODEX"), 30, JournalSilver);
        BuildNarrativeCodex(Body);
        return;
    }
    AddMessage(Body, TEXT("JOURNAL"), 30, JournalSilver);
    if (!NarrativeFeedback.IsEmpty()) AddMessage(Body, NarrativeFeedback, 18, JournalGold);

    if (!PendingNarrativePreview.ConfirmationToken.IsEmpty())
    {
        AddMessage(Body, PendingNarrativePreview.DisplayName, 24, JournalGold);
        for (const FOGNarrativeResourcePreview& Resource : PendingNarrativePreview.Resources)
            AddMessage(Body, FString::Printf(TEXT("%s  /  owned %s  /  cost %s  /  reward %s  /  after %s"),
                *Resource.DisplayName, *FText::AsNumber(Resource.BalanceBefore).ToString(),
                *FText::AsNumber(Resource.Cost).ToString(), *FText::AsNumber(Resource.Reward).ToString(),
                *FText::AsNumber(Resource.BalanceAfter).ToString()), 17, JournalSilver);
        for (const FString& Missing : PendingNarrativePreview.MissingRequirements)
            AddMessage(Body, Missing, 17, JournalWarning);
        AddMessage(Body, TEXT("Your decision and its consequences will be saved."), 16, JournalMuted);
        if (PendingNarrativePreview.bCanApply)
            AddAction(Body, TEXT("CONFIRM CHOICE"), TEXT("narrative/confirm"));
        AddAction(Body, TEXT("CANCEL"), TEXT("narrative/cancel"));
        return;
    }

    FString Error;
    if (OpenNarrativeDialogueId.IsValid())
    {
        AddAction(Body, TEXT("BACK TO JOURNAL"), TEXT("narrative/list"));
        FOGNarrativeConversationView Conversation;
        if (!Narrative->ReadConversation(OpenNarrativeDialogueId, Conversation, Error))
        {
            AddMessage(Body, Error.IsEmpty() ? TEXT("This conversation is currently unavailable.") : Error, 18, JournalMuted);
            return;
        }
        if (!Conversation.State.bExists)
        {
            FOGNarrativePreview Start;
            if (Narrative->ReviewDialogue(OpenNarrativeDialogueId, FOGContentId(), Start, Error) && Start.bCanApply)
            {
                AddMessage(Body, Start.DisplayName, 24, JournalGold);
                AddAction(Body, TEXT("BEGIN CONVERSATION"), TEXT("narrative/start"));
            }
            else AddMessage(Body, Error.IsEmpty() ? TEXT("This conversation is currently unavailable.") : Error, 18, JournalMuted);
            return;
        }
        AddMessage(Body, Conversation.DisplayName, 24, JournalGold);
        AddMessage(Body, Conversation.Text, 21, JournalSilver);
        for (const FOGNarrativeChoiceView& Choice : Conversation.Choices)
        {
            AddAction(Body, Choice.Text, TEXT("narrative/choice/") + Choice.ChoiceId.ToString(), Choice.bEligible);
            if (!Choice.bEligible)
                for (const FString& Requirement : Choice.MissingRequirements) AddMessage(Body, Requirement, 15, JournalMuted);
        }
        if (Conversation.State.bCompleted) AddMessage(Body, TEXT("Conversation complete."), 16, JournalMuted);
        return;
    }

    TArray<FOGNarrativeJournalEntry> Quests;
    const bool bJournalAvailable = Narrative->ReadJournal(Quests, Error);
    if (!bJournalAvailable)
        AddMessage(Body, Error.IsEmpty() ? TEXT("The quest journal is unavailable.") : Error, 18, JournalWarning);
    if (OpenNarrativeQuestId.IsValid())
    {
        AddAction(Body, TEXT("BACK TO JOURNAL"), TEXT("narrative/list"));
        if (!bJournalAvailable) return;
        const FOGNarrativeJournalEntry* Quest = Quests.FindByPredicate(
            [this](const FOGNarrativeJournalEntry& Q) { return Q.Definition.QuestId == OpenNarrativeQuestId; });
        if (!Quest) { AddMessage(Body, TEXT("This quest is no longer available in your current history."), 18, JournalMuted); return; }
        AddMessage(Body, Quest->Definition.DisplayName, 26, JournalGold);
        AddMessage(Body, Quest->Definition.Description, 20, JournalSilver);
        const TCHAR* Status = Quest->State.bCompleted ? TEXT("COMPLETED") :
            Quest->bExpired ? TEXT("CLOSED") : Quest->State.bExists ? TEXT("IN PROGRESS") : TEXT("AVAILABLE");
        AddMessage(Body, Status, 17, JournalMuted);
        for (const FOGNarrativeObjective& Objective : Quest->Definition.Objectives)
        {
            const TArray<FOGEntityId>* Credited = Quest->State.CreditedEvents.Find(Objective.ObjectiveId);
            AddMessage(Body, FString::Printf(TEXT("%s   %d / %d"), *Objective.DisplayName,
                Credited ? FMath::Min(Credited->Num(), Objective.RequiredCount) : 0, Objective.RequiredCount), 19, JournalSilver);
        }
        if (!Quest->State.bExists)
            AddAction(Body, TEXT("REVIEW QUEST"), TEXT("narrative/accept"));
        else if (!Quest->State.bCompleted && !Quest->bExpired)
        {
            AddAction(Body, TEXT("REVIEW RECENT PROGRESS"), TEXT("narrative/advance"));
            AddAction(Body, TEXT("REVIEW COMPLETION"), TEXT("narrative/complete"));
            const FString TrackingAction = Quest->bTracked ? TEXT("narrative/untrack/") : TEXT("narrative/track/");
            AddAction(Body, Quest->bTracked ? TEXT("STOP TRACKING") : TEXT("TRACK QUEST"),
                TrackingAction + NarrativePageRulerId.ToString() + TEXT("|") + Quest->Definition.QuestId.ToString());
            if (Quest->bTracked)
            {
                AddMessage(Body, TEXT("This quest is tracked in your journal."), 16, JournalGold);
                if (Quest->bTrackedSubjectHere)
                    AddMessage(Body, TEXT("You are speaking with this quest's contact."), 16, JournalSilver);
                else if (Quest->State.SubjectId.IsValid() && !Quest->Definition.bAllowRemoteCompletion)
                    AddMessage(Body, TEXT("Return to this quest's contact to review completion."), 16, JournalMuted);
                AddAction(Body, TEXT("RETURN TO WORLD"), TEXT("world"));
            }
        }
        return;
    }

    AddAction(Body, TEXT("OPEN CODEX"), TEXT("narrative/codex"));
    if (bJournalAvailable)
    {
        if (const auto* Tracked = Quests.FindByPredicate(
                [](const FOGNarrativeJournalEntry& Entry) { return Entry.bTracked; }))
        {
            AddMessage(Body, TEXT("TRACKED QUEST"), 19, JournalGold);
            AddAction(Body, Tracked->Definition.DisplayName,
                TEXT("narrative/quest/") + Tracked->Definition.QuestId.ToString());
            if (Tracked->bTrackedSubjectHere)
                AddMessage(Body, TEXT("You are speaking with this quest's contact."), 16, JournalSilver);
            for (const FOGNarrativeObjective& Objective : Tracked->Definition.Objectives)
            {
                const auto* Credits = Tracked->State.CreditedEvents.Find(Objective.ObjectiveId);
                const int32 Recorded = Credits ? Credits->Num() : 0;
                if (Recorded >= Objective.RequiredCount) continue;
                AddMessage(Body, FString::Printf(TEXT("%s   %d / %d"),
                    *Objective.DisplayName, Recorded, Objective.RequiredCount), 16, JournalSilver);
                break;
            }
        }
        if (Narrative->HasOpenSubject())
        {
            TArray<FOGNarrativeDialogueDefinition> Dialogues;
            if (!Narrative->ReadSubjectDialogues(Dialogues, Error)) AddMessage(Body, Error, 18, JournalWarning);
            for (const FOGNarrativeDialogueDefinition& Dialogue : Dialogues)
                AddAction(Body, Dialogue.DisplayName, TEXT("narrative/dialogue/") + Dialogue.DialogueId.ToString());
        }
        if (Quests.IsEmpty()) AddMessage(Body, TEXT("No quests are available in your current history."), 19, JournalMuted);
        for (const FOGNarrativeJournalEntry& Quest : Quests)
        {
            if (Quest.bTracked) continue;
            AddAction(Body, Quest.Definition.DisplayName +
                (Quest.State.bCompleted ? TEXT("  ·  COMPLETED") : Quest.bExpired ? TEXT("  ·  CLOSED") : TEXT("")),
                TEXT("narrative/quest/") + Quest.Definition.QuestId.ToString());
        }
    }

    AddMessage(Body, TEXT("STORY HISTORY"), 22, JournalGold);
    TArray<FOGNarrativeMemoryEntry> Memories;
    if (!Narrative->ReadMemoryPage(NarrativeMemoryBefore, Memories, NarrativeMemoryNext, Error))
        AddMessage(Body, Error, 18, JournalWarning);
    else if (Memories.IsEmpty())
        AddMessage(Body, TEXT("No story decisions have been recorded yet."), 18, JournalMuted);
    for (const FOGNarrativeMemoryEntry& Memory : Memories)
        AddMessage(Body, FString::Printf(TEXT("%s  /  world tick %lld"), *Memory.Summary, Memory.WorldTick),
            18, JournalSilver);
    if (NarrativeMemoryBefore.EventId.IsValid())
        AddAction(Body, TEXT("LATEST HISTORY"), TEXT("narrative/history/latest"));
    if (NarrativeMemoryNext.EventId.IsValid())
        AddAction(Body, TEXT("OLDER HISTORY"), TEXT("narrative/history/older"));
}

void UOGGate12NativeRulerWidget::BuildNarrativeCodex(UVerticalBox* Parent)
{
    if (!Parent || !WidgetTree) return;
    UEditableTextBox* Search = WidgetTree->ConstructWidget<UEditableTextBox>();
    Search->SetHintText(FText::FromString(TEXT("Search discovered entries")));
    Search->SetText(FText::FromString(NarrativeCodexSearch));
    Search->OnTextChanged.AddDynamic(this,&ThisClass::HandleNarrativeCodexSearch);
    Parent->AddChildToVerticalBox(Search);

    UComboBoxString* Categories = WidgetTree->ConstructWidget<UComboBoxString>();
    Categories->AddOption(AllCodexCategories);
    for (FName Category : FOGUiViewModelService::BuildCodex({},FString(),NAME_None).Categories)
        Categories->AddOption(Category.ToString().Replace(TEXT("_"),TEXT(" ")));
    Categories->SetSelectedOption(NarrativeCodexCategory.IsNone() ? FString(AllCodexCategories) :
        NarrativeCodexCategory.ToString().Replace(TEXT("_"),TEXT(" ")));
    Categories->OnSelectionChanged.AddDynamic(this,&ThisClass::HandleNarrativeCodexCategory);
    Parent->AddChildToVerticalBox(Categories);

    UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>();
    Parent->AddChildToVerticalBox(Rows);
    NarrativeCodexRows = Rows;
    RebuildNarrativeCodexRows();
}

void UOGGate12NativeRulerWidget::HandleNarrativeCodexSearch(const FText& Search)
{
    NarrativeCodexSearch = Search.ToString();
    RebuildNarrativeCodexRows();
}

void UOGGate12NativeRulerWidget::HandleNarrativeCodexCategory(FString Selected, ESelectInfo::Type)
{
    NarrativeCodexCategory = Selected == AllCodexCategories ? NAME_None : FName(*Selected.Replace(TEXT(" "),TEXT("_")));
    RebuildNarrativeCodexRows();
}

void UOGGate12NativeRulerWidget::RebuildNarrativeCodexRows()
{
    UVerticalBox* Parent = NarrativeCodexRows.Get();
    if (!Parent) return;
    Parent->ClearChildren();
    const UGameInstance* Game = GetGameInstance();
    const auto* Narrative = Game ? Game->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    TArray<FOGNarrativeCodexEntry> Entries;
    FString Error;
    if (!Narrative || !Narrative->ReadCodexDiscoveries(NarrativeCodexSearch,NarrativeCodexCategory,Entries,Error))
    {
        AddMessage(Parent, Error.IsEmpty() ? TEXT("Codex knowledge is unavailable.") : Error, 18, JournalWarning);
        return;
    }
    if (Entries.IsEmpty())
    {
        AddMessage(Parent, TEXT("No discovered entries match this view."), 18, JournalMuted);
        return;
    }

    for (const FOGNarrativeCodexEntry& Discovery : Entries)
    {
        const TCHAR* Belief = TEXT("Unknown");
        switch (Discovery.Entry.KnowledgeState)
        {
        case EOGUiKnowledgeState::Confirmed: Belief = TEXT("Confirmed"); break;
        case EOGUiKnowledgeState::Estimated: Belief = TEXT("Estimated"); break;
        case EOGUiKnowledgeState::Rumor: Belief = TEXT("Rumor"); break;
        case EOGUiKnowledgeState::Contradicted: Belief = TEXT("Contradicted"); break;
        case EOGUiKnowledgeState::Outdated: Belief = TEXT("Outdated"); break;
        default: break;
        }
        AddMessage(Parent, Discovery.Entry.DisplayNameKey, 21, JournalSilver);
        AddMessage(Parent, Belief, 16, JournalMuted);
        if (!Discovery.Summary.IsEmpty()) AddMessage(Parent, Discovery.Summary, 18, JournalSilver);
    }
}

void UOGGate12NativeRulerWidget::HandleNarrativeAction(const FString& Action)
{
    UOGNarrativeSessionSubsystem* Narrative = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UOGNarrativeSessionSubsystem>() : nullptr;
    if (!Narrative) return;
    bNarrativeJournalOpen = true;
    NarrativeFeedback.Reset();
    FString Error;
    if (Action == TEXT("narrative/confirm"))
    {
        if (Narrative->ConfirmReview(PendingNarrativePreview.ConfirmationToken, PendingNarrativeRequestId, Error))
            NarrativeFeedback = TEXT("Your choice has been saved.");
        else NarrativeFeedback = Error.IsEmpty() ? TEXT("The choice changed. Review it again.") : Error;
        ClearNarrativeReview();
    }
    else
    {
        ClearNarrativeReview();
        if (Action == TEXT("narrative/close"))
        {
            Narrative->CloseSubject();
            ResetNarrativePage();
            RecordsSection = -1;
        }
        else if (Action == TEXT("narrative/list"))
        {
            bNarrativeCodexOpen = false;
            OpenNarrativeQuestId = FOGContentId();
            OpenNarrativeDialogueId = FOGContentId();
        }
        else if (Action == TEXT("narrative/codex"))
            bNarrativeCodexOpen = true;
        else if (Action == TEXT("narrative/history/latest"))
            NarrativeMemoryBefore = FOGWorldEventCursor();
        else if (Action == TEXT("narrative/history/older") && NarrativeMemoryNext.EventId.IsValid())
            NarrativeMemoryBefore = NarrativeMemoryNext;
        else if (Action.StartsWith(TEXT("narrative/quest/")))
        { OpenNarrativeQuestId = FOGContentId(*Action.Mid(16)); OpenNarrativeDialogueId = FOGContentId(); }
        else if (Action.StartsWith(TEXT("narrative/dialogue/")))
        { OpenNarrativeDialogueId = FOGContentId(*Action.Mid(19)); OpenNarrativeQuestId = FOGContentId(); }
        else if (Action.StartsWith(TEXT("narrative/track/")) || Action.StartsWith(TEXT("narrative/untrack/")))
        {
            const bool bTrack = Action.StartsWith(TEXT("narrative/track/"));
            const FString Prefix = bTrack ? TEXT("narrative/track/") : TEXT("narrative/untrack/");
            FString RulerText, QuestText;
            FGuid RulerGuid;
            if (bNarrativeCodexOpen || OpenNarrativeDialogueId.IsValid() ||
                !Action.Mid(Prefix.Len()).Split(TEXT("|"), &RulerText, &QuestText) ||
                !FGuid::Parse(RulerText, RulerGuid) || !RulerGuid.IsValid() ||
                FOGEntityId(RulerGuid) != NarrativePageRulerId ||
                !OpenNarrativeQuestId.IsValid() || OpenNarrativeQuestId != FOGContentId(QuestText))
            {
                Error = TEXT("The journal selection changed. Open the quest again.");
            }
            else if (Narrative->SetQuestTracking(FOGEntityId(RulerGuid), OpenNarrativeQuestId, bTrack, Error))
                NarrativeFeedback = bTrack ? TEXT("Quest tracked.") : TEXT("Quest tracking cleared.");
        }
        else if (Action == TEXT("narrative/accept") || Action == TEXT("narrative/advance") || Action == TEXT("narrative/complete"))
        {
            const EOGNarrativeCommand Kind = Action == TEXT("narrative/accept") ? EOGNarrativeCommand::AcceptQuest :
                Action == TEXT("narrative/advance") ? EOGNarrativeCommand::AdvanceQuest : EOGNarrativeCommand::CompleteQuest;
            Narrative->ReviewQuest(OpenNarrativeQuestId, Kind, PendingNarrativePreview, Error);
        }
        else if (Action == TEXT("narrative/start") || Action.StartsWith(TEXT("narrative/choice/")))
        {
            const FOGContentId Choice = Action == TEXT("narrative/start") ? FOGContentId() : FOGContentId(*Action.Mid(17));
            Narrative->ReviewDialogue(OpenNarrativeDialogueId, Choice, PendingNarrativePreview, Error);
        }
        if (!PendingNarrativePreview.ConfirmationToken.IsEmpty()) PendingNarrativeRequestId = FOGEntityId::NewId();
        if (!Error.IsEmpty()) NarrativeFeedback = Error;
    }
    BuildCurrentPage();
}
