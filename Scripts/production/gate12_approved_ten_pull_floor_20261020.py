from pathlib import Path
p=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
def edit(rel,replacements):
    path=p/rel
    original=path.read_text(encoding="utf-8-sig")
    code=original
    for old,new in replacements:
        n=code.count(old)
        if n!=1: raise RuntimeError(f"{rel}: match count {n} for {old[:80]!r}")
        code=code.replace(old,new,1)
    path.write_text(code,encoding="utf-8",newline="")
    print("CHANGED",rel)
cpp=Path("Source/OfflineGame/Private/Gacha/OGGachaService.cpp")
hdr=Path("Source/OfflineGame/Public/Gacha/OGGachaService.h")
edit(hdr,[
("""        FString& OutError, bool bOwnTransaction);""",
"""        FString& OutError, bool bOwnTransaction,
        bool bGuaranteedSrPlus = false);"""),
])
edit(cpp,[
("""    bool bUnfeaturedTopOnly,
    int32& OutIndex,""",
"""    bool bUnfeaturedTopOnly,
    bool bGuaranteedSrPlus,
    int32& OutIndex,"""),
("""        if ((bTopOnly && !bTop) ||
            (bFeaturedTopOnly && (!bTop || !Entry.bFeatured)) ||
            (bUnfeaturedTopOnly && (!bTop || Entry.bFeatured)) ||
            (bAdditivePity && bTop != bSoftTop))""",
"""        if ((bTopOnly && !bTop) ||
            (bFeaturedTopOnly && (!bTop || !Entry.bFeatured)) ||
            (bUnfeaturedTopOnly && (!bTop || Entry.bFeatured)) ||
            (bGuaranteedSrPlus && Entry.Rarity == FName(TEXT("R"))) ||
            (bAdditivePity && bTop != bSoftTop))"""),
("""    // D03 floor method remains deferred. Never charge for a ten-pull
    // without its mandatory SR+ atomic floor; fail before transaction.
    if (Count == 10)
    {
        OutError = TEXT("Ten-pull unavailable until approved SR+ floor implementation.");
        return false;
    }
    if (!ValidateBanner(Banner, OutError) || !RulerId.IsValid())""",
"""    if (!ValidateBanner(Banner, OutError) || !RulerId.IsValid())"""),
("""    if (!Store.BeginTransaction(OutError)) return false;
    auto Fail = [this, &OutResults, &OutError]()""",
"""    // Gate07 D03 approved: a ten-pull must contain at least one SR-or-above
    // Manifestation. Refuse unsuitable pools before charging or writing.
    if (Count == 10 &&
        !Banner.Entries.ContainsByPredicate([](const FOGGachaPoolEntry& Entry)
        {
            return Entry.Weight > 0 && Entry.Rarity != FName(TEXT("R"));
        }))
    {
        OutError = TEXT("Ten-pull requires at least one eligible SR+ banner entry.");
        return false;
    }
    if (!Store.BeginTransaction(OutError)) return false;
    auto Fail = [this, &OutResults, &OutError]()"""),
("""    int64 NextSeed = Seed;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        FOGGachaPullResult Result;
        if (!PullInternal(Banner, RulerId, WorldTick, NextSeed, Result, OutError, false))
            return Fail();""",
"""    int64 NextSeed = Seed;
    bool bAlreadyRolledSrPlus = false;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        FOGGachaPullResult Result;
        const bool bApplyFloor = Count == 10 && Index == 9 &&
            !bAlreadyRolledSrPlus;
        if (!PullInternal(Banner, RulerId, WorldTick, NextSeed, Result,
                OutError, false, bApplyFloor))
            return Fail();
        if (Result.Rarity != FName(TEXT("R"))) bAlreadyRolledSrPlus = true;"""),
("""    bool bOwnTransaction)
{
    OutResult = FOGGachaPullResult();""",
"""    bool bOwnTransaction,
    bool bGuaranteedSrPlus)
{
    OutResult = FOGGachaPullResult();"""),
("""            bHardPity,
            false,
            false,
            SelectedIndex,""",
"""            bHardPity,
            false,
            false,
            bGuaranteedSrPlus,
            SelectedIndex,"""),
("""                true, bUseFeatured, !bUseFeatured,
                SelectedIndex,""",
"""                true, bUseFeatured, !bUseFeatured,
                false,
                SelectedIndex,"""),
])
print("SOURCE_ONLY_NO_BUILD")
