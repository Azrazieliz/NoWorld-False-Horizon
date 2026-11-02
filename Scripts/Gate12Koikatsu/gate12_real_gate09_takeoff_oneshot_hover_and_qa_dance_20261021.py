from pathlib import Path
root=Path(r"D:\UnrealProjects\OfflineGame_Gate12_Koikatsu_20261010\Source\OfflineGame")
h=root/"Public/Koikatsu/OGLuminousV3PlayableCharacter.h"
cpp=root/"Private/Koikatsu/OGLuminousV3PlayableCharacter.cpp"
def patch(p,replacements):
 s=p.read_text(encoding="utf-8-sig")
 for a,b in replacements:
  n=s.count(a)
  if n!=1:raise RuntimeError(f"{p.name}: expected unique exact fragment, found {n}: {a[:90]!r}")
  s=s.replace(a,b,1)
 p.write_text(s,encoding="utf8",newline="")
 print("UPDATED",p.name)
patch(h,[
 ("""    float Gate09EmoteEndTime = 0.f;
    int32 QACycleStep = 0;""",
 """    float Gate09EmoteEndTime = 0.f;
    // Gate09's authentic flight-start clip is a one-shot takeoff, NEVER a
    // cyclic airborne locomotion replacement. Hover has its own loop.
    bool bGate09TakeoffAlreadyPlayedThisFlight = false;
    float Gate09TakeoffEndsAt = 0.f;
    int32 QACycleStep = 0;""")
])
patch(cpp,[
 ("""    CurrentWardrobe = Desired;
    if (!RefreshPrivacyRenderedAppearance())""",
 """    if (bGate09EmotePlaying)
        InterruptGate09AuthoredEmote();
    CurrentWardrobe = Desired;
    if (!RefreshPrivacyRenderedAppearance())"""),
 ("""bool AOGLuminousV3PlayableCharacter::PlayGate09AuthoredEmote(
    EOGLuminousV3AuthoredEmote Emote)
{
    if (!bLuminousReady ||""",
 """bool AOGLuminousV3PlayableCharacter::PlayGate09AuthoredEmote(
    EOGLuminousV3AuthoredEmote Emote)
{
    // The user explicitly rejected the dance's current artistic motion
    // quality. Keep its native source in the isolated existing Gate09 QA map
    // instead of marketing unaccepted choreography as a gameplay animation.
    if(Emote==EOGLuminousV3AuthoredEmote::DancePerformance &&
       !IsIsolatedWardrobeQAContext())
    {
        LastLuminousError=TEXT(
            "Gate09 dance source is imported but awaits visual acceptance; "
            "only native QA can preview it.");
        return false;
    }
    if (!bLuminousReady ||"""),
 ("""    if (Desired == CurrentMotionPhase)
    {
        // Flying has separate moving and hovering source motions. Update the
        // native sequence on velocity change without changing World state.
        if (Desired == EOGLuminousV3MotionPhase::Flying)
        {
            if (UAnimSequence* FlightClip = ResolveAuthoredMotionClip(Desired))
                if (FlightClip != CurrentNativeClip)
                    SetLuminousNativeMotionClip(FlightClip, true);
        }
        return;
    }""",
 """    // Gate09 verified flight-start, hover and swim are three distinct
    // Koikatsu source clips, not a single looping generic traversal gait.
    if(Desired==EOGLuminousV3MotionPhase::Flying)
    {
        const float Now=GetWorld()?GetWorld()->GetTimeSeconds():0.f;
        if(!bGate09TakeoffAlreadyPlayedThisFlight)
        {
            bGate09TakeoffAlreadyPlayedThisFlight=true;
            // Exact original character rig and Koikatsu asset; never synthesize
            // flight from a generic Walk, Run, ground Jump or dance.
            UAnimSequence* Takeoff=LoadObject<UAnimSequence>(nullptr,
                TEXT("/Game/Gate12NativeKoikatsu20261020/FlightDanceSwimming/"
                     "G12_LuminousV3_Koikatsu_Flight_Start."
                     "G12_LuminousV3_Koikatsu_Flight_Start"));
            if(Takeoff && SetLuminousNativeMotionClip(Takeoff,false))
                Gate09TakeoffEndsAt=Now+Takeoff->GetPlayLength();
            else
            {
                Gate09TakeoffEndsAt=Now;
                UE_LOG(LogTemp,Warning,
                    TEXT("GATE09_FLIGHT_TAKEOFF_NOT_PLAYABLE native clip missing or mismatch: %s"),
                    *LastLuminousError);
            }
            CurrentMotionPhase=Desired;
            return;
        }
        if(Now<Gate09TakeoffEndsAt)
            return; // Preserve the real single flight-start motion to its end.

        if(GetVelocity().SizeSquared()<FMath::Square(35.f))
        {
            if(UAnimSequence* Hover=ResolveAuthoredMotionClip(Desired))
                if(Hover!=CurrentNativeClip)
                    SetLuminousNativeMotionClip(Hover,true);
        }
        else if(RuntimeParts.IsValidIndex(ActiveMotionLeaderIndex) &&
                RuntimeParts[ActiveMotionLeaderIndex] &&
                CurrentNativeClip)
        {
            // No approved moving-flight loop exists yet. Do not repeat
            // takeoff, or depict fake forward flight using stationary hover.
            RuntimeParts[ActiveMotionLeaderIndex]->Stop();
            CurrentNativeClip=nullptr;
        }
        CurrentMotionPhase=Desired;
        return;
    }
    if(bGate09TakeoffAlreadyPlayedThisFlight)
    {
        bGate09TakeoffAlreadyPlayedThisFlight=false;
        Gate09TakeoffEndsAt=0.f; // Landing, fall or actual swimming ends flight.
    }
    if (Desired == CurrentMotionPhase)
        return;"""),
 ("""    bGate09EmotePlaying = false;
    ActiveMotionLeaderIndex = INDEX_NONE;
    Gate09EmoteEndTime = 0.f;""",
 """    bGate09EmotePlaying = false;
    ActiveMotionLeaderIndex = INDEX_NONE;
    Gate09EmoteEndTime = 0.f;
    bGate09TakeoffAlreadyPlayedThisFlight=false;
    Gate09TakeoffEndsAt=0.f;""")
])
print("CHECKPOINT_4: flight takeoff once, preserve takeoff completion, hover only when stationary, swim independent, rejected dance QA only, wardrobe interrupts emote")
