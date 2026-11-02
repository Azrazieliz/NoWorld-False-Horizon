from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
f=root/"Source/OfflineGame/Private/UI/OGGate12NativeRulerWidget.cpp"
s=f.read_text(encoding="utf-8-sig")
def repl(a,b):
 global s
 n=s.count(a)
 if n!=1: raise RuntimeError(f"Expected 1 got {n} {a[:100]!r}")
 s=s.replace(a,b,1)
repl('''    if (!Session || !Session->IsBoundToStoredRuler()) return;

    UImage* Chrome =''','''    if (!Session || !Session->IsBoundToStoredRuler()) return;

    AOGWorldPresentationHud* Hud = GetOwningPlayer()
        ? Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()) : nullptr;
    const bool bCanSummon = Hud && Hud->HasActiveGachaPresentation() &&
        Session->bRealGachaAccessUnlocked;

    UImage* Chrome =''')
repl('''    ActionAt(TEXT("RATES"),TEXT("gacha-rates"),686.f,132.f,70.f,88.f,false,18);''','''    ActionAt(TEXT("RATES"),TEXT("gacha-rates"),686.f,132.f,70.f,88.f,bCanSummon,18);''')
repl('''    if (!Session->bApprovedBannerProvided)
        TextAt(TEXT("NO ACTIVE BANNER"),
            TEXT("AuthoritativeNoActiveBanner"),155.f,830.f,555.f,96.f,48,Main);
    else
        TextAt(TEXT("APPROVED BANNER AVAILABLE"),
            TEXT("AuthoritativeBannerAvailable"),143.f,835.f,574.f,94.f,38,Main);''','''    if (!Hud || !Hud->HasActiveGachaPresentation())
        TextAt(TEXT("NO ACTIVE BANNER"),
            TEXT("AuthoritativeNoActiveBanner"),155.f,830.f,555.f,96.f,48,Main);
    else
        TextAt(Hud->GetActiveGachaBanner().BannerId.ToString(),
            TEXT("AuthoritativeInstalledBannerId"),108.f,812.f,640.f,118.f,35,Main);''')
repl('''    ActionAt(TEXT("NO BANNER"),TEXT("gacha/banner/0"),''','''    ActionAt(bCanSummon ? TEXT("ACTIVE") : TEXT("NO BANNER"),TEXT("gacha/banner/0"),''')
repl('''    ActionAt(TEXT("DRAW ×1"),TEXT("gacha-draw-1"),
             50.f,1170.f,309.f,100.f,false,37);
    ActionAt(TEXT("DRAW ×10"),TEXT("gacha-draw-10"),
             511.f,1170.f,311.f,100.f,false,37);
    ActionAt(TEXT("DETAILS"),TEXT("gacha-details"),
             151.f,1289.f,223.f,50.f,false,24);''','''    ActionAt(TEXT("DRAW ×1"),TEXT("gacha-draw-1"),
             50.f,1170.f,309.f,100.f,bCanSummon,37);
    ActionAt(TEXT("DRAW ×10"),TEXT("gacha-draw-10"),
             511.f,1170.f,311.f,100.f,bCanSummon,37);
    ActionAt(TEXT("DETAILS"),TEXT("gacha-rates"),
             151.f,1289.f,223.f,50.f,bCanSummon,24);''')
hook='''    if (!bGachaHistoryExpanded) return;
    // This panel is the existing canonical acquisition history projection.'''
addition='''    if (bGachaResultsExpanded || bGachaRatesExpanded)
    {
        const bool bShowRates = bGachaRatesExpanded;
        UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),TEXT("CanonicalGachaResultOrRatesPanel"));
        Panel->SetBrushColor(FLinearColor(.008f,.005f,.016f,.985f));
        Panel->SetPadding(FMargin(18.f));
        UCanvasPanelSlot* Slot=Gate10PortraitGachaCanvas->AddChildToCanvas(Panel);
        Slot->SetAnchors(FAnchors(100.f/864.f,250.f/1536.f,
                                 765.f/864.f,1020.f/1536.f));
        Slot->SetOffsets(FMargin(0.f));
        UScrollBox* Scroll=WidgetTree->ConstructWidget<UScrollBox>(
            UScrollBox::StaticClass());
        Panel->AddChild(Scroll);
        UVerticalBox* Rows=WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass());
        Scroll->AddChild(Rows);
        if (bShowRates && Hud && Hud->HasActiveGachaPresentation())
        {
            const FOGGachaBannerDefinition& Banner=Hud->GetActiveGachaBanner();
            AddMessage(Rows,TEXT("AUTHORITATIVE BANNER RATES"),26,Main);
            AddMessage(Rows,Banner.BannerId.ToString(),17,GachaMuted);
            int64 Total=0;
            for (const FOGGachaPoolEntry& E:Banner.Entries)
                Total+=FMath::Max(0,E.Weight);
            for (const FOGGachaPoolEntry& E:Banner.Entries)
            {
                const double Rate=Total>0
                    ? 100.0*static_cast<double>(E.Weight)/static_cast<double>(Total)
                    : 0.0;
                AddMessage(Rows,FString::Printf(TEXT("%s / %s / %.3f%%%s"),
                    *E.IdentityId.ToString(),*E.Rarity.ToString(),Rate,
                    E.bFeatured?TEXT(" / FEATURED"):TEXT("")),17,Main);
            }
            AddMessage(Rows,TEXT("Displayed weights are BASE rates. Persisted pity and featured guarantees apply to individual draws."),17,GachaMuted);
        }
        else if (Hud)
        {
            AddMessage(Rows,TEXT("ACTUAL SUMMON RESULTS"),26,Main);
            for (const FOGGachaPullResult& Result:Hud->GetLastGachaResults())
                AddMessage(Rows,FString::Printf(
                    TEXT("%s / %s / %s%s"),
                    *Result.IdentityId.ToString(),*Result.Rarity.ToString(),
                    *Result.ManifestationId.ToString(),
                    Result.bDuplicateIdentity?TEXT(" / INDEPENDENT COPY"):TEXT("")),
                    17,Main);
        }
        ActionAt(TEXT("CLOSE"),TEXT("gacha-overlay-close"),
                 629.f,256.f,125.f,53.f,true,19);
        return;
    }
    if (!bGachaHistoryExpanded) return;
    // This panel is the existing canonical acquisition history projection.'''
repl(hook,addition)
repl('''    const bool bCanDraw=false;''','''    const AOGWorldPresentationHud* Hud=GetOwningPlayer()
        ? Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()) : nullptr;
    const bool bCanDraw=Hud && Hud->HasActiveGachaPresentation() &&
        Session->bRealGachaAccessUnlocked;''')
repl('''    else if (Action == TEXT("gacha-history-toggle"))
    {
        bGachaHistoryExpanded = !bGachaHistoryExpanded;
    }''','''    else if (Action == TEXT("gacha-history-toggle"))
    {
        bGachaRatesExpanded = false;
        bGachaResultsExpanded = false;
        bGachaHistoryExpanded = !bGachaHistoryExpanded;
    }
    else if (Action == TEXT("gacha-rates"))
    {
        bGachaHistoryExpanded = false;
        bGachaResultsExpanded = false;
        bGachaRatesExpanded = !bGachaRatesExpanded;
    }
    else if (Action == TEXT("gacha-overlay-close"))
    {
        bGachaRatesExpanded = false;
        bGachaResultsExpanded = false;
        bGachaHistoryExpanded = false;
    }
    else if (Action == TEXT("gacha-draw-1") ||
             Action == TEXT("gacha-draw-10"))
    {
        AOGWorldPresentationHud* Hud=GetOwningPlayer()
            ? Cast<AOGWorldPresentationHud>(GetOwningPlayer()->GetHUD()) : nullptr;
        if (!Hud || !Session->bRealGachaAccessUnlocked ||
            !Hud->HasActiveGachaPresentation())
            Error=TEXT("No eligible installed banner or unlocked Ruler.");
        else if (Hud->ExecuteActiveGachaDraw(
                     Action == TEXT("gacha-draw-10")?10:1,Error))
        {
            bGachaRatesExpanded = false;
            bGachaHistoryExpanded = false;
            bGachaResultsExpanded = true;
        }
    }''')
repl('''        RecordsSection = -1;
        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);''','''        RecordsSection = -1;
        bGachaRatesExpanded = false;
        bGachaHistoryExpanded = false;
        bGachaResultsExpanded = false;
        Session->Navigate(static_cast<EOGRulerPrimaryDestination>(Index), Error);''')
f.write_text(s,encoding="utf-8",newline="")
print("CONNECTED_NATIVE_PORTRAIT_GACHA_TO_EXISTING_CANONICAL_EXECUTOR; NO_BUILD")
