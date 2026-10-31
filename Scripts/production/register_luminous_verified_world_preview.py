from pathlib import Path
r=Path(__file__).resolve().parents[2]
p=r/"Source/OfflineGame/Private/UI/OGGate12CharacterVisualCatalog.cpp"
s=p.read_text("utf8")
anchor='''    if (InstalledNativePreview && Exact.GenuineCharacterPreviewClass.IsNull())
        Exact.GenuineCharacterPreviewClass =
            TSoftClassPtr<AActor>(FSoftObjectPath(InstalledNativePreview));'''
assert s.count(anchor)==1
extra='''
    // These two source-verified forms are now real native UE5.8 skeletal
    // asset packages (9 skinned parts each). Bind the exact original
    // non-Pawn viewer and canonical-owned World pawn, not the historical
    // unmatched Berserker Blueprint nor a diagnostic mannequin. The other
    // character Versions remain unmodified.
    if (RequestedVersion ==
            TEXT("character:luminous_valentine.version_awakened_v4") ||
        RequestedVersion ==
            TEXT("character:luminous_valentine.version_berserker_v4"))
    {
        Exact.GenuineCharacterPreviewClass =
            TSoftClassPtr<AActor>(FSoftObjectPath(
                TEXT("/Script/OfflineGame.OGLuminousExactOriginalPreviewActor")));
        const TCHAR* NativeWorld = RequestedVersion ==
            TEXT("character:luminous_valentine.version_awakened_v4")
                ? TEXT("/Script/OfflineGame.OGLuminousAwakenedOriginalWorldCharacter")
                : TEXT("/Script/OfflineGame.OGLuminousBerserkerOriginalWorldCharacter");
        Exact.ApprovedWorldPlayableClass =
            TSoftClassPtr<AOGWorldPrototypeCharacter>(
                FSoftObjectPath(NativeWorld));
    }'''
s=s.replace(anchor,anchor+extra)
p.write_text(s,"utf8")
print("EXACT_KOIKATSU_3D_NOW_CONNECTED_TO_ACTUAL_ROSTER_AND_WORLD",p)
