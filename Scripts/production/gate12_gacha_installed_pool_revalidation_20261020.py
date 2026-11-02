from pathlib import Path
P=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame\Private\World\OGStartingRegionPresentation.cpp")
s=P.read_text(encoding="utf-8-sig")
before='''void AOGWorldPresentationHud::SetActiveGachaPresentation(
    const FOGGachaBannerDefinition& Banner,
    int64 CanonicalWorldTick,
    int64 DeterministicSeed)
{'''
after='''namespace
{
bool ValidateInstalledPlayableGachaPool(
    const FOGGachaBannerDefinition& Banner,FString& OutError)
{
    // A syntactically valid weighted pool is not D04 eligibility.
    // Never charge the Ruler for a nonexistent/unauthorized Version or
    // grant an empty placeholder Manifestation with no real World actor.
    UOGGate12CharacterVisualCatalog* Catalog =
        LoadObject<UOGGate12CharacterVisualCatalog>(nullptr,
            TEXT("/Game/NoWorld/UI/DA_NoWorld_CharacterVisuals."
                 "DA_NoWorld_CharacterVisuals"));
    if (!Catalog)
    {
        OutError=TEXT("No installed accepted character visual Registry catalog.");
        return false;
    }
    for (const FOGGachaPoolEntry& Pool:Banner.Entries)
    {
        const FOGGate12CharacterVisualEntry* Installed =
            Catalog->FindExact(Pool.IdentityId,Pool.VersionId);
        if (!Installed || Installed->ApprovedWorldPlayableClass.IsNull())
        {
            OutError=FString::Printf(
                TEXT("Uninstalled or ineligible pool Identity/Version: %s / %s"),
                *Pool.IdentityId.ToString(),*Pool.VersionId.ToString());
            return false;
        }
        const UClass* Class=Installed->ApprovedWorldPlayableClass.LoadSynchronous();
        if (!Class || !Class->IsChildOf(AOGWorldPrototypeCharacter::StaticClass()) ||
            !Class->ImplementsInterface(
                UOGGate12OwnedWorldCharacterBinding::StaticClass()))
        {
            OutError=TEXT("Banner member lacks an approved exact owned World actor.");
            return false;
        }
    }
    return true;
}
}

void AOGWorldPresentationHud::SetActiveGachaPresentation(
    const FOGGachaBannerDefinition& Banner,
    int64 CanonicalWorldTick,
    int64 DeterministicSeed)
{'''
if s.count(before)!=1:raise RuntimeError("banner method missing")
s=s.replace(before,after,1)
before='''    if (!FOGGachaService::ValidateBanner(Banner, Error) ||
        CanonicalWorldTick < 0)'''
after='''    if (!FOGGachaService::ValidateBanner(Banner, Error) ||
        !ValidateInstalledPlayableGachaPool(Banner, Error) ||
        CanonicalWorldTick < 0)'''
if s.count(before)!=1:raise RuntimeError("banner validation missing")
s=s.replace(before,after,1)
before='''    if (Tick < 0 || !FOGGachaService::ValidateBanner(ActiveGachaBanner, OutError))'''
after='''    if (Tick < 0 ||
        !FOGGachaService::ValidateBanner(ActiveGachaBanner, OutError) ||
        !ValidateInstalledPlayableGachaPool(ActiveGachaBanner, OutError))'''
if s.count(before)!=1:raise RuntimeError("transaction guard missing")
s=s.replace(before,after,1)
P.write_text(s,encoding="utf-8",newline="")
print("GACHA_D04_EXACT_INSTALLED_WORLD_ASSET_VALIDATION_IN_EXISTING_HUD_NO_BUILD")
