from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
f=root/"Source/OfflineGame/Private/Gacha/OGGachaService.cpp"
s=f.read_text(encoding="utf-8-sig")
pairs=[
("""    bool bUnfeaturedTopOnly,
    bool bGuaranteedSrPlus,
    int32& OutIndex,""",
"""    bool bUnfeaturedTopOnly,
    bool bSrOnly,
    int32& OutIndex,"""),
("""    const bool bAdditivePity = !bTopOnly && !bFeaturedTopOnly &&
        !bUnfeaturedTopOnly && Banner.SoftPityStart > 0 &&""",
"""    const bool bAdditivePity = !bTopOnly && !bFeaturedTopOnly &&
        !bUnfeaturedTopOnly && !bSrOnly && Banner.SoftPityStart > 0 &&"""),
("""            (bGuaranteedSrPlus && Entry.Rarity == FName(TEXT("R"))) ||
            (bAdditivePity && bTop != bSoftTop))""",
"""            (bSrOnly && Entry.Rarity != FName(TEXT("SR"))) ||
            (bAdditivePity && bTop != bSoftTop))"""),
("""    // Gate07 D03 approved: a ten-pull must contain at least one SR-or-above
    // Manifestation. Refuse unsuitable pools before charging or writing.
    if (Count == 10 &&
        !Banner.Entries.ContainsByPredicate([](const FOGGachaPoolEntry& Entry)
        {
            return Entry.Weight > 0 && Entry.Rarity != FName(TEXT("R"));
        }))
    {
        OutError = TEXT("Ten-pull requires at least one eligible SR+ banner entry.");
        return false;
    }""",
"""    // Gate07 D03 exact approved A: if all ten initial results would be R,
    // replace the LAST with an actual eligible SR. Never manufacture SR
    // content; fail before transaction if a banner cannot honor that floor.
    if (Count == 10 &&
        !Banner.Entries.ContainsByPredicate([](const FOGGachaPoolEntry& Entry)
        {
            return Entry.Weight > 0 && Entry.Rarity == FName(TEXT("SR"));
        }))
    {
        OutError = TEXT("Ten-pull requires an eligible authored SR pool entry.");
        return false;
    }"""),
("""        const bool bApplyFloor = Count == 10 && Index == 9 &&
            !bAlreadyRolledSrPlus;
        if (!PullInternal(Banner, RulerId, WorldTick, NextSeed, Result,
                OutError, false, bApplyFloor))""",
"""        // First nine R allow a potential floor on the final slot, but the
        // final slot is still rolled normally before any conditional upgrade.
        const bool bApplyFloor = Count == 10 && Index == 9 &&
            !bAlreadyRolledSrPlus;
        if (!PullInternal(Banner, RulerId, WorldTick, NextSeed, Result,
                OutError, false, bApplyFloor))"""),
("""            bHardPity,
            false,
            false,
            bGuaranteedSrPlus,
            SelectedIndex,""",
"""            bHardPity,
            false,
            false,
            false,
            SelectedIndex,"""),
("""    const FOGGachaPoolEntry* Selected =
        &Banner.Entries[SelectedIndex];

    bool bTopRarity =""",
"""    // D03 applies only when the final naturally rolled result is ALSO R,
    // not when the tenth result is already an SR, SSR, UR, LR or MR.
    if (bGuaranteedSrPlus &&
        Banner.Entries[SelectedIndex].Rarity == FName(TEXT("R")))
    {
        if (!SelectWeightedEntry(Banner, Rng, PullOrdinalSinceTop,
                false, false, false, true, SelectedIndex, Error))
            return Fail(Error);
    }

    const FOGGachaPoolEntry* Selected =
        &Banner.Entries[SelectedIndex];

    bool bTopRarity ="""),
]
for old,new in pairs:
    n=s.count(old)
    if n!=1: raise RuntimeError(f"match {n}: {old[:100]!r}")
    s=s.replace(old,new,1)
f.write_text(s,encoding="utf-8",newline="")
print("EXACT_D03_UPGRADE_AFTER_NATURAL_TENTH_ROLL_SOURCE_CHANGED; NO_BUILD")
