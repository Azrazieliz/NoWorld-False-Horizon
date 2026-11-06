#include "UI/OGGate12NativeRulerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "UI/OGGate12GothicChromeWidget.h"
#include "UI/OGUiViewModelService.h"

namespace
{
const FLinearColor DevelopmentGold(.72f, .62f, .44f, 1.f);
const FLinearColor DevelopmentText(.90f, .90f, .89f, 1.f);
const FLinearColor DevelopmentMuted(.55f, .58f, .61f, 1.f);
const FLinearColor DevelopmentWarning(.88f, .57f, .43f, 1.f);
const FLinearColor DevelopmentSuccess(.60f, .79f, .65f, 1.f);

FString DevelopmentStateName(FName State)
{
    return State.IsNone() ? TEXT("Not recorded") : State.ToString();
}

FString DevelopmentReinforcementName(FName State, bool bComplete)
{
    return DevelopmentStateName(State) + (bComplete ? TEXT(" (complete)") : TEXT(""));
}

FString DevelopmentNumber(int64 Value)
{
    return FText::AsNumber(Value).ToString();
}
}

void UOGGate12NativeRulerWidget::ClearDevelopmentReview()
{
    PendingDevelopmentReview = FOGCharacterDevelopmentPreview();
    PendingDevelopmentRequestId = FOGEntityId();
}

void UOGGate12NativeRulerWidget::ResetCharacterDevelopment()
{
    ClearDevelopmentReview();
    DevelopmentContextRulerId = FOGEntityId();
    DevelopmentContextIdentityId = FOGContentId();
    DevelopmentContextManifestationId = FOGEntityId();
    DevelopmentSessionIdentityId = FOGContentId();
    DevelopmentSessionManifestationId = FOGEntityId();
    DevelopmentFeedback.Reset();
    DevelopmentRefreshWarning.Reset();
    bDevelopmentFeedbackError = false;
}

bool UOGGate12NativeRulerWidget::SynchronizeDevelopmentContext(
    const UOGGate12RulerSessionSubsystem* Session)
{
    FOGEntityId RulerId;
    FOGContentId IdentityId;
    FOGEntityId ManifestationId;
    const FOGContentId SelectedIdentity = Session
        ? Session->SelectedIdentityId : FOGContentId();
    const FOGEntityId SelectedManifestation = Session
        ? Session->SelectedManifestationId : FOGEntityId();
    if (Session && Session->IsBoundToStoredRuler() &&
        Session->CurrentDestination == EOGRulerPrimaryDestination::Characters &&
        Session->OwnedRoster.IsValidIndex(OpenIdentityIndex))
    {
        const FOGRosterIdentityViewModel& Identity = Session->OwnedRoster[OpenIdentityIndex];
        RulerId = Session->BoundRulerId;
        IdentityId = Identity.IdentityId;
        ManifestationId = Identity.SelectedManifestationId;
    }

    const bool bChanged = RulerId != DevelopmentContextRulerId ||
        IdentityId != DevelopmentContextIdentityId ||
        ManifestationId != DevelopmentContextManifestationId ||
        SelectedIdentity != DevelopmentSessionIdentityId ||
        SelectedManifestation != DevelopmentSessionManifestationId;
    const bool bHasReviewOrFeedback =
        !PendingDevelopmentReview.ConfirmationToken.IsEmpty() ||
        !DevelopmentFeedback.IsEmpty() || !DevelopmentRefreshWarning.IsEmpty();
    if (bChanged)
    {
        ResetCharacterDevelopment();
        DevelopmentContextRulerId = RulerId;
        DevelopmentContextIdentityId = IdentityId;
        DevelopmentContextManifestationId = ManifestationId;
        DevelopmentSessionIdentityId = SelectedIdentity;
        DevelopmentSessionManifestationId = SelectedManifestation;
        return bHasReviewOrFeedback;
    }
    return false;
}

void UOGGate12NativeRulerWidget::AddDevelopmentPreview(
    UVerticalBox* Parent, const FOGCharacterDevelopmentPreview& Preview)
{
    AddMessage(Parent, Preview.DisplayName.IsEmpty()
        ? Preview.DevelopmentId.ToString() : Preview.DisplayName, 21, DevelopmentGold);
    if (!Preview.Description.IsEmpty())
        AddMessage(Parent, Preview.Description, 16, DevelopmentText);
    AddMessage(Parent, FString::Printf(TEXT("Level %d -> %d   /   Rarity %s -> %s"),
        Preview.Current.Level, Preview.Result.Level,
        *DevelopmentStateName(Preview.Current.CurrentRarity),
        *DevelopmentStateName(Preview.Result.CurrentRarity)), 16, DevelopmentText);
    AddMessage(Parent, FString::Printf(TEXT("Reinforcement: %s -> %s"),
        *DevelopmentReinforcementName(Preview.CurrentReinforcementState,
            Preview.bCurrentlyMaxReinforced),
        *DevelopmentReinforcementName(Preview.ResultReinforcementState,
            Preview.bResultMaxReinforced)), 16, DevelopmentText);

    AddMessage(Parent, TEXT("RESOURCE COSTS"), 18, DevelopmentGold);
    if (Preview.Costs.IsEmpty())
        AddMessage(Parent, TEXT("No resource cost is authored for this option."),
            15, DevelopmentMuted);
    for (const FOGDevelopmentCostPreview& Cost : Preview.Costs)
    {
        const FString ResourceName = Cost.Cost.DisplayName.IsEmpty()
            ? Cost.Cost.ResourceId.ToString() : Cost.Cost.DisplayName;
        const int64 Shortage = Cost.Cost.Amount > Cost.Balance
            ? Cost.Cost.Amount - FMath::Max<int64>(0, Cost.Balance) : 0;
        AddMessage(Parent, FString::Printf(TEXT("%s: cost %s  /  balance %s  /  short %s  /  remaining %s"),
            *ResourceName, *DevelopmentNumber(Cost.Cost.Amount),
            *DevelopmentNumber(Cost.Balance), *DevelopmentNumber(Shortage),
            *DevelopmentNumber(Shortage > 0 ? 0 : Cost.Balance - Cost.Cost.Amount)),
            16, Shortage > 0 ? DevelopmentWarning : DevelopmentText);
    }

    AddMessage(Parent, TEXT("INVENTORY COSTS"), 18, DevelopmentGold);
    if (Preview.ItemCosts.IsEmpty())
        AddMessage(Parent, TEXT("No inventory instances are required."), 15, DevelopmentMuted);
    for (const auto& Input : Preview.ItemCosts)
    {
        AddMessage(Parent, FString::Printf(TEXT("%s: use %d / available %d / remaining %d"),
            *Input.DisplayName, Input.Cost.InstanceCount, Input.AvailableInstances,
            FMath::Max(0, Input.AvailableInstances - Input.Cost.InstanceCount)), 16,
            Input.AvailableInstances < Input.Cost.InstanceCount ? DevelopmentWarning : DevelopmentText);
        AddMessage(Parent, TEXT("Exact rank: ") +
            (Input.Cost.RankId.IsEmpty() ? TEXT("Unranked") : Input.Cost.RankId.ToString()) +
            TEXT(" / quality: ") + (Input.Cost.QualityId.IsEmpty() ? TEXT("Ungraded") : Input.Cost.QualityId.ToString()),
            15, DevelopmentMuted);
    }

    AddMessage(Parent, TEXT("EARNED STAT INCREASES"), 18, DevelopmentGold);
    if (Preview.StatGrants.IsEmpty())
        AddMessage(Parent, TEXT("No stat increases are authored for this option."),
            15, DevelopmentMuted);
    else
    {
        TArray<FName> StatNames;
        Preview.StatGrants.GetKeys(StatNames);
        StatNames.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });
        for (FName StatName : StatNames)
        {
            const FOGLargeNumber& Gain = Preview.StatGrants.FindChecked(StatName);
            const FString FormattedGain = FOGUiNumberFormatter::Format(Gain, true, false);
            const FString Before = FOGUiNumberFormatter::Format(
                Preview.CurrentEarnedStats.FindRef(StatName), true, false);
            const FString After = FOGUiNumberFormatter::Format(
                Preview.ResultEarnedStats.FindRef(StatName), true, false);
            AddMessage(Parent, StatName.ToString() + TEXT(": ") + Before +
                TEXT(" -> ") + After + TEXT("  (+") + FormattedGain + TEXT(")"),
                16, DevelopmentSuccess);
        }
        AddMessage(Parent, TEXT("These are increases from this choice, not total combat stats."),
            15, DevelopmentMuted);
    }

    for (const auto& Route : Preview.GrantedRoutes)
        AddMessage(Parent, TEXT("Route ") + Route.RouteId.ToString() + TEXT(" / ") +
            Route.NodeId.ToString() + TEXT(" -> ") + DevelopmentStateName(Route.State),
            16, DevelopmentSuccess);
    for (const auto& Form : Preview.GrantedForms)
        AddMessage(Parent, TEXT("Unlock form: ") + Form.ToString(), 16, DevelopmentSuccess);

    AddMessage(Parent, TEXT("REQUIREMENTS"), 18, DevelopmentGold);
    if (Preview.bAlreadyCompleted)
        AddMessage(Parent, TEXT("Already completed for this Manifestation."),
            16, DevelopmentMuted);
    else if (Preview.bCanApply && Preview.MissingRequirements.IsEmpty())
        AddMessage(Parent, TEXT("All requirements met."), 16, DevelopmentSuccess);
    else if (Preview.MissingRequirements.IsEmpty())
        AddMessage(Parent, TEXT("This choice is currently unavailable."),
            16, DevelopmentWarning);
    for (const FString& Requirement : Preview.MissingRequirements)
        AddMessage(Parent, TEXT("Required: ") + Requirement, 16, DevelopmentWarning);
}

void UOGGate12NativeRulerWidget::BuildCharacterDevelopment(
    const FOGRosterIdentityViewModel& Identity,
    const FOGRosterManifestationViewModel* Manifestation)
{
    if (!Content || !WidgetTree) return;
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    SynchronizeDevelopmentContext(Session);

    // Share the existing native dark surface, typography and ornamental chrome.
    UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Surface->SetBrushColor(FLinearColor(.010f, .010f, .014f, .97f));
    Surface->SetPadding(FMargin(12.f, 18.f));
    UVerticalBox* Section = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Surface->AddChild(Section);
    UOverlaySlot* SurfaceSlot = Layers->AddChildToOverlay(Surface);
    SurfaceSlot->SetHorizontalAlignment(HAlign_Fill);
    SurfaceSlot->SetVerticalAlignment(VAlign_Fill);
    UOGGate12GothicChromeWidget* Chrome =
        WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(
            UOGGate12GothicChromeWidget::StaticClass());
    Chrome->Configure(DevelopmentGold, INDEX_NONE, false);
    Chrome->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* ChromeSlot = Layers->AddChildToOverlay(Chrome);
    ChromeSlot->SetHorizontalAlignment(HAlign_Fill);
    ChromeSlot->SetVerticalAlignment(VAlign_Fill);
    Content->AddChildToVerticalBox(Layers)->SetPadding(FMargin(4.f, 10.f, 4.f, 14.f));

    AddMessage(Section, TEXT("CHARACTER DEVELOPMENT"), 24, DevelopmentGold);
    if (!DevelopmentFeedback.IsEmpty())
        AddMessage(Section, DevelopmentFeedback, 18,
            bDevelopmentFeedbackError ? DevelopmentWarning : DevelopmentSuccess);
    if (!DevelopmentRefreshWarning.IsEmpty())
        AddMessage(Section, DevelopmentRefreshWarning, 16, DevelopmentWarning);
    if (!Session || !Manifestation)
    {
        ClearDevelopmentReview();
        AddMessage(Section, TEXT("Select an owned Manifestation above to see its development."),
            17, DevelopmentMuted);
        return;
    }

    FOGCharacterManifestationRecord Current;
    FName Reinforcement;
    bool bMaxReinforced = false;
    FString Error;
    if (!Session->ReadOwnedDevelopmentState(Identity.IdentityId,
            Manifestation->ManifestationId, Current, Reinforcement, bMaxReinforced, Error))
    {
        ClearDevelopmentReview();
        AddMessage(Section, TEXT("Character development unavailable: ") + Error,
            17, DevelopmentWarning);
        return;
    }
    AddMessage(Section, Current.BuildLabel.IsEmpty()
        ? TEXT("OWNED MANIFESTATION") : Current.BuildLabel, 19, DevelopmentText);
    AddMessage(Section, TEXT("Copy: ") + Current.ManifestationId.ToString(),
        15, DevelopmentMuted);
    AddMessage(Section, FString::Printf(TEXT("Current level %d   /   Rarity %s"),
        Current.Level, *DevelopmentStateName(Current.CurrentRarity)), 18, DevelopmentText);
    AddMessage(Section, TEXT("Current reinforcement: ") +
        DevelopmentReinforcementName(Reinforcement, bMaxReinforced), 17, DevelopmentText);

    // Saved progression is visible independently of which offers are installed.
    const UOGGameCoreSubsystem* Core = GI->GetSubsystem<UOGGameCoreSubsystem>();
    IOGWorldStore* Store = Core ? Core->GetWorldStore() : nullptr;
    if (Store && Store->IsOpen())
    {
        const FOGEntityId& Copy = Current.ManifestationId;
        FOGEntityRankStateRecord Rank;
        bool bRankFound = false;
        if (!Store->TryReadEntityRankState(Copy,bRankFound,Rank,Error))
            AddMessage(Section,Error,16,DevelopmentWarning);
        else if (bRankFound)
        {
            AddMessage(Section,FString::Printf(TEXT("Attained Rank %s / level %d"),
                *Rank.AttainedRankId.Value,Rank.AttainedLevel),17,DevelopmentText);
            if (Rank.EffectiveRankId.IsValid())
                AddMessage(Section,FString::Printf(TEXT("Effective Rank %s / level %d"),
                    *Rank.EffectiveRankId.Value,Rank.EffectiveLevel),17,DevelopmentText);
            if (Rank.PeakRankId.IsValid() &&
                (Rank.PeakRankId != Rank.AttainedRankId || Rank.PeakLevel != Rank.AttainedLevel))
                AddMessage(Section,FString::Printf(TEXT("Peak Rank %s / level %d"),
                    *Rank.PeakRankId.Value,Rank.PeakLevel),16,DevelopmentMuted);
        }

        FOGManifestationDetailViewModel Detail;
        if (FOGUiViewModelService(*Store).BuildManifestationDetail(Session->BoundRulerId,Copy,
            FString(),4,FOGResolvedStatsProjectionResolver(),Detail,Error))
        {
            AddMessage(Section,FString::Printf(TEXT("Learned techniques: %d"),Detail.TotalLearnedSkillCount),
                17,DevelopmentText);
            for (const FOGSkillViewModel& Skill : Detail.Skills)
                AddMessage(Section,Skill.SkillId.Value + TEXT(" / ") + DevelopmentStateName(Skill.CurrentState),
                    15,DevelopmentMuted);
            for (const FOGRouteNodeViewModel& Route : Detail.RouteNodes)
                if (Route.bEntered || Route.bCompleted)
                    AddMessage(Section,Route.NodeId.Value + TEXT(" / ") + DevelopmentStateName(Route.State),
                        16,DevelopmentText);
            for (const FOGFormViewModel& Form : Detail.Forms)
                if (Form.State == TEXT("unlocked") || Form.State == TEXT("active"))
                    AddMessage(Section,TEXT("Form: ") + Form.FormId.Value,16,DevelopmentText);

            if (!Detail.Proficiencies.IsEmpty())
                AddMessage(Section,TEXT("EQUIPMENT PROFICIENCY"),18,DevelopmentGold);
            for (const FOGEquipmentProficiencySummaryViewModel& Proficiency : Detail.Proficiencies)
            {
                const FString Grade = Proficiency.GradeId.IsValid()
                    ? Proficiency.GradeId.Value : TEXT("No grade recorded");
                AddMessage(Section,Proficiency.ProficiencyId.Value + TEXT(" / ") + Grade,16,DevelopmentText);
            }
            for (const FOGEquipmentSlotViewModel& Equipment : Detail.Equipment)
                if (Equipment.bAffinityKnown && Equipment.AffinityMilestoneId.IsValid())
                    AddMessage(Section,TEXT("Affinity: ") + Equipment.DefinitionId.Value +
                        TEXT(" / ") + Equipment.AffinityMilestoneId.Value,16,DevelopmentText);
        }
        else
            AddMessage(Section,Error,16,DevelopmentWarning);

        FOGCharacterDevelopmentService Development(*Store);
        TMap<FName,FOGLargeNumber> EarnedStats;
        if (!Development.ReadEarnedStats(Copy,EarnedStats,Error))
            AddMessage(Section,Error,16,DevelopmentWarning);
        else if (!EarnedStats.IsEmpty())
        {
            AddMessage(Section,TEXT("EARNED DEVELOPMENT BONUSES"),18,DevelopmentGold);
            TArray<FName> Names;
            EarnedStats.GetKeys(Names);
            Names.Sort(FNameLexicalLess());
            for (FName Name : Names)
                AddMessage(Section,Name.ToString().Replace(TEXT("_"),TEXT(" ")) + TEXT("  +") +
                    FOGUiNumberFormatter::Format(EarnedStats.FindChecked(Name),true,false),16,DevelopmentText);
        }

        TArray<FOGDevelopmentHistoryEntry> History;
        if (!Development.ReadAppliedHistory(Session->BoundRulerId,Copy,8,History,Error))
            AddMessage(Section,Error,16,DevelopmentWarning);
        else if (!History.IsEmpty())
        {
            AddMessage(Section,TEXT("RECENT DEVELOPMENT"),18,DevelopmentGold);
            for (const FOGDevelopmentHistoryEntry& Entry : History)
            {
                AddMessage(Section,Entry.DisplayName + TEXT(" / world tick ") +
                    DevelopmentNumber(Entry.WorldTick),16,DevelopmentText);
                if (Entry.PreviousLevel != Entry.ResultLevel)
                    AddMessage(Section,FString::Printf(TEXT("Level %d to %d"),Entry.PreviousLevel,Entry.ResultLevel),
                        15,DevelopmentMuted);
                if (Entry.PreviousRarity != Entry.ResultRarity)
                    AddMessage(Section,DevelopmentStateName(Entry.PreviousRarity) + TEXT(" to ") +
                        DevelopmentStateName(Entry.ResultRarity),15,DevelopmentMuted);
                if (Entry.bCompletedCeiling || Entry.bReopenedCeiling)
                    AddMessage(Section,Entry.bCompletedCeiling ? TEXT("Reinforcement ceiling reached") :
                        TEXT("Further reinforcement opened"),15,DevelopmentSuccess);
            }
        }
    }

    TArray<FOGCharacterDevelopmentPreview> Options;
    if (!Session->ReadOwnedDevelopmentOptions(Identity.IdentityId,
            Manifestation->ManifestationId, Options, Error))
    {
        ClearDevelopmentReview();
        AddMessage(Section, TEXT("Development options unavailable: ") + Error,
            17, DevelopmentWarning);
        return;
    }
    if (Options.IsEmpty())
    {
        ClearDevelopmentReview();
        AddMessage(Section,
            TEXT("No authored development options are installed for this character's exact version."),
            17, DevelopmentMuted);
        return;
    }

    const bool bExactSelection = Session->SelectedIdentityId == Identity.IdentityId &&
        Session->SelectedManifestationId == Manifestation->ManifestationId;
    if (!bExactSelection)
        AddMessage(Section, TEXT("Select this Manifestation above to review its development choices."),
            17, DevelopmentMuted);

    if (!PendingDevelopmentReview.ConfirmationToken.IsEmpty())
    {
        const FOGCharacterDevelopmentPreview* Fresh = Options.FindByPredicate(
            [&](const FOGCharacterDevelopmentPreview& Option)
            {
                return Option.DevelopmentId == PendingDevelopmentReview.DevelopmentId;
            });
        if (!bExactSelection || !Fresh || !Fresh->bCanApply ||
            Fresh->ConfirmationToken != PendingDevelopmentReview.ConfirmationToken)
        {
            ClearDevelopmentReview();
            AddMessage(Section,
                TEXT("Development details or requirements changed. Review an available choice again."),
                17, DevelopmentWarning);
        }
        else
        {
            AddMessage(Section, TEXT("REVIEW DEVELOPMENT"), 23, DevelopmentGold);
            AddDevelopmentPreview(Section, PendingDevelopmentReview);
            AddMessage(Section,
                TEXT("Confirm applies this choice to the copy shown above and spends the listed resources and inventory instances."),
                17, DevelopmentText);
            AddAction(Section, TEXT("CONFIRM DEVELOPMENT"), TEXT("development/confirm"));
            AddAction(Section, TEXT("CANCEL"), TEXT("development/cancel"));
            return;
        }
    }

    AddMessage(Section, TEXT("AUTHORED CHOICES"), 21, DevelopmentGold);
    for (const FOGCharacterDevelopmentPreview& Option : Options)
    {
        UBorder* Choice = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Choice->SetBrushColor(FLinearColor(.019f, .019f, .025f, .98f));
        Choice->SetPadding(FMargin(5.f, 8.f));
        UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        Choice->AddChild(Details);
        Section->AddChildToVerticalBox(Choice)->SetPadding(FMargin(2.f, 8.f));
        AddDevelopmentPreview(Details, Option);
        if (bExactSelection && Option.bCanApply && !Option.ConfirmationToken.IsEmpty())
            AddAction(Details, TEXT("REVIEW DEVELOPMENT"),
                TEXT("development/review/") + Option.DevelopmentId.ToString());
    }
}

void UOGGate12NativeRulerWidget::HandleDevelopmentAction(
    const FString& Action, UOGGate12RulerSessionSubsystem& Session)
{
    if (bDevelopmentCommandInFlight) return;
    TGuardValue<bool> CommandGuard(bDevelopmentCommandInFlight, true);
    SynchronizeDevelopmentContext(&Session);
    // A second queued tap must not replace a saved result with a purchase error.
    if (Action == TEXT("development/confirm") &&
        PendingDevelopmentReview.ConfirmationToken.IsEmpty() && !DevelopmentFeedback.IsEmpty()) return;
    LastUiError.Reset();
    DevelopmentFeedback.Reset();
    DevelopmentRefreshWarning.Reset();
    bDevelopmentFeedbackError = false;
    FString Error;
    if (Action == TEXT("development/cancel"))
    {
        ClearDevelopmentReview();
        DevelopmentFeedback = TEXT("Development review closed.");
    }
    else if (!Session.IsBoundToStoredRuler() ||
        Session.CurrentDestination != EOGRulerPrimaryDestination::Characters ||
        !DevelopmentContextIdentityId.IsValid() ||
        !DevelopmentContextManifestationId.IsValid() ||
        Session.SelectedIdentityId != DevelopmentContextIdentityId ||
        Session.SelectedManifestationId != DevelopmentContextManifestationId)
        Error = TEXT("Character selection changed. Select this Manifestation and review again.");
    else if (Action.StartsWith(TEXT("development/review/")))
    {
        ClearDevelopmentReview();
        const FOGContentId ChoiceId(Action.Mid(FString(TEXT("development/review/")).Len()));
        TArray<FOGCharacterDevelopmentPreview> Options;
        if (Session.ReadOwnedDevelopmentOptions(DevelopmentContextIdentityId,
                DevelopmentContextManifestationId, Options, Error))
        {
            const FOGCharacterDevelopmentPreview* Choice = Options.FindByPredicate(
                [&](const FOGCharacterDevelopmentPreview& Option)
                {
                    return Option.DevelopmentId == ChoiceId;
                });
            if (!Choice || !Choice->bCanApply || Choice->ConfirmationToken.IsEmpty())
                Error = TEXT("This development choice is no longer available. Check its requirements.");
            else
            {
                PendingDevelopmentReview = *Choice;
                PendingDevelopmentRequestId = FOGEntityId::NewId();
            }
        }
    }
    else if (Action == TEXT("development/confirm"))
    {
        // Copy the reviewed command before notifications can rebuild this widget.
        const FOGCharacterDevelopmentPreview Review = PendingDevelopmentReview;
        const FOGEntityId RequestId = PendingDevelopmentRequestId;
        const FOGContentId IdentityId = DevelopmentContextIdentityId;
        const FOGEntityId ManifestationId = DevelopmentContextManifestationId;
        ClearDevelopmentReview();
        if (Review.ConfirmationToken.IsEmpty() || !RequestId.IsValid() ||
            Review.ManifestationId != ManifestationId ||
            Review.Current.IdentityId != IdentityId ||
            Review.Current.OwningRulerId != Session.BoundRulerId)
            Error = TEXT("Review an available development choice before confirming.");
        else if (Session.ConfirmOwnedDevelopment(IdentityId, ManifestationId,
                Review.DevelopmentId, Review.ConfirmationToken, RequestId, Error))
        {
            // A true command result is canonical success. Display refresh problems
            // remain separate, and must never offer to repeat the transaction.
            DevelopmentRefreshWarning = Error;
            Error.Reset();
            DevelopmentFeedback = Review.DisplayName + TEXT(" saved for this Manifestation.");
            for (const auto& Cost : Review.Costs)
                DevelopmentFeedback += TEXT(" Spent ") + DevelopmentNumber(Cost.Cost.Amount) + TEXT(" ") +
                    (Cost.Cost.DisplayName.IsEmpty() ? Cost.Cost.ResourceId.ToString() : Cost.Cost.DisplayName) + TEXT(".");
            for (const auto& Cost : Review.ItemCosts)
                DevelopmentFeedback += FString::Printf(TEXT(" Consumed %d %s."), Cost.Cost.InstanceCount, *Cost.DisplayName);
            FOGCharacterManifestationRecord Current;
            FName Reinforcement;
            bool bComplete = false;
            FString ReadError;
            if (Session.ReadOwnedDevelopmentState(IdentityId, ManifestationId,
                    Current, Reinforcement, bComplete, ReadError))
                DevelopmentFeedback += FString::Printf(
                    TEXT(" Now level %d, rarity %s; reinforcement %s."), Current.Level,
                    *DevelopmentStateName(Current.CurrentRarity),
                    *DevelopmentReinforcementName(Reinforcement, bComplete));
            else
                DevelopmentRefreshWarning += TEXT(" Saved state display unavailable: ") + ReadError;
        }
        ClearDevelopmentReview();
    }
    else
        Error = TEXT("Unrecognized development action. Review an available choice again.");

    if (!Error.IsEmpty())
    {
        ClearDevelopmentReview();
        DevelopmentFeedback = Error;
        bDevelopmentFeedbackError = true;
    }
    BuildCurrentPage();
}
