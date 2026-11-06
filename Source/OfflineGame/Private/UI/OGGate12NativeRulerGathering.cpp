#include "UI/OGGate12NativeRulerWidget.h"

#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"

namespace
{
const FLinearColor GatheringGold(.77f,.67f,.45f,1.f);
const FLinearColor GatheringText(.90f,.88f,.92f,1.f);
const FLinearColor GatheringMuted(.60f,.60f,.67f,1.f);

const TCHAR* DispatchTypeName(EOGDispatchType Type)
{
    switch (Type)
    {
    case EOGDispatchType::Exploration: return TEXT("Exploration");
    case EOGDispatchType::Gathering: return TEXT("Gathering");
    case EOGDispatchType::Reconnaissance: return TEXT("Reconnaissance");
    case EOGDispatchType::Espionage: return TEXT("Espionage");
    case EOGDispatchType::Support: return TEXT("Support");
    default: return TEXT("Dispatch");
    }
}

FString CopyName(const UOGGate12RulerSessionSubsystem& Session,const FOGEntityId& Id)
{
    for (const auto& Identity:Session.OwnedRoster)
        for (int32 I=0;I<Identity.Manifestations.Num();++I)
            if (Identity.Manifestations[I].ManifestationId==Id)
            {
                const FString Name=Identity.DisplayNameKey.IsEmpty()?Identity.IdentityId.ToString():Identity.DisplayNameKey;
                return Identity.Manifestations.Num()==1?Name:Name+FString::Printf(TEXT(" / copy %d"),I+1);
            }
    return TEXT("Assigned character / ")+Id.ToString();
}

FString PartyName(const UOGGate12RulerSessionSubsystem& Session,const TArray<FOGEntityId>& Participants)
{
    TArray<FString> Names;
    for (const auto& Participant:Participants) Names.Add(CopyName(Session,Participant));
    return FString::Join(Names,TEXT(", "));
}
}

void UOGGate12NativeRulerWidget::ResetGathering()
{
    bGatheringOpen=false; bGatheringRefresh=false;
    GatheringRulerId=FOGEntityId(); GatheringTerritoryId=FOGEntityId(); GatheringParticipants.Reset();
    GatheringRequest=FOGEntityId(); PendingGathering=FOGGatheringDispatchQuote(); GatheringFeedback.Reset();
}

void UOGGate12NativeRulerWidget::HandleGatheringEvents(const TArray<FOGWorldEvent>& Events)
{
    if (bGatheringOpen && Events.ContainsByPredicate([&](const auto& Event)
    { return Event.PrimaryEntity==GatheringRulerId &&
        (Event.EventType==TEXT("dispatch.gathering_resolved") || Event.EventType==TEXT("dispatch.authored_resolved")); }))
        bGatheringRefresh=true;
}

void UOGGate12NativeRulerWidget::BuildGathering(UOGGate12RulerSessionSubsystem& Session,const FOGTerritorySummaryViewModel& Holding)
{
    if (GatheringRulerId!=Session.BoundRulerId || GatheringTerritoryId!=Holding.TerritoryId)
    {
        ResetGathering(); GatheringRulerId=Session.BoundRulerId; GatheringTerritoryId=Holding.TerritoryId;
    }
    AddAction(Content,bGatheringOpen?TEXT("CLOSE DISPATCH ORDERS"):TEXT("DISPATCH ORDERS"),TEXT("gathering/toggle"));
    if (!bGatheringOpen) return;
    if (!GatheringFeedback.IsEmpty()) AddMessage(Content,GatheringFeedback,17,GatheringText);

    auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    auto* Store=Core && Core->IsCoreReady()?Core->GetWorldStore():nullptr;
    if (!Store) return;
    FOGGatheringDispatch Dispatch(*Store);
    FString Error;
    AddMessage(Content,TEXT("DISPATCH HISTORY"),20,GatheringGold);
    TArray<FOGDispatchRecord> Orders;
    if (!Dispatch.ReadOrders(Session.BoundRulerId,Orders,Error)) AddMessage(Content,Error,16,GatheringMuted);
    else if (Orders.IsEmpty()) AddMessage(Content,TEXT("No parties have been dispatched."),16,GatheringMuted);
    else
    {
        int32 Completed=0;
        for (const auto& Order:Orders)
        {
            if (Order.Status!=EOGDispatchStatus::Active && ++Completed>8) continue;
            FString Name,Result;
            if (FOGGatheringDispatch::ReadSummary(Order,Name,Result))
                AddMessage(Content,FString(DispatchTypeName(Order.Type))+TEXT("  /  ")+Name+TEXT("  /  ")+
                    PartyName(Session,Order.ParticipantEntityIds)+TEXT("\n")+Result,17,GatheringText);
        }
    }
    if (GatheringRequest.IsValid())
    {
        const auto* Definition=FOGGatheringDispatch::FindDefinition(PendingGathering.DefinitionId);
        FOGGatheringDispatchQuote Fresh;
        if (Definition && !GatheringParticipants.Contains(Session.SelectedManifestationId) &&
            Dispatch.PreviewParty(*Definition,Session.BoundRulerId,Holding.TerritoryId,GatheringParticipants,
                Core->GetCanonicalWorldTick(),Fresh,Error) && Fresh.bCanStart && Fresh.Token==PendingGathering.Token)
        {
            AddMessage(Content,Fresh.Name,22,GatheringGold);
            AddMessage(Content,PartyName(Session,Fresh.ParticipantIds)+TEXT("  /  ")+
                DispatchTypeName(Fresh.Type),18,GatheringText);
            if (Fresh.Amount>0)
                AddMessage(Content,Fresh.ResourceName+TEXT(" × ")+FText::AsNumber(Fresh.Amount).ToString(),18,GatheringText);
            AddMessage(Content,TEXT("Duration: ")+FText::AsNumber(Fresh.Duration).ToString()+
                TEXT(" world ticks."),16,GatheringMuted);
            if (Fresh.Type==EOGDispatchType::Gathering)
                AddMessage(Content,TEXT("Collection depends on the holding and its remaining stock at arrival."),16,GatheringMuted);
            else
            {
                AddMessage(Content,Fresh.Briefing,17,GatheringText);
                AddMessage(Content,Fresh.RiskDescription,16,GatheringMuted);
                if (Fresh.Risk>Fresh.RiskTolerance)
                    AddMessage(Content,TEXT("The authored risk policy calls for return without completing this order."),
                        16,GatheringMuted);
                for (const FString& Objective:Fresh.ObjectiveNames)
                    AddMessage(Content,Objective,16,GatheringText);
            }
            AddAction(Content,TEXT("DISPATCH"),TEXT("gathering/confirm/")+GatheringRequest.ToString());
            AddAction(Content,TEXT("CANCEL"),TEXT("gathering/cancel"));
            return;
        }
        GatheringRequest=FOGEntityId(); PendingGathering=FOGGatheringDispatchQuote();
        AddMessage(Content,Error.IsEmpty()?FString(TEXT("The dispatch changed. Review it again.")):Error,16,GatheringMuted);
    }

    auto* Library=UOGDefinitionLibrary::Get();
    TArray<UDataAsset*> Assets;
    if (Library) Library->List(TEXT("gathering_dispatch"),UOGGatheringDispatchDefinition::StaticClass(),Assets);
    Assets.RemoveAll([&](const UDataAsset* Asset)
    { return CastChecked<UOGGatheringDispatchDefinition>(Asset)->LocationId!=Holding.RootLocationId; });
    if (Assets.IsEmpty())
    { AddMessage(Content,TEXT("No dispatch orders are available from this holding."),16,GatheringMuted); return; }

    AddMessage(Content,FString::Printf(TEXT("DISPATCH PARTY / %d SELECTED"),GatheringParticipants.Num()),20,GatheringGold);
    for (const auto& Participant:GatheringParticipants)
        AddAction(Content,TEXT("REMOVE / ")+CopyName(Session,Participant),
            TEXT("gathering/participant/")+Participant.ToString());

    TSet<FOGEntityId> Assigned;
    if (!FOGGatheringDispatch::ReadAssignedParticipants(*Store,Assigned,Error))
    {
        AddMessage(Content,Error,16,GatheringMuted);
        return;
    }
    for (const auto& Identity:Session.OwnedRoster)
        for (const auto& Copy:Identity.Manifestations)
        {
            if (Copy.bLocked || Copy.ManifestationId==Session.SelectedManifestationId ||
                GatheringParticipants.Contains(Copy.ManifestationId) || Assigned.Contains(Copy.ManifestationId)) continue;
            AddAction(Content,TEXT("ADD / ")+CopyName(Session,Copy.ManifestationId),
                TEXT("gathering/participant/")+Copy.ManifestationId.ToString());
        }
    if (GatheringParticipants.Contains(Session.SelectedManifestationId))
    {
        AddMessage(Content,TEXT("Remove the character you currently control from the party."),16,GatheringMuted);
        return;
    }

    bool bVisibleOrder=false;
    for (const auto* Asset:Assets)
    {
        const auto* Definition=CastChecked<UOGGatheringDispatchDefinition>(Asset);
        FOGGatheringDispatchQuote Quote;
        if (!Dispatch.PreviewParty(*Definition,Session.BoundRulerId,Holding.TerritoryId,GatheringParticipants,
            Core->GetCanonicalWorldTick(),Quote,Error) || !Quote.bVisible) continue;
        bVisibleOrder=true;
        const FString PartyLimit=Quote.MinimumPartySize==Quote.MaximumPartySize
            ? FString::Printf(TEXT("%d"),Quote.MinimumPartySize)
            : FString::Printf(TEXT("%d–%d"),Quote.MinimumPartySize,Quote.MaximumPartySize);
        FString Label=FString(DispatchTypeName(Quote.Type))+TEXT(" / ")+Quote.Name+
            TEXT(" / party size ")+PartyLimit;
        if (Quote.Amount>0)
            Label+=TEXT(" / ")+Quote.ResourceName+TEXT(" × ")+FText::AsNumber(Quote.Amount).ToString();
        if (Quote.bCanStart)
            AddAction(Content,Label,TEXT("gathering/review/")+Quote.DefinitionId.ToString());
        else AddMessage(Content,Label+TEXT(" / ")+Quote.Reason,16,GatheringMuted);
    }
    if (!bVisibleOrder)
        AddMessage(Content,TEXT("No known orders are available for this character."),16,GatheringMuted);
}

void UOGGate12NativeRulerWidget::HandleGatheringAction(const FString& Action,UOGGate12RulerSessionSubsystem& Session)
{
    if (bGatheringBusy || !Session.IsBoundToStoredRuler() || Session.CurrentDestination!=EOGRulerPrimaryDestination::Territory ||
        GatheringRulerId!=Session.BoundRulerId || GatheringTerritoryId!=SelectedTerritoryId) return;
    TGuardValue<bool> Busy(bGatheringBusy,true);
    if (Action==TEXT("gathering/toggle"))
    {
        bGatheringOpen=!bGatheringOpen;
        GatheringRequest=FOGEntityId(); PendingGathering=FOGGatheringDispatchQuote(); GatheringFeedback.Reset();
        BuildCurrentPage(); return;
    }
    if (!bGatheringOpen) return;
    const auto* Holding=Session.TerritoryProjection.Territories.FindByPredicate(
        [&](const auto& Row){return Row.TerritoryId==GatheringTerritoryId;});
    auto* Core=GetGameInstance()->GetSubsystem<UOGGameCoreSubsystem>();
    auto* Store=Core && Core->IsCoreReady()?Core->GetWorldStore():nullptr;
    auto* Clock=Core?Core->GetCanonicalClockRuntime():nullptr;
    if (!Holding || !Store || !Clock) return;
    FOGGatheringDispatch Dispatch(*Store);
    FString Error;
    if (Action.StartsWith(TEXT("gathering/confirm/")))
    {
        if (!GatheringRequest.IsValid() || Action!=TEXT("gathering/confirm/")+GatheringRequest.ToString() ||
            PendingGathering.ParticipantIds!=GatheringParticipants ||
            GatheringParticipants.Contains(Session.SelectedManifestationId)) return;
        const auto Review=PendingGathering; const auto Request=GatheringRequest;
        GatheringRequest=FOGEntityId(); PendingGathering=FOGGatheringDispatchQuote();
        if (Dispatch.Start(Review,Request,*Clock,Error))
        {
            GatheringParticipants.Reset();
            GatheringFeedback=Error.IsEmpty()?TEXT("Party dispatched."):Error;
        }
        else GatheringFeedback=Error;
    }
    else
    {
        GatheringRequest=FOGEntityId(); PendingGathering=FOGGatheringDispatchQuote(); GatheringFeedback.Reset();
        const FString ParticipantPrefix=TEXT("gathering/participant/"),ReviewPrefix=TEXT("gathering/review/");
        if (Action.StartsWith(ParticipantPrefix))
        {
            FGuid Guid;
            if (FGuid::Parse(Action.Mid(ParticipantPrefix.Len()),Guid) && Guid.IsValid())
            {
                const FOGEntityId Participant(Guid);
                if (GatheringParticipants.RemoveSingle(Participant)==0 && Participant!=Session.SelectedManifestationId)
                {
                    const bool bOwnedSelectable=Session.OwnedRoster.ContainsByPredicate([&](const auto& Identity)
                    {
                        return Identity.Manifestations.ContainsByPredicate([&](const auto& Copy)
                        { return Copy.ManifestationId==Participant && !Copy.bLocked; });
                    });
                    bool Assigned=false;
                    if (bOwnedSelectable && FOGGatheringDispatch::IsAssigned(
                        *Store,Session.BoundRulerId,Participant,Assigned,Error) && !Assigned)
                        GatheringParticipants.Add(Participant);
                    GatheringFeedback=Error;
                }
            }
        }
        else if (Action.StartsWith(ReviewPrefix) && !GatheringParticipants.Contains(Session.SelectedManifestationId))
        {
            const auto* Definition=FOGGatheringDispatch::FindDefinition(FOGContentId(Action.Mid(ReviewPrefix.Len())));
            FOGGatheringDispatchQuote Review;
            if (Definition && Definition->LocationId==Holding->RootLocationId &&
                Dispatch.PreviewParty(*Definition,Session.BoundRulerId,Holding->TerritoryId,GatheringParticipants,
                    Core->GetCanonicalWorldTick(),Review,Error))
            {
                if (Review.bCanStart) { PendingGathering=MoveTemp(Review); GatheringRequest=FOGEntityId::NewId(); }
                else Error=Review.Reason;
            }
            GatheringFeedback=Error;
        }
        else if (Action!=TEXT("gathering/cancel")) return;
    }
    BuildCurrentPage();
}
