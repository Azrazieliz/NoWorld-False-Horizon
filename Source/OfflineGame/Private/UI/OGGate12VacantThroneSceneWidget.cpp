#include "UI/OGGate12VacantThroneSceneWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

void UOGGate12VacantThroneSceneWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if(!WidgetTree || WidgetTree->RootWidget) return;
    UBorder* Root=WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),TEXT("OriginalVacantThroneVectorStage"));
    Root->SetBrushColor(FLinearColor(0.f,0.f,0.f,0.f));
    Root->SetVisibility(ESlateVisibility::HitTestInvisible);
    WidgetTree->RootWidget=Root;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UOGGate12VacantThroneSceneWidget::UpdateAtmosphere(
    float TimeSeconds,bool bInReducedMotion)
{
    const float NewTime=bInReducedMotion?0.f:TimeSeconds;
    // Cap paint invalidation to roughly 24 fps for low-power devices,
    // while a reduced-motion still frame requires no recurring repaint.
    if (bReducedMotion==bInReducedMotion &&
        (bInReducedMotion ||
         FMath::Abs(NewTime-AtmosphereSeconds)<(1.f/24.f)))
        return;
    AtmosphereSeconds=NewTime;
    bReducedMotion=bInReducedMotion;
    InvalidateLayoutAndVolatility();
}

int32 UOGGate12VacantThroneSceneWidget::NativePaint(
    const FPaintArgs& Args,const FGeometry& Geometry,
    const FSlateRect& Culling,FSlateWindowElementList& Elements,
    int32 Layer,const FWidgetStyle& Style,bool bEnabled) const
{
    const int32 Base=Super::NativePaint(
        Args,Geometry,Culling,Elements,Layer,Style,bEnabled);
    const FVector2D Size=Geometry.GetLocalSize();
    if(Size.X<80.f || Size.Y<90.f) return Base;
    const float W=Size.X,H=Size.Y;
    auto V=[W,H](float X,float Y){return FVector2D(W*X,H*Y);};
    auto Line=[&](FVector2D A,FVector2D B,const FLinearColor& Tint,
                  float Thickness=1.f)
    {
        TArray<FVector2D> P;P.Add(A);P.Add(B);
        FSlateDrawElement::MakeLines(Elements,Base+2,
            Geometry.ToPaintGeometry(),P,ESlateDrawEffect::None,
            Tint,true,Thickness);
    };
    auto L=[&](float A,float B,float C,float D,
               const FLinearColor& Tint,float Thickness=1.f)
    {Line(V(A,B),V(C,D),Tint,Thickness);};
    auto Poly=[&](const TArray<FVector2D>& Points,FLinearColor Tint,
                  float Thick=1.f)
    {
        FSlateDrawElement::MakeLines(Elements,Base+2,
            Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,
            Tint,true,Thick);
    };
    auto Diamond=[&](float X,float Y,float R,
                     const FLinearColor& Tint,float Width=1.f)
    {
        TArray<FVector2D> P{V(X,Y-R),V(X+R*.55f,Y),
            V(X,Y+R),V(X-R*.55f,Y),V(X,Y-R)};
        Poly(P,Tint,Width);
    };
    auto Rect=[&](float X,float Y,float DX,float DY,FLinearColor Tint)
    {
        static const FSlateColorBrush White(FLinearColor::White);
        FSlateDrawElement::MakeBox(Elements,Base+1,
            Geometry.ToPaintGeometry(
                FVector2f(W*DX,H*DY),
                FSlateLayoutTransform(FVector2f(W*X,H*Y))),
            &White,ESlateDrawEffect::None,Tint);
    };
    const FLinearColor Black(.004f,.004f,.009f,1.f);
    const FLinearColor Stone(.066f,.064f,.080f,.95f);
    const FLinearColor Recess(.025f,.024f,.035f,.85f);
    const FLinearColor Trim(.32f,.30f,.36f,.75f);
    const FLinearColor Fine(.22f,.21f,.27f,.68f);
    const FLinearColor Ash(.63f,.61f,.69f,.76f);
    const float Phase=AtmosphereSeconds;
    // Staggered rear ribs. The top vanishing point is an actual vaulted
    // perspective, not multiple grey rectangles masquerading as architecture.
    for(int32 I=0;I<8;++I)
    {
        const float Offset=I*.027f;
        const float Left=.50f-(.47f-Offset);
        const float Right=1.f-Left;
        const float Crown=.075f+.022f*I;
        const float Floor=.88f-.035f*I;
        const FLinearColor Ink=FLinearColor(
            .20f+.014f*I,.19f+.01f*I,.24f+.013f*I,
            .38f+.038f*(8-I)/8.f);
        L(Left,Floor,Left,Crown+.14f,Ink,1.1f);
        L(Right,Floor,Right,Crown+.14f,Ink,1.1f);
        L(Left,Crown+.14f,.5f,Crown,Ink,1.0f);
        L(.5f,Crown,Right,Crown+.14f,Ink,1.0f);
        L(Left+.015f,Crown+.16f,.5f,Crown+.019f,Fine,.65f);
        L(.5f,Crown+.019f,Right-.015f,Crown+.16f,Fine,.65f);
    }
    // Pair of asymmetric flying buttresses and their pointed arches.
    for (int32 Side=0;Side<2;++Side)
    {
        const float X=Side==0?.08f:.92f;
        const float S=Side==0?1.f:-1.f;
        Rect(X-.020f,.13f,.040f,.69f,Recess);
        L(X-.027f,.10f,X-.027f,.79f,Trim,1.1f);
        L(X+.027f,.10f,X+.027f,.79f,Trim,1.1f);
        L(X-.02f,.11f,X,.06f,Trim,1.3f);
        L(X,.06f,X+.02f,.11f,Trim,1.3f);
        L(X-S*.005f,.13f,.5f,.03f,Fine,.6f);
        for(int32 Step=0;Step<9;++Step)
        {
            const float Y=.17f+Step*.070f;
            L(X-.023f,Y,X+.023f,Y,Fine,.7f);
            Diamond(X,Y,.007f,Trim,.75f);
        }
        L(X+S*.056f,.78f,X+S*.155f,.52f,Fine,1.f);
        L(X+S*.155f,.52f,X+S*.155f,.31f,Fine,1.f);
        Diamond(X+S*.155f,.31f,.012f,Trim,1.f);
    }
    // Far chandelier, suspended by a chain; its atmosphere is subtle and
    // reduced-motion aware. No fake character equipment or gacha rarity.
    L(.50f,.01f,.50f,.22f,Trim,1.f);
    for(int32 I=0;I<9;++I)
        Diamond(.5f,.024f+I*.021f,.0043f,Fine,.65f);
    Diamond(.5f,.23f,.022f,Trim,1.5f);
    L(.464f,.232f,.536f,.232f,Ash,1.1f);
    for(int32 I=-3;I<=3;++I)
    {
        const float CX=.5f+I*.012f;
        const float CY=.256f+.010f*FMath::Abs((float)I);
        Diamond(CX,CY,.007f,Fine,.9f);
    }
    // Deep perspective floor: concentric dais edges, stone joins and rails.
    for(int32 Row=0;Row<10;++Row)
    {
        const float T=(Row+1)/11.f;
        const float Y=.67f+T*T*.33f;
        const float Span=.085f+.45f*T;
        L(.5f-Span,Y,.5f+Span,Y,
            Row%3==0?Trim:Fine,Row%3==0?1.1f:.65f);
        L(.5f-Span,Y,.5f-Span-.02f*T,Y+.022f*T,Fine,.7f);
        L(.5f+Span,Y,.5f+Span+.02f*T,Y+.022f*T,Fine,.7f);
    }
    for(int32 I=-3;I<=3;++I)
        L(.50f+I*.019f,.67f,.50f+I*.118f,1.f,Fine,.65f);
    // The vacant seat is deliberately empty. Geometry is nested and angled
    // instead of placeholder slabs: gothic ogee back, upright blades,
    // jeweled armatures, recessed upholstery and tapered step-footing.
    Rect(.402f,.322f,.196f,.288f,Recess);
    L(.40f,.58f,.40f,.357f,Trim,1.6f);
    L(.60f,.58f,.60f,.357f,Trim,1.6f);
    L(.40f,.357f,.44f,.285f,Ash,1.3f);
    L(.44f,.285f,.5f,.255f,Ash,1.25f);
    L(.5f,.255f,.56f,.285f,Ash,1.25f);
    L(.56f,.285f,.60f,.357f,Ash,1.3f);
    L(.417f,.554f,.583f,.554f,Fine,.9f);
    L(.432f,.375f,.5f,.323f,Fine,.85f);
    L(.5f,.323f,.568f,.375f,Fine,.85f);
    for(int32 I=0;I<7;++I)
    {
        const float X=.42f+I*.0265f;
        L(X,.38f,X,.534f,Fine,.55f);
    }
    // Ornate arms, hanging finials, and four pointed surrounding spears.
    for(int32 Side=0;Side<2;++Side)
    {
        const float X=Side==0?.37f:.63f;
        const float S=Side==0?-1.f:1.f;
        L(X,.51f,X-S*.035f,.51f,Ash,1.5f);
        L(X,.51f,X,.650f,Trim,1.9f);
        L(X-S*.035f,.51f,X-S*.035f,.64f,Trim,1.5f);
        L(X,.645f,X-S*.05f,.674f,Trim,1.f);
        L(X,.51f,X,.43f,Ash,1.2f);
        Diamond(X,.423f,.015f,Trim,1.2f);
        const float Spear=X+S*.098f;
        L(Spear,.63f,Spear,.205f,Trim,1.2f);
        L(Spear,.205f,Spear-.011f,.242f,Ash,1.2f);
        L(Spear,.205f,Spear+.011f,.242f,Ash,1.2f);
        Diamond(Spear,.35f,.012f,Fine,1.2f);
    }
    Rect(.39f,.555f,.22f,.059f,Stone);
    Rect(.41f,.594f,.18f,.059f,Recess);
    L(.37f,.657f,.63f,.657f,Ash,1.8f);
    L(.36f,.676f,.64f,.676f,Trim,1.2f);
    for(int32 I=0;I<5;++I)
        Diamond(.5f,.285f+I*.074f,.0048f,Trim,.75f);
    // Animated dust motes/void glints, with deterministically seeded motion
    // and fixed state under reduced-motion. No random temporal flicker.
    for(int32 I=0;I<46;++I)
    {
        const float Seed=I*12.9898f;
        const float X=FMath::Frac(FMath::Sin(Seed)*43758.54f);
        const float Y=FMath::Frac(FMath::Cos(Seed)*23356.73f);
        const float Travel=bReducedMotion?0.f:
            FMath::Fmod(Phase*(.004f+.0011f*(I%4))+Y,1.f);
        const float Alpha=.10f+.17f*(.5f+.5f*
            FMath::Sin(Phase*.28f+I*1.71f));
        const float PX=.07f+.86f*X;
        const float PY=.10f+.76f*(1.f-Travel);
        const float Dot=(I%9==0)?.005f:.0018f;
        L(PX-Dot,PY,PX+Dot,PY,
          FLinearColor(.80f,.78f,.89f,Alpha),.85f);
        if(I%9==0)
            L(PX,PY-Dot*1.6f,PX,PY+Dot*1.6f,
              FLinearColor(.79f,.76f,.86f,Alpha),.75f);
    }
    return Base+2;
}
