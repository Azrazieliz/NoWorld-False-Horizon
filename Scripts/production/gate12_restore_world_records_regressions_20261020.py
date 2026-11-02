from pathlib import Path
P=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
def apply(path, pairs):
    f=P/path
    a=f.read_text(encoding="utf-8-sig")
    b=a
    for before,after in pairs:
        count=b.count(before)
        if count!=1:
            raise RuntimeError(f"{path}: expected 1 match, got {count} for {before[:80]!r}")
        b=b.replace(before,after,1)
    if b!=a:
        f.write_text(b,encoding="utf-8",newline="")
        print("CHANGED",path)
base=Path("Source/OfflineGame")
apply(base/"Public/UI/OGGate12NativeRulerWidget.h",[
("    int32 RecordsSection = 0;","    // -1 is the four-destination Records hub; selecting a card opens a full page.\n    int32 RecordsSection = -1;"),
])
apply(base/"Private/UI/OGGate12NativeRulerWidget.cpp",[
("""    Content->AddChildToVerticalBox(Board);
    for (int32 Index=0; Index<4; ++Index)
    {
        const bool bActive = RecordsSection == Index;""",
"""    // The four cards are a navigation HUB, not persistent tabs above a
    // miniature detail panel. Each selected section owns the whole page.
    if (RecordsSection < 0)
    {
        Content->AddChildToVerticalBox(Board);
    for (int32 Index=0; Index<4; ++Index)
    {
        const bool bActive = false;"""),
("""        RecordCell->SetVerticalAlignment(VAlign_Fill);
    }
    UBorder* DetailFrame = WidgetTree->ConstructWidget<UBorder>(""",
"""        RecordCell->SetVerticalAlignment(VAlign_Fill);
    }
        return;
    }

    AddAction(Content, TEXT("BACK TO RECORDS"), TEXT("records/back"));
    UBorder* DetailFrame = WidgetTree->ConstructWidget<UBorder>("""),
("""    DetailHeight->SetHeightOverride(380.f);""","""    // Long Reports/Chronicle/Intelligence histories are independent scrolling
    // pages, not clipped to the 380px footer of a four-card overview.
    DetailHeight->SetMinDesiredHeight(980.f);"""),
("""    else if (Action.StartsWith(TEXT("records/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(8));
        if (Index < 0 || Index >= 4) return;
        RecordsSection = Index;
    }""",
"""    else if (Action == TEXT("records/back"))
    {
        RecordsSection = -1;
    }
    else if (Action.StartsWith(TEXT("records/")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(8));
        if (Index < 0 || Index >= 4) return;
        RecordsSection = Index;
    }"""),
("""        OpenIdentityIndex = INDEX_NONE;
        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);""",
"""        OpenIdentityIndex = INDEX_NONE;
        RecordsSection = -1;
        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);"""),
])
apply(base/"Private/World/OGFoundationIntegrationWorld.cpp",[
("""    if (!Party || !Party->GetSlots().IsValidIndex(Party->GetControlledSlot()) ||
        Party->GetSlots()[Party->GetControlledSlot()].Unit.UnitEntityId != FlightOwnerId ||
        !Pawn->GetTraversalCapabilities()->HasCapability(FOGTraversalCapabilityIds::Flight))
    {
        SetFlight(Pawn, false);
    }""",
"""    // The Ruler can control the baseline World Pawn without owning any
    // Manifestation/party slot. In that legitimate state FlightOwnerId is
    // intentionally empty. Never revoke the station's temporary grant
    // immediately just because the empty roster has no party slot.
    const bool bCapturedPartyOwnerChanged = FlightOwnerId.IsValid() &&
        (!Party ||
         !Party->GetSlots().IsValidIndex(Party->GetControlledSlot()) ||
         Party->GetSlots()[Party->GetControlledSlot()].Unit.UnitEntityId != FlightOwnerId);
    if (bCapturedPartyOwnerChanged ||
        !Pawn->GetTraversalCapabilities() ||
        !Pawn->GetTraversalCapabilities()->HasCapability(
            FOGTraversalCapabilityIds::Flight))
    {
        SetFlight(Pawn, false);
    }""")
])
print("NO_BUILD_PERFORMED")
