#include "UI/OGGate12NativeRulerWidget.h"

#include "Engine/GameInstance.h"
#include "Runtime/OGGameCoreSubsystem.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"

namespace
{
const FLinearColor GatheringGold(.77f,.67f,.45f,1.f);
const FLinearColor GatheringText(.90f,.88f,.92f,1.f);
const FLinearColor GatheringMuted(.60f,.60f,.67f,1.f);

FString CopyName(const UOGGate12RulerSessionSubsystem& Session,const FOGEntityId& Id)
{
    for (const auto& Identity:Session.OwnedRoster)
        for (int32 I=0;I<Identity.Manifestations.Num();++I)
            if (Identity.Manifestations[I].ManifestationId==Id)
            {
                const FString Name=Identity.DisplayNameKey.IsEmpty()?Identity.IdentityId.ToString():Identity.DisplayNameKey;
                return Identity.Manifestations.Num()==1?Name:Name+FString::Printf(TEXT(" / copy %d"),I+1);
            }
    return TEXT("Assigned character");
}
}

void UOGGate12NativeRulerWidget::ResetGathering()
{
    bGatheringOpen=false; bGatheringRefresh=false;
    GatheringRulerId=FOGEntityId(); GatheringTerritoryId=FOGEntityId(); GatheringParticipantId=FOGEntityId();
    GatheringRequest=FOGEntityId(); PendingGathering=FOGGatheringDispatchQuote(); GatheringFeedback.Reset();
}

void UOGGate12NativeRulerWidget::HandleGatheringEvents(const TArray<FOGWorldEvent>& Events)
{
    if (bGatheringOpen && Events.ContainsByPredicate([&](const auto& Event)
    { return Event.PrimaryEntity==GatheringRulerId && Event.EventType==TEXT("dispatch.gathering_resolved"); }))
        bGatheringRefresh=true;
}

void UOGGate12NativeRulerWidget::BuildGathering(UOGGate12RulerSessionSubsystem& Session,const FOGTerritorySummaryViewModel& Holding)
{
    if (GatheringRulerId!=Session.BoundRulerId || GatheringTerritoryId!=Holding.TerritoryId)
    {
        ResetGathering(); GatheringRulerId=Session.BoundRulerId; GatheringTerritoryId=Holding.TerritoryId;
    }
    AddAction(Content,bGatheringOpen?TEXT("CLOSE GATHERING ORDERS"):TEXT("GATHERING ORDERS"),TEXT("gathering/toggle"));
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
    else if (Orders.IsEmpty()) AddMessage(Content,TEXT("No gathering parties have been dispatched."),16,GatheringMuted);
    else
    {
        int32 Completed=0;
        for (const auto& Order:Orders)
        {
            if (Order.Status!=EOGDispatchStatus::Active && ++Completed>8) continue;
            FString Name,Result;
            if (FOGGatheringDispatch::ReadSummary(Order,Name,Result))
                AddMessage(Content,Name+TEXT("  /  ")+(Order.ParticipantEntityIds.Num()==1?
                    CopyName(Session,Order.ParticipantEntityIds[0]):FString(TEXT("Party")))+TEXT("\n")+Result,17,GatheringText);
        }
    }
    if (GatheringRequest.IsValid())
    {
        const auto* Definition=FOGGatheringDispatch::FindDefinition(PendingGathering.DefinitionId);
        FOGGatheringDispatchQuote Fresh;
        if (Definition && GatheringParticipantId!=Session.SelectedManifestationId &&
            Dispatch.Preview(*Definition,Session.BoundRulerId,Holding.TerritoryId,GatheringParticipantId,
                Core->GetCanonicalWorldTick(),Fresh,Error) && Fresh.bCanStart && Fresh.Token==PendingGathering.Token)
        {
            AddMessage(Content,Fresh.Name,22,GatheringGold);
            AddMessage(Content,CopyName(Session,Fresh.ParticipantId)+TEXT("  /  ")+Fresh.ResourceName+
                TEXT(" × ")+FText::AsNumber(Fresh.Amount).ToString(),18,GatheringText);
            AddMessage(Content,TEXT("Duration: ")+FText::AsNumber(Fresh.Duration).ToString()+
                TEXT(" world ticks. Collection depends on the holding and its remaining stock at arrival."),16,GatheringMuted);
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
    { AddMessage(Content,TEXT("No gathering orders are available from this holding."),16,GatheringMuted); return; }

    AddMessage(Content,TEXT("ASSIGN AN OWNED CHARACTER"),20,GatheringGold);
    for (const auto& Identity:Session.OwnedRoster)
        for (const auto& Copy:Identity.Manifestations)
        {
            if (Copy.bLocked || Copy.ManifestationId==Session.SelectedManifestationId) continue;
            bool Assigned=false;
            if (!FOGGatheringDispatch::IsAssigned(*Store,Session.BoundRulerId,Copy.ManifestationId,Assigned,Error) || Assigned) continue;
            AddAction(Content,(Copy.ManifestationId==GatheringParticipantId?TEXT("SELECTED / "):TEXT(""))+
                CopyName(Session,Copy.ManifestationId),TEXT("gathering/participant/")+Copy.ManifestationId.ToString());
        }
    if (!GatheringParticipantId.IsValid())
    { AddMessage(Content,TEXT("Select an available owned copy other than the one you control."),16,GatheringMuted); return; }

    for (const auto* Asset:Assets)
    {
        const auto* Definition=CastChecked<UOGGatheringDispatchDefinition>(Asset);
        FOGGatheringDispatchQuote Quote;
        if (!Dispatch.Preview(*Definition,Session.BoundRulerId,Holding.TerritoryId,GatheringParticipantId,
            Core->GetCanonicalWorldTick(),Quote,Error)) continue;
        if (Quote.bCanStart)
            AddAction(Content,Quote.Name+TEXT(" / ")+Quote.ResourceName+TEXT(" × ")+FText::AsNumber(Quote.Amount).ToString(),
                TEXT("gathering/review/")+Quote.DefinitionId.ToString());
        else AddMessage(Content,Quote.Name+TEXT(" / ")+Quote.Reason,16,GatheringMuted);
    }
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
            PendingGathering.ParticipantId!=GatheringParticipantId ||
            PendingGathering.ParticipantId==Session.SelectedManifestationId) return;
        const auto Review=PendingGathering; const auto Request=GatheringRequest;
        GatheringRequest=FOGEntityId(); PendingGathering=FOGGatheringDispatchQuote();
        if (Dispatch.Start(Review,Request,*Clock,Error))
        {
            GatheringParticipantId=FOGEntityId();
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
            if (FGuid::Parse(Action.Mid(ParticipantPrefix.Len()),Guid) && FOGEntityId(Guid)!=Session.SelectedManifestationId)
                GatheringParticipantId=FOGEntityId(Guid);
        }
        else if (Action.StartsWith(ReviewPrefix) && GatheringParticipantId!=Session.SelectedManifestationId)
        {
            const auto* Definition=FOGGatheringDispatch::FindDefinition(FOGContentId(Action.Mid(ReviewPrefix.Len())));
            FOGGatheringDispatchQuote Review;
            if (Definition && Definition->LocationId==Holding->RootLocationId &&
                Dispatch.Preview(*Definition,Session.BoundRulerId,Holding->TerritoryId,GatheringParticipantId,
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
