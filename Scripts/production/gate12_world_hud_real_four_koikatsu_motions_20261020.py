from pathlib import Path
P=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010")
h=P/"Source/OfflineGame/Public/World/OGStartingRegionPresentation.h"
c=P/"Source/OfflineGame/Private/World/OGStartingRegionPresentation.cpp"
H=h.read_text(encoding="utf-8-sig"); C=c.read_text(encoding="utf-8-sig")
def repl(src,old,new):
 n=src.count(old)
 if n!=1:raise RuntimeError(f"count {n} {old[:110]!r}")
 return src.replace(old,new,1)
H=repl(H,'''    bool bInteractionMenuOpen = false;
    bool DrawCanonicalTurnSurface''','''    bool bInteractionMenuOpen = false;
    // Visible genuine Koikatsu motions for the actually possessed Luminous,
    // not a standalone costume-test pawn or fictional approved animation.
    bool bLuminousAuthoredEmotesExpanded = false;
    bool DrawCanonicalTurnSurface''')
C=repl(C,'''#include "UI/OGGate12NativeRulerWidget.h"''','''#include "UI/OGGate12NativeRulerWidget.h"
#include "Koikatsu/OGLuminousV3PlayableCharacter.h"''')
C=repl(C,'''    DrawFoundationDiagnosticSurface(Character, Scale, false);
    if (Character)
    {
        const auto Caption =''','''    // Source-authentic Gate09 clips now reachable from the REAL World HUD
    // only when the exact native Luminous playable pawn is possessed. The
    // four clips never play on the Ruler or a non-compatible body/skeleton.
    if (bWorldSurface && Cast<AOGLuminousV3PlayableCharacter>(Character))
    {
        const float W=FMath::Min(156.f*Scale,Canvas->ClipX*.24f);
        const float X=18.f*Scale;
        const float Y=Canvas->ClipY*.26f;
        DrawGameButton(
            bLuminousAuthoredEmotesExpanded ? TEXT("CLOSE POSES") : TEXT("POSES"),
            TEXT("OG.Gate09.Emotes.Toggle"),X,Y,W,46.f*Scale,false,true,112);
        if (bLuminousAuthoredEmotesExpanded)
        {
            static const TCHAR* Names[] = {
                TEXT("Thoughtful"),TEXT("Greeting"),TEXT("Expressive"),TEXT("Bow")
            };
            for(int32 I=0;I<4;++I)
                DrawGameButton(Names[I],
                    FName(*FString::Printf(TEXT("OG.Gate09.Emote.%d"),I)),
                    X,Y+(I+1)*50.f*Scale,W,44.f*Scale,false,true,112);
        }
    }
    else bLuminousAuthoredEmotesExpanded=false;

    DrawFoundationDiagnosticSurface(Character, Scale, false);
    if (Character)
    {
        const auto Caption =''')
C=repl(C,'''    if (!Character)
    {
        return;
    }

    if (FoundationSurface == FName(TEXT("opening")) &&''','''    if (!Character)
    {
        return;
    }
    if (FoundationSurface == FName(TEXT("world")) &&
        BoxName == TEXT("OG.Gate09.Emotes.Toggle"))
    {
        bLuminousAuthoredEmotesExpanded =
            Cast<AOGLuminousV3PlayableCharacter>(Character) &&
            !bLuminousAuthoredEmotesExpanded;
        return;
    }
    if (FoundationSurface == FName(TEXT("world")) &&
        BoxName.ToString().StartsWith(TEXT("OG.Gate09.Emote.")))
    {
        if (AOGLuminousV3PlayableCharacter* Luminous =
            Cast<AOGLuminousV3PlayableCharacter>(Character))
        {
            const FString Suffix=BoxName.ToString().Mid(16);
            int32 Index=INDEX_NONE;
            if (LexTryParseString(Index,*Suffix) && Index>=0 && Index<4)
            {
                if (!Luminous->PlayGate09AuthoredEmote(
                    static_cast<EOGLuminousV3AuthoredEmote>(Index)))
                    FoundationStatusMessage=Luminous->GetLastLuminousError();
                else FoundationStatusMessage.Reset();
            }
        }
        bLuminousAuthoredEmotesExpanded=false;
        return;
    }

    if (FoundationSurface == FName(TEXT("opening")) &&''')
# Parse prefix exact length OG.Gate09.Emote. = 16 char; ensure
assert len("OG.Gate09.Emote.")==16
h.write_text(H,encoding="utf-8",newline="")
c.write_text(C,encoding="utf-8",newline="")
print("EXISTING_WORLD_HUD_NOW_DISPATCHES_4_REAL_KOIKATSU_ANIMSEQUENCES_SOURCE_ONLY")
