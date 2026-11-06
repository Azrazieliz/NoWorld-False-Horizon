#include "UI/OGGate12NativeRulerWidget.h"

#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"

namespace
{
const FLinearColor DiplomacyGold(.77f,.67f,.45f,1.f);
const FLinearColor DiplomacyText(.90f,.88f,.92f,1.f);
const FLinearColor DiplomacyMuted(.60f,.60f,.67f,1.f);
}

void UOGGate12NativeRulerWidget::ResetDiplomacy()
{
    bDiplomacyOpen=false;
    DiplomacyRulerId=FOGEntityId(); DiplomacyOwnFaction=FOGEntityId(); DiplomacyOtherFaction=FOGEntityId();
    DiplomacyRequest=FOGEntityId(); PendingDiplomacy=FOGRulerDiplomacyReview(); DiplomacyFeedback.Reset();
}

void UOGGate12NativeRulerWidget::BuildDiplomacy(UOGGate12RulerSessionSubsystem& Session)
{
    if (DiplomacyRulerId!=Session.BoundRulerId) ResetDiplomacy();
    AddAction(Content,bDiplomacyOpen?TEXT("CLOSE DIPLOMACY"):TEXT("DIPLOMACY"),TEXT("diplomacy/toggle"));
    if (!bDiplomacyOpen) return;
    if (!DiplomacyFeedback.IsEmpty()) AddMessage(Content,DiplomacyFeedback,17,DiplomacyText);

    auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    auto* Store=Core && Core->IsCoreReady()?Core->GetWorldStore():nullptr;
    if (!Store) return;
    FOGRulerDiplomacyService Diplomacy(*Store);
    FString Error;
    TArray<FOGRulerDiplomacyFaction> Led,Known;
    if (!Diplomacy.ReadFactions(Session.BoundRulerId,Led,Known,Error))
    { AddMessage(Content,Error,17,DiplomacyMuted); return; }

    if (Led.IsEmpty())
    { AddMessage(Content,TEXT("You do not lead a faction."),17,DiplomacyMuted); return; }
    AddMessage(Content,TEXT("ACTING FACTION"),19,DiplomacyGold);
    if (!Led.ContainsByPredicate([&](const auto& F){return F.Id==DiplomacyOwnFaction;}))
    {
        DiplomacyOwnFaction=Led[0].Id; DiplomacyRequest=FOGEntityId();
        PendingDiplomacy=FOGRulerDiplomacyReview();
    }
    for (const auto& Faction:Led)
        AddAction(Content,(Faction.Id==DiplomacyOwnFaction?TEXT("SELECTED / "):TEXT(""))+Faction.Name,
            TEXT("diplomacy/own/")+Faction.Id.ToString());

    AddMessage(Content,TEXT("KNOWN CONTACTS"),19,DiplomacyGold);
    if (Known.IsEmpty())
    { AddMessage(Content,TEXT("No foreign factions have been discovered."),17,DiplomacyMuted); return; }
    for (const auto& Faction:Known)
        AddAction(Content,(Faction.Id==DiplomacyOtherFaction?TEXT("SELECTED / "):TEXT(""))+Faction.Name,
            TEXT("diplomacy/contact/")+Faction.Id.ToString());

    if (!Known.ContainsByPredicate([&](const auto& F){return F.Id==DiplomacyOtherFaction;}))
    { DiplomacyRequest=FOGEntityId(); PendingDiplomacy=FOGRulerDiplomacyReview(); return; }
    if (DiplomacyRequest.IsValid())
    {
        FOGRulerDiplomacyReview Fresh;
        if (Diplomacy.Review(Session.BoundRulerId,DiplomacyOwnFaction,DiplomacyOtherFaction,PendingDiplomacy.Action,Fresh,Error) &&
            Fresh.bCanApply && Fresh.Token==PendingDiplomacy.Token)
        {
            AddMessage(Content,FOGRulerDiplomacyService::ActionLabel(Fresh.Action),22,DiplomacyGold);
            AddMessage(Content,Fresh.OwnName+TEXT("  /  ")+Fresh.OtherName,18,DiplomacyText);
            AddAction(Content,TEXT("CONFIRM"),TEXT("diplomacy/confirm/")+DiplomacyRequest.ToString());
            AddAction(Content,TEXT("CANCEL"),TEXT("diplomacy/cancel"));
            return;
        }
        DiplomacyRequest=FOGEntityId(); PendingDiplomacy=FOGRulerDiplomacyReview();
        AddMessage(Content,Error.IsEmpty()?FString(TEXT("The agreement changed. Review the current terms.")):Error,17,DiplomacyMuted);
    }
    FOGRulerDiplomacyLinks Links;
    if (!Diplomacy.ReadLinks(Session.BoundRulerId,DiplomacyOwnFaction,DiplomacyOtherFaction,Links,Error))
    { AddMessage(Content,Error,17,DiplomacyMuted); return; }
    AddMessage(Content,Links.Alliance.bActive?TEXT("Alliance active"):TEXT("No active alliance"),17,DiplomacyText);
    if (Links.IncomingSupport.bActive) AddMessage(Content,TEXT("They have pledged support to your faction."),17,DiplomacyText);
    if (Links.OutgoingSupport.bActive) AddMessage(Content,TEXT("Your faction has pledged support."),17,DiplomacyText);
    if (Links.Subordination.bActive) AddMessage(Content,TEXT("Your faction is subordinate to this faction."),17,DiplomacyText);
    auto Command=[this](EOGRulerDiplomacyAction Choice)
    {
        AddAction(Content,FOGRulerDiplomacyService::ActionLabel(Choice),
            TEXT("diplomacy/review/")+LexToString(static_cast<int32>(Choice)));
    };
    if (Links.Alliance.bActive) Command(EOGRulerDiplomacyAction::EndAlliance);
    Command(Links.OutgoingSupport.bActive?EOGRulerDiplomacyAction::WithdrawSupport:EOGRulerDiplomacyAction::PledgeSupport);
    if (Links.Subordination.bActive) Command(EOGRulerDiplomacyAction::EndSubordination);
}

void UOGGate12NativeRulerWidget::HandleDiplomacyAction(const FString& Action,UOGGate12RulerSessionSubsystem& Session)
{
    if (bDiplomacyBusy || !Session.IsBoundToStoredRuler() || Session.CurrentDestination!=EOGRulerPrimaryDestination::Territory)
        return;
    TGuardValue<bool> Busy(bDiplomacyBusy,true);
    if (Action==TEXT("diplomacy/toggle"))
    {
        const bool Open=!bDiplomacyOpen || DiplomacyRulerId!=Session.BoundRulerId;
        ResetDiplomacy();
        bDiplomacyOpen=Open;
        DiplomacyRulerId=Session.BoundRulerId;
        BuildCurrentPage();
        return;
    }
    if (!bDiplomacyOpen || DiplomacyRulerId!=Session.BoundRulerId) return;
    auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    auto* Store=Core && Core->IsCoreReady()?Core->GetWorldStore():nullptr;
    if (!Store) return;
    FOGRulerDiplomacyService Diplomacy(*Store);
    FString Error;
    if (Action.StartsWith(TEXT("diplomacy/confirm/")))
    {
        if (!DiplomacyRequest.IsValid() || Action!=TEXT("diplomacy/confirm/")+DiplomacyRequest.ToString() ||
            PendingDiplomacy.RulerId!=Session.BoundRulerId || PendingDiplomacy.OwnFactionId!=DiplomacyOwnFaction ||
            PendingDiplomacy.OtherFactionId!=DiplomacyOtherFaction) return;
        const auto Review=PendingDiplomacy;
        const auto Request=DiplomacyRequest;
        DiplomacyRequest=FOGEntityId(); PendingDiplomacy=FOGRulerDiplomacyReview();
        if (Diplomacy.Confirm(Review,Request,Core->GetCanonicalWorldTick(),Error))
        {
            DiplomacyFeedback=FOGRulerDiplomacyService::ActionLabel(Review.Action)+TEXT(" saved.");
            FString RefreshError;
            if (!Session.RefreshCanonicalProjections(RefreshError)) DiplomacyFeedback+=TEXT(" ")+RefreshError;
        }
        else DiplomacyFeedback=Error;
        BuildCurrentPage();
        return;
    }
    DiplomacyRequest=FOGEntityId(); PendingDiplomacy=FOGRulerDiplomacyReview(); DiplomacyFeedback.Reset();
    const FString OwnPrefix=TEXT("diplomacy/own/"), ContactPrefix=TEXT("diplomacy/contact/"),
        ReviewPrefix=TEXT("diplomacy/review/");
    if (Action.StartsWith(OwnPrefix) || Action.StartsWith(ContactPrefix))
    {
        const bool Own=Action.StartsWith(OwnPrefix);
        FGuid Id;
        if (FGuid::Parse(Action.Mid(Own?OwnPrefix.Len():ContactPrefix.Len()),Id))
        {
            if (Own) { DiplomacyOwnFaction=FOGEntityId(Id); DiplomacyOtherFaction=FOGEntityId(); }
            else DiplomacyOtherFaction=FOGEntityId(Id);
        }
    }
    else if (Action.StartsWith(ReviewPrefix))
    {
        int32 Value=INDEX_NONE;
        if (LexTryParseString(Value,*Action.Mid(ReviewPrefix.Len())) && Value>=0 &&
            Value<=static_cast<int32>(EOGRulerDiplomacyAction::EndSubordination))
        {
            FOGRulerDiplomacyReview Review;
            if (Diplomacy.Review(Session.BoundRulerId,DiplomacyOwnFaction,DiplomacyOtherFaction,
                static_cast<EOGRulerDiplomacyAction>(Value),Review,Error))
            {
                if (Review.bCanApply) { PendingDiplomacy=MoveTemp(Review); DiplomacyRequest=FOGEntityId::NewId(); }
                else Error=Review.Reason;
            }
        }
    }
    else if (Action!=TEXT("diplomacy/cancel")) return;
    DiplomacyFeedback=Error;
    BuildCurrentPage();
}
