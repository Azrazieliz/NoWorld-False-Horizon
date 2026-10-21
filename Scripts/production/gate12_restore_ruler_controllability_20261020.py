from pathlib import Path
R=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
def change(file, swaps):
    path=R/file
    code=path.read_text(encoding="utf-8-sig")
    for x,y in swaps:
        n=code.count(x)
        if n!=1: raise RuntimeError(f"{file}: expected one occurrence, got {n} for {x[:95]!r}")
        code=code.replace(x,y,1)
    path.write_text(code,encoding="utf-8",newline="")
    print("EDITED",file)
base=Path("Source/OfflineGame")
change(base/"Public/Runtime/OGGate12RulerSessionSubsystem.h",[
("""    /** Persist identity-specific "last used" only after validating actual ownership. */""",
"""    /** The Ruler is the original player and is NEVER a gacha Manifestation.
     * Selecting the Ruler must remain possible after selecting a summon. */
    UFUNCTION(BlueprintCallable, Category="OfflineGame|Ruler|Characters")
    bool SelectRulerForWorld(FString& OutError);

    /** Persist identity-specific "last used" only after validating actual ownership. */"""),
])
change(base/"Private/Runtime/OGGate12RulerSessionSubsystem.cpp",[
("""bool UOGGate12RulerSessionSubsystem::SelectOwnedManifestation(
    const FOGContentId& IdentityId,""",
"""bool UOGGate12RulerSessionSubsystem::SelectRulerForWorld(FString& OutError)
{
    OutError.Reset();
    if (!RefreshCanonicalProjections(OutError))
        return false;

    // No manufactured owned Manifestation and no deletion of the previously
    // acquired roster. This selection changes only the World controller.
    SelectedIdentityId = FOGContentId();
    SelectedManifestationId = FOGEntityId();
    CurrentDestination = EOGRulerPrimaryDestination::Characters;
    UserFacingAvailability = TEXT("Controlling the original Ruler protagonist.");
    return true;
}

bool UOGGate12RulerSessionSubsystem::SelectOwnedManifestation(
    const FOGContentId& IdentityId,"""),
])
change(base/"Private/UI/OGGate12NativeRulerWidget.cpp",[
("""    if (!Session || Session->OwnedRoster.IsEmpty())
    {
        // No invented portrait, no stand-alone rotating QA tab in the""",
"""    // The protagonist is always available independently of gacha ownership.
    // This is NOT a character Manifestation and is never counted as one.
    if (Session && Session->IsBoundToStoredRuler())
    {
        AddMessage(Content, TEXT("RULER — ORIGINAL PLAYER"), 22, TextMain);
        AddAction(Content, TEXT("CONTROL THE RULER"), TEXT("select-ruler"));
    }
    if (!Session || Session->OwnedRoster.IsEmpty())
    {
        // No invented portrait, no stand-alone rotating QA tab in the"""),
("""    else if (Action == TEXT("roster/sort"))
    {
        RosterSortMode = (RosterSortMode + 1) % 3;
    }""",
"""    else if (Action == TEXT("select-ruler"))
    {
        if (Session->SelectRulerForWorld(Error))
        {
            OpenIdentityIndex = INDEX_NONE;
            bPrefer3DCharacterPresentation = false;
            ReleasePreviewStage();
        }
    }
    else if (Action == TEXT("roster/sort"))
    {
        RosterSortMode = (RosterSortMode + 1) % 3;
    }"""),
])
# Use the existing GameMode's original default protagonist pawn, only after
# a genuine owned pawn was installed and the player explicitly reselected Ruler.
change(base/"Private/World/OGStartingRegionPresentation.cpp",[
("""    if (Session->SelectedManifestationId.IsValid())
    {
        UOGGate12CharacterVisualCatalog* Catalog = Gate12NativeRulerWidget""",
"""    if (!Session->SelectedManifestationId.IsValid())
    {
        AOGWorldPrototypeCharacter* Previous =
            Cast<AOGWorldPrototypeCharacter>(PlayerOwner->GetPawn());
        const bool bWasControllingSummon = Previous &&
            Previous->GetClass()->ImplementsInterface(
                UOGGate12OwnedWorldCharacterBinding::StaticClass());
        if (bWasControllingSummon)
        {
            FActorSpawnParameters Spawn;
            Spawn.Owner = PlayerOwner;
            Spawn.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AOGWorldPrototypeCharacter* OriginalRuler =
                GetWorld()->SpawnActor<AOGWorldPrototypeCharacter>(
                    AOGWorldPrototypeCharacter::StaticClass(),
                    Previous->GetActorTransform(), Spawn);
            if (!IsValid(OriginalRuler))
            {
                FailAndReturnToRuler(TEXT("Original Ruler World pawn could not be restored."));
                return;
            }
            PlayerOwner->Possess(OriginalRuler);
            if (PlayerOwner->GetPawn() != OriginalRuler)
            {
                OriginalRuler->Destroy();
                if (IsValid(Previous)) PlayerOwner->Possess(Previous);
                FailAndReturnToRuler(TEXT("Ruler possession failed; previous pawn preserved."));
                return;
            }
            OriginalRuler->InitializeFoundationIntegration();
            Previous->Destroy();
        }
    }
    else
    {
        UOGGate12CharacterVisualCatalog* Catalog = Gate12NativeRulerWidget"""),
])
print("ONLY_SOURCE_MODIFIED_NO_BUILD")
