#include "World/OGStartingRegionPresentation.h"

#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/TouchInterface.h"
#include "InputCoreTypes.h"
#include "Runtime/OGPlayerProfileSettings.h"

void AOGWorldPrototypeCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (HasActorBegunPlay() && IsLocallyControlled())
    {
        LoadControlProfile();
        LastTouchViewportSize = FIntPoint::ZeroValue;
        RefreshFoundationTouchInterfaceIfNeeded();
    }
}

void AOGWorldPrototypeCharacter::ToggleMotionBlur()
{
    FOGPlayerProfileSettings Profile;
    if (!LoadMutablePlayerProfile(Profile)) return;
    Profile.bMotionBlurEnabled = !Profile.bMotionBlurEnabled;
    SaveMutablePlayerProfileAndApply(Profile);
}

void AOGWorldPrototypeCharacter::AdjustUiEffectsIntensity(float Delta)
{
    FOGPlayerProfileSettings Profile;
    if (!LoadMutablePlayerProfile(Profile)) return;
    Profile.UiEffectsIntensity = FMath::Clamp(Profile.UiEffectsIntensity + Delta, 0.f, 1.f);
    SaveMutablePlayerProfileAndApply(Profile);
}

void AOGWorldPrototypeCharacter::CreateFoundationTouchInterface()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    auto* PC = Cast<APlayerController>(Controller);
    if (!PC || !PC->IsLocalController()) return;

    int32 Width = 0;
    int32 Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return;

    JumpReleased();
    TraversalDownReleased();
    SprintReleased();
    bCameraTouchActive = bCameraPinchActive = bCameraPinchUsed = false;
    LastCameraPinchDistance = 0.f;
    FoundationTouchInterface = NewObject<UTouchInterface>(this);

    const float Size = FMath::Min(Width, Height) * TouchControlScale;
    const FVector2D Square(Size / Width, Size / Height);
    FTouchInputControl Stick;
    Stick.Image1 = LoadObject<UTexture2D>(nullptr,
        TEXT("/Engine/MobileResources/HUD/VirtualJoystick_Thumb.VirtualJoystick_Thumb"));
    Stick.Image2 = LoadObject<UTexture2D>(nullptr,
        TEXT("/Engine/MobileResources/HUD/VirtualJoystick_Background.VirtualJoystick_Background"));
    Stick.InteractionSize = Square * .20f;
    Stick.VisualSize = Square * .15f;
    Stick.ThumbSize = Square * .065f;
    const float InsetX = FMath::Clamp(MovementStickInset, Stick.InteractionSize.X * .5f + .015f, .34f);
    const float InsetY = FMath::Clamp(MovementStickBottom, Stick.InteractionSize.Y * .5f + .015f, .36f);
    Stick.Center = FVector2D(bLeftHandedControls ? -InsetX : InsetX, -InsetY);
    Stick.InputScale = FVector2D(1.f, 1.f);
    Stick.MainInputKey = EKeys::Gamepad_LeftX;
    Stick.AltInputKey = EKeys::Gamepad_LeftY;

    // The Canvas owns contextual action hitboxes. Only movement uses a virtual stick.
    FoundationTouchInterface->Controls.Add(Stick);
    FoundationTouchInterface->ActiveOpacity = TouchControlOpacity;
    FoundationTouchInterface->InactiveOpacity = FMath::Clamp(TouchControlOpacity * .45f, .18f, .50f);
    FoundationTouchInterface->TimeUntilDeactive = .25f;
    FoundationTouchInterface->TimeUntilReset = 0.f;
    FoundationTouchInterface->ActivationDelay = 0.f;
    FoundationTouchInterface->StartupDelay = 0.f;
    FoundationTouchInterface->bPreventRecenter = true;
    LastTouchViewportSize = FIntPoint(Width, Height);
    PC->ActivateTouchInterface(FoundationTouchInterface);
    RefreshFoundationInputState();
#endif
}

bool AOGWorldPrototypeCharacter::IsCameraDragTouch(const FVector& ScreenLocation) const
{
    const auto* PC = Cast<APlayerController>(Controller);
    if (!PC || !IsFoundationWorldInputAllowed()) return false;

    int32 Width = 0;
    int32 Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0) return false;

    const FVector2D Point(ScreenLocation.X, ScreenLocation.Y);
    if (AHUD* Hud = PC->GetHUD())
        if (Hud->GetHitBoxAtCoordinates(Point, true)) return false;

    const FVector2D Normalized(Point.X / Width, Point.Y / Height);
    if (Normalized.Y < .11f) return false;
    if (!FoundationTouchInterface || FoundationTouchInterface->Controls.IsEmpty()) return true;

    const FTouchInputControl& Stick = FoundationTouchInterface->Controls[0];
    const FVector2D Center(Stick.Center.X < 0.f ? 1.f + Stick.Center.X : Stick.Center.X,
        Stick.Center.Y < 0.f ? 1.f + Stick.Center.Y : Stick.Center.Y);
    return FMath::Abs(Normalized.X - Center.X) > Stick.InteractionSize.X * .5f ||
        FMath::Abs(Normalized.Y - Center.Y) > Stick.InteractionSize.Y * .5f;
}
