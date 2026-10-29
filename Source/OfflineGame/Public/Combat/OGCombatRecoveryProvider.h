#pragma once
#include "CoreMinimal.h"
#include "Combat/OGTurnBattle.h"
class IOGWorldStore;

/** Installed code owns this factory. Save data names it and carries data only.
 * A factory receives the reopened Store; it must not retain a prior World's borrowed services.
 * Capture is read-only and runs inside the existing canonical action transaction. */
struct OFFLINEGAME_API FOGCombatRecoveryProvider
{
    FName Id;
    int32 Version=1;
    int32 MinimumReadableVersion=1;
    TFunction<bool(const FOGTurnBattle&,IOGWorldStore&,const FOGEntityId&,FString&,FString&)> Capture;
    TFunction<bool(IOGWorldStore&,const FOGEntityId&,int32,const FString&,FOGCombatRecoveryBindings&,FString&)> Restore;
};
