#include "UI/OGGate12NativeRulerWidget.h"
#include "Runtime/OGGate12RulerSessionSubsystem.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOGInventoryQueuedConfirmationTest,
    "OfflineGame.Inventory.QueuedAndReentrantConfirmsPreserveReviewAndSuccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOGInventoryQueuedConfirmationTest::RunTest(const FString&)
{
    auto* Game = NewObject<UGameInstance>();
    auto* Session = NewObject<UOGGate12RulerSessionSubsystem>(Game);
    auto* Widget = NewObject<UOGGate12NativeRulerWidget>(Game);
    const auto OldEquipment = FOGEntityId::NewId();
    const auto OldObject = FOGEntityId::NewId();
    const auto CurrentEquipment = FOGEntityId::NewId();
    const auto CurrentObject = FOGEntityId::NewId();
    Widget->PendingEquipmentRequestId = CurrentEquipment;
    Widget->PendingObjectRequestId = CurrentObject;
    Widget->PendingEquipmentReview.ConfirmationToken = TEXT("current-equipment-review");
    Widget->PendingObjectReview.ConfirmationToken = TEXT("current-object-review");
    Widget->InventoryFeedback = TEXT("Equipment change saved.");
    Widget->InventoryRefreshWarning = TEXT("Saved; a view still needs refresh.");
    Widget->HandleInventoryAction(TEXT("inventory/confirm/") + OldEquipment.ToString(), *Session);
    Widget->HandleInventoryAction(TEXT("inventory/object/confirm/") + OldObject.ToString(), *Session);
    TestTrue(TEXT("Old queued confirms cannot consume a later equipment request"),
        Widget->PendingEquipmentRequestId == CurrentEquipment);
    TestTrue(TEXT("Old queued confirms cannot consume a later object request"),
        Widget->PendingObjectRequestId == CurrentObject);
    TestEqual(TEXT("Old tap does not replace success"), Widget->InventoryFeedback, FString(TEXT("Equipment change saved.")));
    TestEqual(TEXT("Old tap preserves the post-commit warning"), Widget->InventoryRefreshWarning,
        FString(TEXT("Saved; a view still needs refresh.")));
    Widget->bInventoryCommandInFlight = true;
    Widget->HandleInventoryAction(TEXT("inventory/confirm/") + CurrentEquipment.ToString(), *Session);
    Widget->HandleInventoryAction(TEXT("inventory/object/confirm/") + CurrentObject.ToString(), *Session);
    Widget->HandleInventoryAction(TEXT("inventory/cancel"), *Session);
    TestTrue(TEXT("Reentrant callbacks cannot replace the active review"), Widget->PendingEquipmentRequestId == CurrentEquipment &&
        Widget->PendingObjectRequestId == CurrentObject);
    Widget->bInventoryCommandInFlight = false;
    Widget->ClearInventoryReview(); // Same state as after a completed command.
    Widget->HandleInventoryAction(TEXT("inventory/confirm/") + CurrentEquipment.ToString(), *Session);
    Widget->HandleInventoryAction(TEXT("inventory/object/confirm/") + CurrentObject.ToString(), *Session);
    Widget->HandleInventoryAction(TEXT("inventory/confirm"), *Session);
    Widget->HandleInventoryAction(TEXT("inventory/object/confirm"), *Session);
    TestEqual(TEXT("Duplicate and legacy callbacks preserve committed success"), Widget->InventoryFeedback,
        FString(TEXT("Equipment change saved.")));
    TestFalse(TEXT("Ignored callbacks cannot change success into an error"), Widget->bInventoryFeedbackError);
    Session->bInventoryCommandInFlight = true;
    FString Error;
    TestFalse(TEXT("Session blocks nested equipment execution before resolving or writing store"),
        Session->ConfirmOwnedEquipment(FOGOwnedEquipmentPreview(), FOGEntityId::NewId(), Error));
    TestTrue(TEXT("Equipment reports active confirmation"), Error.Contains(TEXT("already in progress")));
    TestFalse(TEXT("Session blocks nested object execution before resolving or writing store"),
        Session->ConfirmOwnedObjectAction(FOGObjectActionPreview(), FOGEntityId::NewId(), Error));
    TestTrue(TEXT("Object action reports active confirmation"), Error.Contains(TEXT("already in progress")));
    return true;
}
#endif
