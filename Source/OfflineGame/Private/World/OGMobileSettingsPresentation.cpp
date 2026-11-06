#include "World/OGStartingRegionPresentation.h"

#include "Combat/OGDiagnosticCombatComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/OGSemanticAudioRuntime.h"
#include "UI/OGMobileSettingsOverlay.h"

namespace
{
struct FMobileSettingsRow
{
    FString Label;
    FName Action;
    FString SecondLabel;
    FName SecondAction;
    bool bAdjustment = false;
};

FString ToggleLabel(const TCHAR* Label, bool bEnabled)
{
    return FString::Printf(TEXT("%s: %s"), Label, bEnabled ? TEXT("On") : TEXT("Off"));
}

FString ChoiceLabel(const TCHAR* Label, FName Value)
{
    return FString::Printf(TEXT("%s: %s"), Label, *Value.ToString());
}
}

void AOGWorldPresentationHud::DrawMobileSettingsSurface(const AOGWorldPrototypeCharacter& Character,
    float Scale, FLinearColor MainText, FLinearColor SubText, FLinearColor Panel, FLinearColor Button)
{
    DrawRect(FLinearColor(0.f, 0.f, 0.f, .55f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
    const float PanelWidth = FMath::Min(820.f * Scale, Canvas->ClipX * .90f);
    const float PanelHeight = Canvas->ClipY * .84f;
    const float PanelX = (Canvas->ClipX - PanelWidth) * .5f;
    const float PanelY = (Canvas->ClipY - PanelHeight) * .5f;
    DrawGamePanel(Panel, PanelX, PanelY, PanelWidth, PanelHeight);

    const float ReadableHeight = FMath::Min(Canvas->ClipX, Canvas->ClipY) / 27.f;
    const float ButtonHeight = FMath::Max(32.f * Scale, ReadableHeight + 16.f);
    const float RowStep = ButtonHeight + 10.f * Scale;
    const float Left = PanelX + 28.f * Scale;
    const float RowWidth = PanelWidth - 56.f * Scale;
    const float TabY = PanelY + 122.f * Scale;
    const float RowsTop = TabY + ButtonHeight + 12.f * Scale;
    const float RowsBottom = PanelY + PanelHeight - ButtonHeight - 20.f * Scale;
    const int32 VisibleRows = FMath::Max(1, FMath::FloorToInt((RowsBottom - RowsTop) / RowStep));
    const float ButtonTextHeight = FMath::Min(ReadableHeight, (ButtonHeight - 12.f * Scale) / 1.12f);

    DrawGameText(TEXT("SETTINGS"), Left, PanelY + 24.f * Scale,
        FMath::Min(ReadableHeight * 1.3f, 38.f * Scale), MainText, RowWidth);
    const FString& Feedback = Character.GetMobileSettingsFeedback();
    DrawGameText(Feedback.IsEmpty() ? FString(TEXT("Changes save automatically")) : Feedback,
        Left, PanelY + 74.f * Scale, FMath::Min(ReadableHeight, 34.f * Scale), SubText, RowWidth);

    auto DrawControl = [&](const FString& Label, FName Action, float X, float Y, float Width, int32 Priority = 110)
    {
        DrawGamePanel(Button, X, Y, Width, ButtonHeight);
        DrawGameText(Label, X + 10.f * Scale, Y + 8.f * Scale, ButtonTextHeight, MainText, Width - 20.f * Scale);
        AddFoundationHitBox({X, Y}, {Width, ButtonHeight}, Action, true, Priority);
    };
    const float TabGap = 6.f * Scale;
    const float TabWidth = (RowWidth - 3.f * TabGap) / 4.f;
    DrawControl(TEXT("Controls"), TEXT("OG.Settings.Page.Controls"), Left, TabY, TabWidth);
    DrawControl(TEXT("Access"), TEXT("OG.Settings.Page.Accessibility"), Left + TabWidth + TabGap, TabY, TabWidth);
    DrawControl(TEXT("Audio"), TEXT("OG.Settings.Page.Audio"), Left + 2.f * (TabWidth + TabGap), TabY, TabWidth);
    DrawControl(TEXT("Content"), TEXT("OG.Settings.Page.Content"), Left + 3.f * (TabWidth + TabGap), TabY, TabWidth);

    TArray<FMobileSettingsRow, TInlineAllocator<16>> Rows;
    auto AddControl = [&Rows](FString Label, FName Action)
    {
        Rows.Add({MoveTemp(Label), Action, FString(), NAME_None, false});
    };
    auto AddAdjustment = [&Rows](const TCHAR* Label, float Value, const TCHAR* Setting)
    {
        const FString Prefix = FString(TEXT("OG.Settings.")) + Setting;
        Rows.Add({Label, FName(*(Prefix + TEXT(".Minus"))), FString::Printf(TEXT("%.2f"), Value),
            FName(*(Prefix + TEXT(".Plus"))), true});
    };

    switch (Character.GetFoundationSettingsPage())
    {
    case 0:
        // Current presentation contract owns orientation by mode.
        Rows.Add({TEXT("Orientation"), NAME_None, TEXT("AUTO BY MODE"), NAME_None, false});
        AddAdjustment(TEXT("Camera horizontal"), Character.GetCameraHorizontalSensitivity(), TEXT("CameraH"));
        AddAdjustment(TEXT("Camera vertical"), Character.GetCameraVerticalSensitivity(), TEXT("CameraV"));
        AddAdjustment(TEXT("Camera response"), Character.GetCameraResponseExponent(), TEXT("Response"));
        Rows.Add({ToggleLabel(TEXT("Invert horizontal"), Character.GetInvertCameraX()), TEXT("OG.Settings.InvertX"),
            ToggleLabel(TEXT("Invert vertical"), Character.GetInvertCameraY()), TEXT("OG.Settings.InvertY"), false});
        AddControl(ToggleLabel(TEXT("Motion blur"), Character.GetMotionBlurEnabled()), TEXT("OG.Settings.MotionBlur"));
        AddAdjustment(TEXT("Control scale"), Character.GetTouchControlScale(), TEXT("TouchScale"));
        AddAdjustment(TEXT("Control opacity"), Character.GetTouchControlOpacity(), TEXT("TouchOpacity"));
        AddAdjustment(TEXT("Movement deadzone"), Character.GetMovementDeadzone(), TEXT("MoveDeadzone"));
        AddAdjustment(TEXT("Look deadzone"), Character.GetLookDeadzone(), TEXT("LookDeadzone"));
        AddAdjustment(TEXT("Stick horizontal"), Character.GetMovementStickInset(), TEXT("MoveInset"));
        AddAdjustment(TEXT("Stick vertical"), Character.GetMovementStickBottom(), TEXT("MoveBottom"));
        AddAdjustment(TEXT("Actions horizontal"), Character.GetActionClusterHorizontalOffset(), TEXT("ActionX"));
        AddAdjustment(TEXT("Actions vertical"), Character.GetActionClusterVerticalOffset(), TEXT("ActionY"));
        Rows.Add({Character.GetLeftHandedControls() ? TEXT("Handedness: Left") : TEXT("Handedness: Right"),
            TEXT("OG.Settings.Handedness"), Character.GetSprintTogglePreference() ? TEXT("Sprint: Toggle") : TEXT("Sprint: Hold"),
            TEXT("OG.Settings.Sprint"), false});
        break;
    case 1:
        AddControl(ToggleLabel(TEXT("Privacy / SFW"), Character.GetSfwPresentation()), TEXT("OG.Settings.SFW"));
        AddControl(ToggleLabel(TEXT("Reduced motion"), Character.GetReducedMotion()), TEXT("OG.Settings.ReducedMotion"));
        AddControl(ToggleLabel(TEXT("Reduced camera shake"), Character.GetReducedCameraShake()), TEXT("OG.Settings.ReducedShake"));
        AddControl(ToggleLabel(TEXT("Subtitles / captions"), Character.GetSubtitlesEnabled()), TEXT("OG.Settings.Subtitles"));
        AddControl(ChoiceLabel(TEXT("Subtitle presentation"), Character.GetSubtitlePresentation()), TEXT("OG.Settings.SubtitlePresentation"));
        AddControl(ChoiceLabel(TEXT("UI readability"), Character.GetUiReadabilityProfile()), TEXT("OG.Settings.Readability"));
        AddControl(ChoiceLabel(TEXT("Color-vision palette"), Character.GetColorVisionProfile()), TEXT("OG.Settings.ColorVision"));
        AddControl(ToggleLabel(TEXT("Haptics"), Character.GetHapticsEnabled()), TEXT("OG.Settings.Haptics"));
        AddAdjustment(TEXT("Haptics intensity"), Character.GetHapticsIntensity(), TEXT("HapticsIntensity"));
        AddAdjustment(TEXT("UI effects intensity"), Character.GetUiEffectsIntensity(), TEXT("UiEffects"));
        AddControl(ChoiceLabel(TEXT("Damage numbers"), Character.GetDamageNumberPresentation()), TEXT("OG.Settings.DamageNumbers"));
        break;
    case 2:
        AddAdjustment(TEXT("Master volume"), Character.GetMasterVolume(), TEXT("MasterVolume"));
        AddAdjustment(TEXT("Music volume"), Character.GetMusicVolume(), TEXT("MusicVolume"));
        AddAdjustment(TEXT("Voice volume"), Character.GetVoiceVolume(), TEXT("VoiceVolume"));
        AddAdjustment(TEXT("SFX volume"), Character.GetSfxVolume(), TEXT("SfxVolume"));
        AddAdjustment(TEXT("Ambience volume"), Character.GetAmbienceVolume(), TEXT("AmbienceVolume"));
        AddControl(ChoiceLabel(TEXT("Dynamic range"), Character.GetDynamicRangeProfile()), TEXT("OG.Settings.DynamicRange"));
        break;
    default:
        AddControl(ChoiceLabel(TEXT("Roster density"), Character.GetRosterDensity()), TEXT("OG.Settings.RosterDensity"));
        AddControl(ChoiceLabel(TEXT("Ultimate cinematics"), Character.GetCinematicRepeatPolicy()), TEXT("OG.Settings.CinematicPolicy"));
        AddControl(ToggleLabel(TEXT("Automatic content downloads"), Character.GetAutoDownloadEnabled()), TEXT("OG.Settings.AutoDownload"));
        AddControl(Character.GetLargeDownloadsUnmeteredOnly() ? TEXT("Large downloads: Wi-Fi / unmetered only")
            : TEXT("Large downloads: Any network"), TEXT("OG.Settings.DownloadNetwork"));
        break;
    }

    SettingsLastRowPage = (Rows.Num() - 1) / VisibleRows;
    SettingsRowPage = FMath::Clamp(SettingsRowPage, 0, SettingsLastRowPage);
    const float SmallWidth = 48.f * Scale;
    const float HalfWidth = (RowWidth - 8.f * Scale) * .5f;
    const int32 FirstRow = SettingsRowPage * VisibleRows;
    for (int32 Index = FirstRow; Index < FMath::Min(Rows.Num(), FirstRow + VisibleRows); ++Index)
    {
        const auto& Row = Rows[Index];
        const float Y = RowsTop + (Index - FirstRow) * RowStep;
        if (Row.bAdjustment)
        {
            const float PlusX = Left + RowWidth - SmallWidth;
            const float MinusX = PlusX - SmallWidth - 6.f * Scale;
            const float ValueX = MinusX - FMath::Max(90.f * Scale, ReadableHeight * 2.5f);
            DrawGameText(Row.Label, Left, Y + 7.f * Scale, ButtonTextHeight, MainText,
                FMath::Max(1.f, ValueX - Left - 8.f * Scale));
            DrawGameText(Row.SecondLabel, ValueX, Y + 8.f * Scale, ButtonTextHeight, SubText,
                FMath::Max(1.f, MinusX - ValueX - 6.f * Scale));
            DrawControl(TEXT("-"), Row.Action, MinusX, Y, SmallWidth);
            DrawControl(TEXT("+"), Row.SecondAction, PlusX, Y, SmallWidth);
        }
        else if (Row.Action.IsNone())
        {
            const float ValueWidth = FMath::Min(150.f * Scale, HalfWidth);
            DrawGameText(Row.Label, Left, Y + 7.f * Scale, ButtonTextHeight, MainText, RowWidth - ValueWidth);
            DrawGameText(Row.SecondLabel, Left + RowWidth - ValueWidth, Y + 7.f * Scale,
                ButtonTextHeight, SubText, ValueWidth);
        }
        else
        {
            DrawControl(Row.Label, Row.Action, Left, Y, Row.SecondAction.IsNone() ? RowWidth : HalfWidth);
            if (!Row.SecondAction.IsNone())
                DrawControl(Row.SecondLabel, Row.SecondAction, Left + HalfWidth + 8.f * Scale, Y, HalfWidth);
        }
    }
    if (SettingsLastRowPage > 0)
    {
        const float FooterY = PanelY + PanelHeight - ButtonHeight - 8.f * Scale;
        const float FooterWidth = (RowWidth - 12.f * Scale) * .5f;
        DrawControl(TEXT("Previous controls"), TEXT("OG.Settings.Rows.Previous"), Left, FooterY, FooterWidth, 112);
        DrawControl(TEXT("More controls"), TEXT("OG.Settings.Rows.Next"),
            Left + FooterWidth + 12.f * Scale, FooterY, FooterWidth, 112);
    }
}

bool AOGWorldPresentationHud::SynchronizeMobileSettingsOverlay(const AOGWorldPrototypeCharacter* Character,
    bool bShowSettingsShortcut)
{
    FOGSemanticCaptionProjection Caption;
    if (Character)
        if (const auto* Audio = Character->FindComponentByClass<UOGSemanticAudioRuntime>())
            Caption = Audio->GetCurrentCaption();

    if ((!bShowSettingsShortcut && !Caption.bVisible) || !PlayerOwner || !PlayerOwner->GetLocalPlayer())
    {
        if (MobileSettingsOverlay && MobileSettingsOverlay->IsInViewport())
            MobileSettingsOverlay->RemoveFromParent();
        return false;
    }

    if (!MobileSettingsOverlay)
        MobileSettingsOverlay = CreateWidget<UOGMobileSettingsOverlay>(PlayerOwner, UOGMobileSettingsOverlay::StaticClass());
    if (!MobileSettingsOverlay) return false;

    MobileSettingsOverlay->UpdatePresentation(bShowSettingsShortcut, Caption);
    if (!MobileSettingsOverlay->IsInViewport()) MobileSettingsOverlay->AddToViewport(100);
    return MobileSettingsOverlay->IsInViewport();
}

void AOGWorldPresentationHud::DrawMobileWorldAction(AOGWorldPrototypeCharacter& Character,
    const FString& Label, FName HitBoxName, float HorizontalInset, float BottomInset,
    FLinearColor MainText, FLinearColor SubText)
{
    const bool bLeftHanded =
        Character.GetLeftHandedControls();
    const float ShortSide = FMath::Min(Canvas->ClipX, Canvas->ClipY);
    const float GridLimit = FMath::Min(Canvas->ClipX * .62f, Canvas->ClipY * .65f);
    const float Margin = ShortSide * .016f;
    const float ButtonSize = FMath::Min(ShortSide * .148f * Character.GetTouchControlScale(),
        (GridLimit - 3.f * Margin) / 3.f);
    const float Cell = ButtonSize + Margin;
    const float Column = FMath::RoundToFloat((HorizontalInset - 0.08f) / 0.09f);
    const float Row = FMath::RoundToFloat((BottomInset - 0.13f) / 0.17f);
    const float MinimumInset = ButtonSize * 0.5f + Margin;
    // Clamp the whole grid, preserving spacing at profile extremes.
    const float BaseX = FMath::Clamp(ShortSide * 0.08f +
        Canvas->ClipX * Character.GetActionClusterHorizontalOffset(),
        MinimumInset, FMath::Max(MinimumInset,
            Canvas->ClipX * 0.62f - 2.0f * Cell - ButtonSize * 0.5f));
    const float BaseY = FMath::Clamp(ShortSide * 0.13f +
        Canvas->ClipY * Character.GetActionClusterVerticalOffset(),
        MinimumInset, FMath::Max(MinimumInset,
            Canvas->ClipY * 0.65f - 2.0f * Cell - ButtonSize * 0.5f));
    const float SideInset = BaseX + Column * Cell;
    const float CenterX = bLeftHanded ? SideInset : Canvas->ClipX - SideInset;
    const float CenterY = Canvas->ClipY - BaseY - Row * Cell;
    const float X = CenterX - ButtonSize * 0.5f;
    const float Y = CenterY - ButtonSize * 0.5f;

    bool bReady = Character.GetFoundationHealth() > 0.0f;
    FString DisabledReason;
    if (auto* Combat = Character.FindComponentByClass<UOGDiagnosticCombatComponent>())
    {
        if (HitBoxName == TEXT("OG.World.Attack")) bReady = Combat->IsWorldActionReady(EOGDiagnosticCommand::Basic, DisabledReason);
        else if (HitBoxName == TEXT("OG.World.Skill.0")) bReady = Combat->IsWorldActionReady(EOGDiagnosticCommand::Skill1, DisabledReason);
        else if (HitBoxName == TEXT("OG.World.Skill.1")) bReady = Combat->IsWorldActionReady(EOGDiagnosticCommand::Skill2, DisabledReason);
        else if (HitBoxName == TEXT("OG.World.Ultimate")) bReady = Combat->IsWorldActionReady(EOGDiagnosticCommand::Ultimate, DisabledReason);
    }
    const bool bPressed = PressedWorldControls.Contains(HitBoxName);
    const FLinearColor Accent = bReady ? FLinearColor(.77f,.69f,.86f,1) : FLinearColor(.38f,.37f,.46f,1);
    const bool bHighContrast = Character.GetUiReadabilityProfile() == TEXT("high_contrast");
    const FLinearColor Face = bHighContrast
        ? FLinearColor(.005f, bPressed ? .11f : .008f, bPressed ? .15f : .012f, Character.GetTouchControlOpacity())
        : FLinearColor(bPressed ? .14f : .025f, bPressed ? .24f : .045f,
            bPressed ? .30f : .075f, Character.GetTouchControlOpacity());
    Canvas->K2_DrawPolygon(nullptr, FVector2D(CenterX,CenterY),
        FVector2D(ButtonSize*.48f,ButtonSize*.48f), 8, Face);
    for(int32 I=0; I<8; ++I)
    {
        const float A=I*2*PI/8.f, B=(I+1)*2*PI/8.f, R=ButtonSize*.46f;
        DrawLine(CenterX+FMath::Cos(A)*R,CenterY+FMath::Sin(A)*R,
            CenterX+FMath::Cos(B)*R,CenterY+FMath::Sin(B)*R,Accent,bPressed ? 3.f : bHighContrast ? 2.5f : 1.5f);
    }
    if (HitBoxName==TEXT("OG.World.Attack"))
    {
        // Universal ACTION glyph, never a fixed sword, gun,
        // bow, staff or anatomical fist. The actual effect and
        // motion are determined by the bound authored kit/weapon,
        // not by the HUD's invented equipment assumption.
        const float L=ButtonSize*.23f;
        for (int32 I=0;I<4;++I)
        {
            const float A=I*PI*.5f+PI*.25f;
            const float B=(I+1)*PI*.5f+PI*.25f;
            DrawLine(CenterX+FMath::Cos(A)*L,
                CenterY+FMath::Sin(A)*L,
                CenterX+FMath::Cos(B)*L,
                CenterY+FMath::Sin(B)*L,Accent,1.8f);
        }
        DrawLine(CenterX-L*.44f,CenterY,
            CenterX+L*.44f,CenterY,Accent,1.8f);
        DrawLine(CenterX,CenterY-L*.44f,
            CenterX,CenterY+L*.44f,Accent,1.8f);
    }
    else
        DrawGameIcon(FName(*Label),X+ButtonSize*.26f,
            Y+ButtonSize*.10f,ButtonSize*.48f,Accent);
    const FString Caption=Label;
    DrawGameText(Caption,X+ButtonSize*.15f,Y+ButtonSize*.65f,
        ButtonSize*.18f,bReady?MainText:SubText,ButtonSize*.70f);
    AddFoundationHitBox(
        FVector2D(X, Y),
        FVector2D(ButtonSize, ButtonSize),
        HitBoxName,
        true,
        109);
}

void AOGWorldPresentationHud::DrawMobileControlText(const FString& Label, FLinearColor Color,
    float X, float Y, float Width, float Height, float OriginalScale)
{
    const UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
    if (!Font || Width <= 0.f || Height <= 0.f) return;
    const float ReadableHeight = FMath::Min(Canvas->ClipX, Canvas->ClipY) / 27.f;
    const float TextHeight = FMath::Min(Height / 1.12f,
        FMath::Max(ReadableHeight, OriginalScale * Font->GetMaxCharHeight()));
    DrawGameText(Label, X, Y, TextHeight, Color, Width);
}
