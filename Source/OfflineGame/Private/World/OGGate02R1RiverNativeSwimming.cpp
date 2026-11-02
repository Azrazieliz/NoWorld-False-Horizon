// Gate02 source river traversal uses the SAME authored water mask + height
// as the visible river. It does not create fake rectangular water volumes or
// disable flying, diving, mounts, underwater abilities or other world rules.
#include "World/OGStartingRegionPresentation.h"
#include "World/OGTraversalFramework.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

void AOGWorldPrototypeCharacter::UpdateNativeGate02R1WaterTraversal()
{
    UWorld* World=GetWorld();
    UCharacterMovementComponent* Movement=GetCharacterMovement();
    if(!World || !World->IsGameWorld() || !Movement ||
       !TraversalCapabilities || !bNativeFirstWorldRuntimeInitialized)
        return;
    // Source R1 may unload/reload independently of the player pawn.
    if(!R1SourceWaterActor.IsValid())
    {
        for(TActorIterator<AOGStartingRegionGenerator> It(World);It;++It)
            if(It->bGate02R1RegionSource)
            {
                R1SourceWaterActor=*It;
                break;
            }
    }
    if(!R1SourceWaterActor.IsValid())
    {
        if(bR1SourceWaterSwimActive)
        {
            bR1SourceWaterSwimActive=false;
            if(Movement->MovementMode==MOVE_Swimming)
                Movement->SetMovementMode(MOVE_Falling);
        }
        return;
    }
    float WorldSurfaceZ=0.f;
    const bool bRiverAtPlayer=R1SourceWaterActor->
        QueryGate02R1SourceWaterSurface(GetActorLocation(),WorldSurfaceZ);
    const bool bSubmerged=bRiverAtPlayer &&
        WorldSurfaceZ > GetActorLocation().Z + 15.f;
    if(bR1SourceWaterSwimActive)
    {
        // Only undo swimming mode that this exact source river installed.
        // Other engine water bodies and traversal providers remain untouched.
        if(Movement->MovementMode!=MOVE_Swimming)
        {
            bR1SourceWaterSwimActive=false;
            return;
        }
        if(!bRiverAtPlayer || WorldSurfaceZ<GetActorLocation().Z-25.f ||
           !TraversalCapabilities->HasCapability(FOGTraversalCapabilityIds::Swim))
        {
            bR1SourceWaterSwimActive=false;
            Movement->SetMovementMode(MOVE_Falling);
        }
        return;
    }
    if(!bSubmerged || !IsFoundationWorldInputAllowed() ||
       CurrentCarrier.IsValid() ||
       !TraversalCapabilities->HasCapability(FOGTraversalCapabilityIds::Swim) ||
       (Movement->MovementMode!=MOVE_Walking &&
        Movement->MovementMode!=MOVE_Falling) ||
       TraversalCapabilities->GetTraversalMode()!=EOGTraversalMode::Ground)
        return;
    // Character crossed a real deep RiverVale surface. The existing
    // OnMovementModeChanged handler owns traversal capability and cancel
    // windows; no independent movement controller is instantiated.
    Movement->SetMovementMode(MOVE_Swimming);
    bR1SourceWaterSwimActive=Movement->MovementMode==MOVE_Swimming;
}
