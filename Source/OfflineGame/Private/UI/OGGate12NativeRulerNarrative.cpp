#include "UI/OGGate12NativeRulerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Narrative/OGNarrativeSessionSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "UI/OGGate12GothicChromeWidget.h"

namespace
{
const FLinearColor JournalSilver(.82f, .79f, .89f, 1.f);
const FLinearColor JournalMuted(.57f, .56f, .63f, 1.f);
const FLinearColor JournalGold(.76f, .65f, .46f, 1.f);
const FLinearColor JournalWarning(.88f, .58f, .43f, 1.f);
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
    if (!Narrative->ReadJournal(Quests, Error)) { AddMessage(Body, Error, 18, JournalWarning); return; }
    if (OpenNarrativeQuestId.IsValid())
    {
        AddAction(Body, TEXT("BACK TO JOURNAL"), TEXT("narrative/list"));
        const FOGNarrativeJournalEntry* Quest = Quests.FindByPredicate(
            [this](const FOGNarrativeJournalEntry& Q) { return Q.Definition.QuestId == OpenNarrativeQuestId; });
        if (!Quest) { AddMessage(Body, TEXT("This quest is no longer available in your current history."), 18, JournalMuted); return; }
        AddMessage(Body, Quest->Definition.DisplayName, 26, JournalGold);
        AddMessage(Body, Quest->Definition.Description, 20, JournalSilver);
        AddMessage(Body, Quest->State.bCompleted ? TEXT("COMPLETED") : Quest->State.bExists ? TEXT("IN PROGRESS") : TEXT("AVAILABLE"), 17, JournalMuted);
        for (const FOGNarrativeObjective& Objective : Quest->Definition.Objectives)
        {
            const TArray<FOGEntityId>* Credited = Quest->State.CreditedEvents.Find(Objective.ObjectiveId);
            AddMessage(Body, FString::Printf(TEXT("%s   %d / %d"), *Objective.DisplayName,
                Credited ? FMath::Min(Credited->Num(), Objective.RequiredCount) : 0, Objective.RequiredCount), 19, JournalSilver);
        }
        if (!Quest->State.bExists)
            AddAction(Body, TEXT("REVIEW QUEST"), TEXT("narrative/accept"));
        else if (!Quest->State.bCompleted)
        {
            AddAction(Body, TEXT("REVIEW RECENT PROGRESS"), TEXT("narrative/advance"));
            AddAction(Body, TEXT("REVIEW COMPLETION"), TEXT("narrative/complete"));
        }
        return;
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
        AddAction(Body, Quest.Definition.DisplayName + (Quest.State.bCompleted ? TEXT("  ·  COMPLETED") : TEXT("")),
            TEXT("narrative/quest/") + Quest.Definition.QuestId.ToString());
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
        { OpenNarrativeQuestId = FOGContentId(); OpenNarrativeDialogueId = FOGContentId(); }
        else if (Action.StartsWith(TEXT("narrative/quest/")))
        { OpenNarrativeQuestId = FOGContentId(*Action.Mid(16)); OpenNarrativeDialogueId = FOGContentId(); }
        else if (Action.StartsWith(TEXT("narrative/dialogue/")))
        { OpenNarrativeDialogueId = FOGContentId(*Action.Mid(19)); OpenNarrativeQuestId = FOGContentId(); }
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
