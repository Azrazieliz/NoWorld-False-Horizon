#include "UI/OGGate12NativeRulerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "UI/OGGate12GothicChromeWidget.h"
#include "Objects/OGObjectActionDefinition.h"
#include "Content/OGDefinitionLibrary.h"
#include "Misc/GuardValue.h"

namespace
{
const FLinearColor InventorySilver(.77f, .73f, .83f, 1.f);
const FLinearColor InventoryText(.90f, .90f, .89f, 1.f);
const FLinearColor InventoryMuted(.55f, .58f, .61f, 1.f);
const FLinearColor InventoryWarning(.88f, .57f, .43f, 1.f);
const FLinearColor InventorySuccess(.60f, .79f, .65f, 1.f);
FString InventoryCopyLabel(const UOGGate12RulerSessionSubsystem* Session, const FOGEntityId& Id)
{
    if (!Session) return TEXT("Owned character");
    if (Id == Session->BoundRulerId) return TEXT("This Ruler");
    for (const auto& Identity : Session->OwnedRoster)
        for (int32 I = 0; I < Identity.Manifestations.Num(); ++I)
        {
            const auto& Copy = Identity.Manifestations[I];
            if (Copy.ManifestationId != Id) continue;
            const FString Name = Identity.DisplayNameKey.IsEmpty() ? TEXT("Owned character") : Identity.DisplayNameKey;
            const FString CopyName = Copy.BuildLabel.IsEmpty() ? FString::Printf(TEXT("Owned copy %d"), I + 1) :
                Copy.BuildLabel + FString::Printf(TEXT(" / owned copy %d"), I + 1);
            return Name + TEXT(" / ") + CopyName;
        }
    return TEXT("Owned character");
}
FString InventoryItemLabel(const FOGOwnedInventorySnapshot& Snapshot, const FOGOwnedInventoryItem& Row)
{
    int32 Count = 0, Ordinal = 0;
    for (const auto& Item : Snapshot.Items)
        if (Item.Item.DefinitionId == Row.Item.DefinitionId)
        {
            ++Count;
            if (Item.Item.ItemId == Row.Item.ItemId) Ordinal = Count;
        }
    return Row.DisplayName + (Count > 1 ? FString::Printf(TEXT(" / owned item %d"), Ordinal) : FString());
}
}

void UOGGate12NativeRulerWidget::ClearInventoryReview()
{
    PendingObjectReview=FOGObjectActionPreview();
    PendingObjectRequestId=FOGEntityId();
    PendingEquipmentReview = FOGOwnedEquipmentPreview();
    PendingEquipmentRequestId = FOGEntityId();
}

void UOGGate12NativeRulerWidget::ResetInventoryUi()
{
    bInventoryOpen = false;
    ObjectActionCounterpartyId=FOGEntityId();
    bInventoryTargetPicker = false;
    InventorySelectedItemId = FOGEntityId();
    InventoryContextRulerId = FOGEntityId();
    InventoryContextIdentityId = FOGContentId();
    InventoryContextManifestationId = FOGEntityId();
    ClearInventoryReview();
    InventoryFeedback.Reset();
    InventoryRefreshWarning.Reset();
    bInventoryFeedbackError = false;
}

bool UOGGate12NativeRulerWidget::SynchronizeInventoryContext(
    const UOGGate12RulerSessionSubsystem* Session)
{
    if (!bInventoryOpen) return false;
    if (!Session || !Session->IsBoundToStoredRuler() ||
        Session->CurrentDestination != EOGRulerPrimaryDestination::Home)
    {
        ResetInventoryUi();
        return true;
    }
    if (InventoryContextRulerId == Session->BoundRulerId &&
        InventoryContextIdentityId == Session->SelectedIdentityId &&
        InventoryContextManifestationId == Session->SelectedManifestationId) return false;
    if (InventoryContextRulerId!=Session->BoundRulerId) ObjectActionCounterpartyId=FOGEntityId();
    InventoryContextRulerId = Session->BoundRulerId;
    InventoryContextIdentityId = Session->SelectedIdentityId;
    InventoryContextManifestationId = Session->SelectedManifestationId;
    InventorySelectedItemId = FOGEntityId();
    ClearInventoryReview();
    InventoryFeedback.Reset();
    InventoryRefreshWarning.Reset();
    bInventoryFeedbackError = false;
    return true;
}

void UOGGate12NativeRulerWidget::BuildEquipmentReview(
    UVerticalBox* Parent, const FOGOwnedEquipmentPreview& Review)
{
    AddMessage(Parent, Review.bUnequip ? TEXT("REVIEW UNEQUIP") : TEXT("REVIEW EQUIP"), 24, InventorySilver);
    auto* GI = GetGameInstance();
    auto* Session = GI ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    AddMessage(Parent, InventoryCopyLabel(Session, Review.Target.ManifestationId), 17, InventoryText);
    FString ItemLabel = Review.ItemName;
    FOGOwnedInventorySnapshot Snapshot;
    FString ReadError;
    if (Session && Session->ReadOwnedInventory(Snapshot, ReadError))
        if (const auto* Row = Snapshot.Items.FindByPredicate([&](const auto& I) { return I.Item.ItemId == Review.Item.ItemId; }))
            ItemLabel = InventoryItemLabel(Snapshot, *Row);
    AddMessage(Parent, Review.SlotName + TEXT(" / ") + ItemLabel, 20, InventoryText);
    if (Review.bUnequip)
        AddMessage(Parent, TEXT("Remove this binding. The owned item stays in inventory."), 17, InventoryText);
    else if (Review.bHasPreviousItem)
        AddMessage(Parent, TEXT("Replaces ") + Review.PreviousItemName +
            TEXT(". The previous item stays owned and becomes unequipped."), 17, InventoryWarning);
    else AddMessage(Parent, TEXT("Equip this owned item in the empty authored slot."), 17, InventoryText);

    AddMessage(Parent, TEXT("EQUIPMENT STATS / BEFORE → AFTER"), 18, InventorySilver);
    if (Review.bStatsKnown)
    {
        const auto AddStat = [&](const FString& Name, const FOGLargeNumber& Before, const FOGLargeNumber& After)
        {
            AddMessage(Parent, Name + TEXT(": ") + FOGUiNumberFormatter::Format(Before, true, false) +
                TEXT(" → ") + FOGUiNumberFormatter::Format(After, true, false), 17, InventoryText);
        };
        AddStat(TEXT("Maximum HP"), Review.BeforeStats.MaxHp, Review.AfterStats.MaxHp);
        AddStat(TEXT("Attack"), Review.BeforeStats.Attack, Review.AfterStats.Attack);
        AddStat(TEXT("Defense"), Review.BeforeStats.Defense, Review.AfterStats.Defense);
        AddStat(TEXT("Speed"), FOGLargeNumber::FromInt64(Review.BeforeStats.Speed), FOGLargeNumber::FromInt64(Review.AfterStats.Speed));
        AddMessage(Parent, TEXT("Includes all resolved equipment and earned development. Current HP is preserved."), 15, InventoryMuted);
    }
    else AddMessage(Parent, Review.StatsAvailability.IsEmpty() ?
        TEXT("Total stats need this owned copy's existing resolved projection.") : Review.StatsAvailability, 16, InventoryMuted);
    AddMessage(Parent, TEXT("Confirm rechecks ownership, the current slot and installed rules. It does not heal the character."),
        16, InventoryMuted);
    AddAction(Parent, Review.bUnequip ? TEXT("CONFIRM UNEQUIP") : TEXT("CONFIRM EQUIP"),
        TEXT("inventory/confirm/") + PendingEquipmentRequestId.ToString());
    AddAction(Parent, TEXT("CANCEL"), TEXT("inventory/cancel"));
}

void UOGGate12NativeRulerWidget::BuildInventory()
{
    if (!Content || !WidgetTree) return;
    UGameInstance* GI = GetGameInstance();
    UOGGate12RulerSessionSubsystem* Session = GI
        ? GI->GetSubsystem<UOGGate12RulerSessionSubsystem>() : nullptr;
    AddAction(Content, TEXT("BACK TO HOME"), TEXT("inventory/close"));
    UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Surface->SetBrushColor(FLinearColor(.010f, .010f, .014f, .97f));
    Surface->SetPadding(FMargin(12.f, 18.f));
    UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Surface->AddChild(Body);
    UOverlaySlot* Background = Layers->AddChildToOverlay(Surface);
    Background->SetHorizontalAlignment(HAlign_Fill);
    Background->SetVerticalAlignment(VAlign_Fill);
    auto* Chrome = WidgetTree->ConstructWidget<UOGGate12GothicChromeWidget>(UOGGate12GothicChromeWidget::StaticClass());
    Chrome->Configure(InventorySilver, INDEX_NONE, false);
    Chrome->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* Ornament = Layers->AddChildToOverlay(Chrome);
    Ornament->SetHorizontalAlignment(HAlign_Fill);
    Ornament->SetVerticalAlignment(VAlign_Fill);
    Content->AddChildToVerticalBox(Layers)->SetPadding(FMargin(4.f, 8.f));
    AddMessage(Body, TEXT("OWNED INVENTORY"), 25, InventorySilver);
    if (!InventoryFeedback.IsEmpty())
        AddMessage(Body, InventoryFeedback, 17, bInventoryFeedbackError ? InventoryWarning : InventorySuccess);
    if (!InventoryRefreshWarning.IsEmpty()) AddMessage(Body, InventoryRefreshWarning, 16, InventoryWarning);
    if (!Session) return;
    FOGOwnedInventorySnapshot Snapshot;
    FString Error;
    if (!Session->ReadOwnedInventory(Snapshot, Error))
    {
        ClearInventoryReview();
        AddMessage(Body, TEXT("Inventory unavailable: ") + Error, 17, InventoryWarning);
        return;
    }
    if (!Snapshot.Availability.IsEmpty()) AddMessage(Body, Snapshot.Availability, 17, InventoryMuted);
    AddMessage(Body, TEXT("EQUIPMENT TARGET"), 20, InventorySilver);
    if (Snapshot.bHasTarget)
    {
        AddMessage(Body, InventoryCopyLabel(Session, Snapshot.Target.ManifestationId), 18, InventoryText);
    }
    else AddMessage(Body, TEXT("No owned Manifestation selected. Inventory can still be browsed."), 17, InventoryMuted);
    if (!Session->OwnedRoster.IsEmpty())
        AddAction(Body, bInventoryTargetPicker ? TEXT("CLOSE CHARACTER PICKER") : TEXT("CHOOSE OWNED CHARACTER"),
            TEXT("inventory/targets"));
    if (bInventoryTargetPicker)
    {
        for (int32 I = 0; I < Session->OwnedRoster.Num(); ++I)
        {
            const auto& Identity = Session->OwnedRoster[I];
            for (int32 J = 0; J < Identity.Manifestations.Num(); ++J)
            {
                const auto& Copy = Identity.Manifestations[J];
                if (Copy.bLocked) continue;
                AddAction(Body, InventoryCopyLabel(Session, Copy.ManifestationId) +
                    FString::Printf(TEXT(" / level %d"), Copy.Level),
                    FString::Printf(TEXT("inventory/target/%d/%d"), I, J));
            }
        }
    }

    if (!PendingEquipmentReview.ConfirmationToken.IsEmpty())
    {
        FOGOwnedEquipmentPreview Fresh;
        if (Session->ReviewOwnedEquipment(PendingEquipmentReview.SlotId,
                PendingEquipmentReview.Item.ItemId, PendingEquipmentReview.bUnequip, Fresh, Error) &&
            Fresh.bCanApply && Fresh.ConfirmationToken == PendingEquipmentReview.ConfirmationToken &&
            (!PendingEquipmentReview.bStatsKnown || (Fresh.bStatsKnown && Fresh.StatPreviewToken == PendingEquipmentReview.StatPreviewToken)))
        {
            PendingEquipmentReview = MoveTemp(Fresh);
            BuildEquipmentReview(Body, PendingEquipmentReview);
            return;
        }
        ClearInventoryReview();
        AddMessage(Body, TEXT("Equipment details changed. Review the current item and slot again."), 17, InventoryWarning);
    }

    BuildObjectActions(Body,*Session);
    if (!PendingObjectReview.ConfirmationToken.IsEmpty()) return;
    AddMessage(Body, TEXT("EQUIPMENT SLOTS"), 20, InventorySilver);
    if (!Snapshot.bHasTarget) AddMessage(Body, TEXT("Choose a character to inspect its slots."), 16, InventoryMuted);
    for (const auto& Slot : Snapshot.Slots)
    {
        const auto* Equipped = Snapshot.Items.FindByPredicate([&](const auto& I)
        { return I.Item.ItemId == Slot.Binding.ItemId; });
        AddMessage(Body, Slot.DisplayName + TEXT(" / ") + (Slot.bOccupied ?
            (Equipped ? InventoryItemLabel(Snapshot, *Equipped) : FString(TEXT("Stored item unavailable"))) : FString(TEXT("Empty"))),
            18, InventoryText);
        if (!Slot.bAuthored)
            AddMessage(Body, TEXT("Stored binding; no approved slot rule is installed for new equipment."), 15, InventoryMuted);
        if (Slot.bOccupied)
        {
            FOGOwnedEquipmentPreview Removal;
            if (Session->ReviewOwnedEquipment(Slot.SlotId, Slot.Binding.ItemId, true, Removal, Error) && Removal.bCanApply)
                AddAction(Body, TEXT("REVIEW UNEQUIP"), TEXT("inventory/unequip/") + Slot.SlotId.ToString());
            else if (!Error.IsEmpty()) AddMessage(Body, Error, 15, InventoryMuted);
        }
    }
    AddMessage(Body, FString::Printf(TEXT("OWNED ITEMS / %d"), Snapshot.Items.Num()), 20, InventorySilver);
    if (Snapshot.Items.IsEmpty())
        AddMessage(Body, TEXT("No owned item instances are recorded for this Ruler or its Manifestations."), 17, InventoryMuted);
    const auto* Selected = Snapshot.Items.FindByPredicate([&](const auto& I) { return I.Item.ItemId == InventorySelectedItemId; });
    if (InventorySelectedItemId.IsValid() && !Selected) InventorySelectedItemId = FOGEntityId();
    for (const auto& Item : Snapshot.Items)
        AddAction(Body, InventoryItemLabel(Snapshot, Item) + (Item.Bindings.IsEmpty() ? TEXT("") : TEXT(" [EQUIPPED]")),
            TEXT("inventory/item/") + Item.Item.ItemId.ToString());
    if (!Selected) return;
    AddMessage(Body, TEXT("OWNED ITEM"), 21, InventorySilver);
    AddMessage(Body, InventoryItemLabel(Snapshot, *Selected), 20, InventoryText);
    AddMessage(Body, TEXT("Owner: ") + InventoryCopyLabel(Session, Selected->Item.OwnerEntityId), 16, InventoryMuted);
    if (!Selected->Modifiers.IsEmpty())
        AddMessage(Body, FString::Printf(TEXT("Stored modifiers: %d"), Selected->Modifiers.Num()), 15, InventoryMuted);
    for (const auto& Binding : Selected->Bindings)
    {
        const auto* Slot = Snapshot.Slots.FindByPredicate([&](const auto& S) { return S.SlotId == Binding.SlotId; });
        AddMessage(Body, TEXT("Equipped by ") + InventoryCopyLabel(Session, Binding.WearerEntityId) +
            TEXT(" / ") + (Slot ? Slot->DisplayName : FString(TEXT("Stored equipment slot"))), 15, InventoryMuted);
    }
    for (const auto& Slot : Snapshot.Slots)
    {
        if (!Slot.bAuthored) continue;
        FOGOwnedEquipmentPreview Option;
        if (Session->ReviewOwnedEquipment(Slot.SlotId, Selected->Item.ItemId, false, Option, Error))
        {
            if (Option.bCanApply)
                AddAction(Body, TEXT("REVIEW EQUIP / ") + Slot.DisplayName,
                    TEXT("inventory/equip/") + Slot.SlotId.ToString() + TEXT("|") + Selected->Item.ItemId.ToString());
            else AddMessage(Body, Slot.DisplayName + TEXT(": ") + FString::Join(Option.MissingRequirements, TEXT(" ")),
                15, InventoryMuted);
        }
        else if (!Error.IsEmpty()) AddMessage(Body, Slot.DisplayName + TEXT(": ") + Error, 15, InventoryMuted);
    }
}

void UOGGate12NativeRulerWidget::HandleInventoryAction(
    const FString& Action, UOGGate12RulerSessionSubsystem& Session)
{
    // Old UMG buttons may still have queued callbacks after the page rebuild.
    // Bind confirms to one review; ignore stale taps before clearing feedback.
    if (bInventoryCommandInFlight) return;
    const bool EquipmentConfirm = Action.StartsWith(TEXT("inventory/confirm/"));
    const bool ObjectConfirm = Action.StartsWith(TEXT("inventory/object/confirm/"));
    if (Action == TEXT("inventory/confirm") || Action == TEXT("inventory/object/confirm")) return;
    if (EquipmentConfirm || ObjectConfirm)
    {
        const auto& Request = EquipmentConfirm ? PendingEquipmentRequestId : PendingObjectRequestId;
        const FString Prefix = EquipmentConfirm ? TEXT("inventory/confirm/") : TEXT("inventory/object/confirm/");
        if (!Request.IsValid() || Action != Prefix + Request.ToString()) return;
        if (SynchronizeInventoryContext(&Session)) { BuildCurrentPage(); return; }
    }
    TGuardValue<bool> InFlight(bInventoryCommandInFlight, true);
    LastUiError.Reset();
    InventoryFeedback.Reset();
    InventoryRefreshWarning.Reset();
    bInventoryFeedbackError = false;
    FString Error;
    if (Action == TEXT("inventory/open"))
    {
        ResetInventoryUi();
        if (Session.Navigate(EOGRulerPrimaryDestination::Home, Error)) bInventoryOpen = true;
        SynchronizeInventoryContext(&Session);
    }
    else if (Action == TEXT("inventory/close")) ResetInventoryUi();
    else if (!bInventoryOpen || !Session.IsBoundToStoredRuler())
        Error = TEXT("Open this Ruler's inventory before reviewing equipment.");
    else if (Action == TEXT("inventory/targets"))
    {
        ClearInventoryReview();
        bInventoryTargetPicker = !bInventoryTargetPicker;
    }
    else if (Action.StartsWith(TEXT("inventory/target/")))
    {
        ClearInventoryReview();
        FString IdentityIndex, CopyIndex;
        int32 I = INDEX_NONE, J = INDEX_NONE;
        if (Action.Mid(17).Split(TEXT("/"), &IdentityIndex, &CopyIndex) &&
            LexTryParseString(I, *IdentityIndex) && LexTryParseString(J, *CopyIndex) &&
            Session.OwnedRoster.IsValidIndex(I) && Session.OwnedRoster[I].Manifestations.IsValidIndex(J))
        {
            const FOGContentId Identity = Session.OwnedRoster[I].IdentityId;
            const FOGEntityId Copy = Session.OwnedRoster[I].Manifestations[J].ManifestationId;
            if (Session.SelectOwnedManifestation(Identity, Copy, Error))
            {
                Session.Navigate(EOGRulerPrimaryDestination::Home, Error);
                bInventoryTargetPicker = false;
                SynchronizeInventoryContext(&Session);
            }
        }
        else Error = TEXT("The owned character picker changed. Open it again.");
    }
    else if (Action.StartsWith(TEXT("inventory/item/")))
    {
        ClearInventoryReview();
        FGuid Id;
        FOGOwnedInventorySnapshot Snapshot;
        if (FGuid::Parse(Action.Mid(15), Id) && Session.ReadOwnedInventory(Snapshot, Error) &&
            Snapshot.Items.ContainsByPredicate([&](const auto& I) { return I.Item.ItemId == FOGEntityId(Id); }))
            InventorySelectedItemId = FOGEntityId(Id);
        else if (Error.IsEmpty()) Error = TEXT("This item instance is no longer in owned inventory.");
    }
    else if (Action == TEXT("inventory/cancel")) ClearInventoryReview();
    else if (Action == TEXT("inventory/object/leave"))
    { ClearInventoryReview(); ObjectActionCounterpartyId=FOGEntityId(); }
    else if (ObjectConfirm)
    {
        const auto Review=PendingObjectReview; const auto Request=PendingObjectRequestId; ClearInventoryReview();
        if (Review.ConfirmationToken.IsEmpty() || !Request.IsValid())
            Error=TEXT("Review an action before confirming.");
        else if (Session.ConfirmOwnedObjectAction(Review,Request,Error))
        {
            InventoryRefreshWarning=Error; Error.Reset();
            InventoryFeedback=Review.DisplayName.ToString()+TEXT(" completed.");
        }
    }
    else if (Action.StartsWith(TEXT("inventory/object/review/")))
    {
        ClearInventoryReview(); FOGObjectActionPreview Review;
        const FOGContentId Id(Action.Mid(24));
        if (Session.ReviewOwnedObjectAction(Id,InventorySelectedItemId,ObjectActionCounterpartyId,Review,Error))
        {
            if (!Review.bCanApply) Error=FString::Join(Review.MissingRequirements,TEXT(" "));
            else { PendingObjectReview=MoveTemp(Review); PendingObjectRequestId=FOGEntityId::NewId(); }
        }
    }
    else if (Action.StartsWith(TEXT("inventory/equip/")) || Action.StartsWith(TEXT("inventory/unequip/")))
    {
        ClearInventoryReview();
        const bool bUnequip = Action.StartsWith(TEXT("inventory/unequip/"));
        const FString Payload = Action.Mid(bUnequip ? 18 : 16);
        FString Slot = Payload, Item;
        FOGEntityId ItemId;
        FGuid Guid;
        if (!bUnequip && (!Payload.Split(TEXT("|"), &Slot, &Item) || !FGuid::Parse(Item, Guid)))
            Error = TEXT("This equipment choice is invalid. Review an owned item again.");
        else
        {
            if (!bUnequip) ItemId = FOGEntityId(Guid);
            FOGOwnedEquipmentPreview Review;
            if (Session.ReviewOwnedEquipment(FOGContentId(Slot), ItemId, bUnequip, Review, Error))
            {
                if (!Review.bCanApply) Error = FString::Join(Review.MissingRequirements, TEXT(" "));
                else
                {
                    PendingEquipmentReview = MoveTemp(Review);
                    PendingEquipmentRequestId = FOGEntityId::NewId();
                }
            }
        }
    }
    else if (EquipmentConfirm)
    {
        const FOGOwnedEquipmentPreview Review = PendingEquipmentReview;
        const FOGEntityId Request = PendingEquipmentRequestId;
        ClearInventoryReview();
        if (Review.ConfirmationToken.IsEmpty() || !Request.IsValid())
            Error = TEXT("Review an equipment change before confirming.");
        else if (Session.ConfirmOwnedEquipment(Review, Request, Error))
        {
            InventoryRefreshWarning = Error;
            Error.Reset();
            InventoryFeedback = Review.bUnequip ? TEXT("Unequip saved. The item remains owned.") :
                TEXT("Equipment change saved for the selected owned copy.");
        }
    }
    else Error = TEXT("Unrecognized inventory action.");
    if (!Error.IsEmpty())
    {
        ClearInventoryReview();
        InventoryFeedback = Error;
        bInventoryFeedbackError = true;
        if (!bInventoryOpen) LastUiError = Error;
    }
    BuildCurrentPage();
}

bool UOGGate12NativeRulerWidget::OpenObjectActionsForCounterparty(const FOGEntityId& Counterparty,FString& Error)
{
    auto* GI=GetGameInstance(); auto* Session=GI?GI->GetSubsystem<UOGGate12RulerSessionSubsystem>():nullptr;
    if (!Session || !Counterparty.IsValid()) { Error=TEXT("This counterparty is unavailable."); return false; }
    TArray<FOGObjectActionPreview> Actions;
    if (!Session->ReadOwnedObjectActions(FOGEntityId(),Counterparty,Actions,Error)) return false;
    if (Actions.IsEmpty()) { Error=TEXT("No offers are available."); return false; }
    if (!Session->ReturnToRuler(Error) || !Session->Navigate(EOGRulerPrimaryDestination::Home,Error)) return false;
    ResetInventoryUi(); bInventoryOpen=true; SynchronizeInventoryContext(Session);
    ObjectActionCounterpartyId=Counterparty; BuildCurrentPage(); return true;
}
void UOGGate12NativeRulerWidget::BuildObjectActions(UVerticalBox* Parent,UOGGate12RulerSessionSubsystem& Session)
{
    AddMessage(Parent,ObjectActionCounterpartyId.IsValid()?TEXT("AVAILABLE OFFERS"):TEXT("CRAFT / USE / REPAIR"),20,InventorySilver);
    if (ObjectActionCounterpartyId.IsValid()) AddAction(Parent,TEXT("LEAVE TRADE"),TEXT("inventory/object/leave"));
    FString Error;
    if (!PendingObjectReview.ConfirmationToken.IsEmpty())
    {
        FOGObjectActionPreview Fresh;
        if (!Session.ReviewOwnedObjectAction(PendingObjectReview.ActionId,InventorySelectedItemId,ObjectActionCounterpartyId,Fresh,Error) ||
            !Fresh.bCanApply || Fresh.ConfirmationToken!=PendingObjectReview.ConfirmationToken)
        { ClearInventoryReview(); AddMessage(Parent,TEXT("The action changed. Review it again."),17,InventoryWarning); }
        else
        {
            AddMessage(Parent,Fresh.DisplayName.ToString(),22,InventorySilver);
            if (!Fresh.Description.IsEmpty()) AddMessage(Parent,Fresh.Description.ToString(),17,InventoryText);
            for (const auto& Input:Fresh.Inputs)
            {
                const FString Verb=Input.Disposition==EOGObjectInputDisposition::Consume?TEXT("Use: "):
                    Input.Disposition==EOGObjectInputDisposition::Transfer?(Input.NewOwnerId==Fresh.Context.RulerId?TEXT("Receive: "):TEXT("Give: ")):TEXT("Required tool: ");
                AddMessage(Parent,Verb+Input.DisplayName.ToString(),16,InventoryText);
            }
            for (const auto& Wallet:Fresh.Wallets)
                AddMessage(Parent,(Wallet.OwnerId==Fresh.Context.RulerId?TEXT("You / "):TEXT("Counterparty / "))+Wallet.DisplayName.ToString()+TEXT(": ")+LexToString(Wallet.Before)+TEXT(" → ")+LexToString(Wallet.After),
                    16,Wallet.After<Wallet.Before?InventoryWarning:InventorySuccess);
            auto* Library=UOGDefinitionLibrary::Get();
            for (const auto& Output:Fresh.Outputs)
            {
                const auto* Definition=Library?Cast<UOGObjectDefinition>(Library->Resolve(TEXT("object"),Output.ObjectId)):nullptr;
                AddMessage(Parent,(Output.To==EOGObjectActionParty::Ruler?TEXT("Receive: "):TEXT("Give: "))+(Definition?Definition->DisplayName.ToString():TEXT("Item"))+
                    FString::Printf(TEXT(" x%d"),Output.InstanceCount),17,InventorySuccess);
            }
            if (Fresh.bRepairsSubject)
                AddMessage(Parent,FString::Printf(TEXT("Condition %s%.2f%% → %.2f%%"),
                    Fresh.bSubjectWasBroken?TEXT("Broken / "):TEXT(""),
                    Fresh.RepairBeforeConditionBps/100.0,Fresh.RepairAfterConditionBps/100.0),17,InventoryText);
            AddAction(Parent,TEXT("CONFIRM"),TEXT("inventory/object/confirm/")+PendingObjectRequestId.ToString());
            AddAction(Parent,TEXT("CANCEL"),TEXT("inventory/cancel")); return;
        }
    }
    TArray<FOGObjectActionPreview> Actions;
    if (!Session.ReadOwnedObjectActions(InventorySelectedItemId,ObjectActionCounterpartyId,Actions,Error))
    { AddMessage(Parent,Error,16,InventoryWarning); return; }
    if (Actions.IsEmpty()) AddMessage(Parent,TEXT("No actions are available for the current selection."),16,InventoryMuted);
    for (const auto& Action:Actions)
    {
        if (Action.bCanApply) AddAction(Parent,Action.DisplayName.ToString(),TEXT("inventory/object/review/")+Action.ActionId.ToString());
        else AddMessage(Parent,Action.DisplayName.ToString()+TEXT(" — ")+FString::Join(Action.MissingRequirements,TEXT(" ")),16,InventoryMuted);
    }
}
